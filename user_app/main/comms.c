#include "comms.h"

static tcp_server_t tcp_server;
static tcp_client_t tcp_client;

// Get the current time in ms
static int64_t comms_get_time_ms()
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    return (int64_t)(((int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec) / 1000);
}

// Process a command
// cmd is the command code
// sock is the socket the command was received from in case further data is required
// tx_buf and rx_buf are the buffers to use for sending and receoving data
static void comms_cmd_process(uint8_t cmd, int sock, uint8_t* tx_buf, uint8_t *rx_buf)
{

}

// Register a connection as closed on the tcp server
static void tcp_server_close_conn(int sock)
{
    for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
    {
        if (tcp_server.conns[i].sock == sock)
        {
            close(tcp_server.conns[i].sock);
            tcp_server.conns[i].open = false;
            tcp_server.num_conns -= 1;
            ESP_LOGW(TAG_TCP_SERVER, "Lost connection with %s", tcp_server.conns[i].name);
        }
    }
}

// TCP server thread
static void tcp_server_task(void *pvParameters)
{
    int rlen = 0;
    ESP_LOGI(TAG_TCP_SERVER, "Started TCP server task");

    // Update ping timer
    // tcp_server.ping_timer = comms_get_time_ms();

    // Socket options
    int opt_keep_alive = 1;                                 // Keep socket alive
    int opt_keep_idle = TCP_SERVER_KEEPALIVE_IDLE;            // Seconds before sending keepalive packets
    int opt_keep_interval = TCP_SERVER_KEEPALIVE_INTERVAL;    // Interval in seconds between keepalive packets
    int opt_keep_count = TCP_SERVER_KEEPALIVE_COUNT;          // Number of keepalive packets to send before dropping connection
    int opt_no_delay = 1;                                   // Send data as soon as it is available, without buffering (disable Nagle's Algorithm)

    while (1)
    {
        // 1) LOOK FOR NEW CONNECTIONS

        // Poll listener socket for waiting connections
        struct pollfd listener_poll = 
        {
            .fd = tcp_server.listen_sock,
            .events = POLLIN,
            .revents = 0,
        };

        poll(&listener_poll, 1, 0);
        if (listener_poll.revents & POLLIN)
        {
            // Check if a connection slot is free
            tcp_conn_t *new_conn = NULL;
            if (tcp_server.num_conns < TCP_SERVER_MAX_CONNS)
            {
                for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
                {
                    if (tcp_server.conns[i].open == false)
                    {
                        new_conn = &tcp_server.conns[i];  // Get a pointer to the free slot
                    }
                }
            }

            if (new_conn != NULL)
            {
                // Try and accept a new connection
                struct sockaddr_storage client_addr;
                socklen_t addr_len = sizeof(client_addr);
                new_conn->sock = accept(tcp_server.listen_sock, (struct sockaddr *)&client_addr, &addr_len);

                if (new_conn->sock < 0)
                {
                    if (errno != EAGAIN)    // Ignore EAGAIN as this simply means no connection was available
                    {
                        ESP_LOGE(TAG_TCP_SERVER, "Error acception incoming connection (%s)", strerror(errno));
                    }
                }
                else
                {
                    // Set socket options
                    setsockopt(new_conn->sock, SOL_SOCKET, SO_KEEPALIVE, &opt_keep_alive, sizeof(int));
                    setsockopt(new_conn->sock, IPPROTO_TCP, TCP_KEEPIDLE, &opt_keep_idle, sizeof(int));
                    setsockopt(new_conn->sock, IPPROTO_TCP, TCP_KEEPINTVL, &opt_keep_interval, sizeof(int));
                    setsockopt(new_conn->sock, IPPROTO_TCP, TCP_KEEPCNT, &opt_keep_count, sizeof(int));
                    setsockopt(new_conn->sock, IPPROTO_TCP, TCP_NODELAY, &opt_no_delay, sizeof(int));

                    // Wait for the new client to transmit its name so we can finalise the connection record
                    recv(new_conn->sock, tcp_server.rx_buf, 1, MSG_WAITALL);
                    if (tcp_server.rx_buf[0] != CMD_NODE_NAME)
                    {
                        ESP_LOGE(TAG_TCP_SERVER, "New client did not transmit CMD_NODE_NAME");
                        close(new_conn->sock);
                    }
                    else
                    {
                        recv(new_conn->sock, tcp_server.rx_buf, 1, MSG_WAITALL);
                        uint8_t name_len = tcp_server.rx_buf[0];

                        recv(new_conn->sock, tcp_server.rx_buf, name_len, MSG_WAITALL);
                        memcpy(new_conn->name, tcp_server.rx_buf, name_len);
                        new_conn->open = true;
                        tcp_server.num_conns += 1;
                        ESP_LOGI(TAG_TCP_SERVER, "New client: %s", new_conn->name);
                    }
                }
            }
        }

        // 2) LOOK FOR PENDING COMMANDS FROM CLIENTS

        // Poll open connections for available data
        struct pollfd client_poll[tcp_server.num_conns];
        int j = 0;
        for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
        {
            if (tcp_server.conns[i].open == true)
            {
                client_poll[j].fd = tcp_server.conns[i].sock;
                client_poll[j].events = POLLIN;
                client_poll[j].revents = 0;
                j++;
            }
        }
        poll(client_poll, tcp_server.num_conns, 0);

        // Read available data
        for (int i = 0; i < tcp_server.num_conns; i++)
        {
            if (client_poll[i].revents & POLLIN)
            {
                rlen = recv(client_poll[i].fd, tcp_server.rx_buf, 1, MSG_WAITALL);
                if (rlen == 1)
                {
                    comms_cmd_process(tcp_server.rx_buf[0], client_poll[i].fd, tcp_server.tx_buf, tcp_server.rx_buf);
                }
                else
                {
                    tcp_server_close_conn(client_poll[i].fd);
                }
            }
        }

        // TODO detect dropped connections

        vTaskDelay(1);
    }
}

// TCP client thread
static void tcp_client_task(void *pvParameters)
{
    ESP_LOGI(TAG_TCP_CLIENT, "Started TCP client task");

    // Socket options
    int opt_no_delay = 1;   // Send data as soon as it is available, without buffering (disable Nagle's Algorithm)

    while (1)
    {
        // Delay before attempting connection again
        vTaskDelay(1000);

        // Use our syscall to retrieve the gateway ip
        uint32_t gw_ip = 0;
        if (usr_sa_network_get_gateway_ip(&gw_ip) == -1)
        {
            continue;   // Abort
        }

        // Create socket
        tcp_client.server.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (tcp_client.server.sock < 0)
        {
            ESP_LOGE(TAG_TCP_CLIENT, "Error creating socket (%s)", strerror(errno));
            continue;
        }

        // Set socket options
        setsockopt(tcp_client.server.sock, IPPROTO_TCP, TCP_NODELAY, &opt_no_delay, sizeof(opt_no_delay));

        // Connect to server
        struct sockaddr_in server_addr =
        {
            .sin_port = htons(COMMS_TCP_PORT),
            .sin_family = AF_INET,
            .sin_addr.s_addr = gw_ip,
        };

        ESP_LOGI(TAG_TCP_CLIENT, "Connecting to %s via %s:%d", tcp_client.server.name, inet_ntoa(gw_ip), COMMS_TCP_PORT);
        int err = connect(tcp_client.server.sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (err != 0)
        {
            ESP_LOGE(TAG_TCP_CLIENT, "Error connecting to server (%s)", strerror(errno));
            close(tcp_client.server.sock);
            continue;
        }

        tcp_client.server.open = true;
        ESP_LOGI(TAG_TCP_CLIENT, "Connected to %s", tcp_client.server.name);

        // Send node name
        uint8_t name_len = strlen(tcp_client.node_name) + 1;    // +1 for NULL terminator

        tcp_client.tx_buf[0] = CMD_NODE_NAME;
        tcp_client.tx_buf[1] = name_len; 
        memcpy(tcp_client.tx_buf + 2, tcp_client.node_name, name_len);

        send(tcp_client.server.sock, tcp_client.tx_buf, 2 + name_len, 0);
        
        // Client loop
        while (1)
        {

            vTaskDelay(1);
        }

        // Cleanup after exiting loop
        close(tcp_client.server.sock);
        tcp_client.server.open = false;
        ESP_LOGW(TAG_TCP_CLIENT, "Connection with %s closed", tcp_client.server.name);
    }
}

// Start the TCP server
bool tcp_server_start()
{
    // Initial server state
    for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
    {
        tcp_server.conns[i].open = false;
    }
    tcp_server.num_conns = 0;

    // Create listener socket
    struct sockaddr_in listen_addr =
    {
        .sin_port = htons(COMMS_TCP_PORT),
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    tcp_server.listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (tcp_server.listen_sock < 0)
    {
        ESP_LOGE(TAG_TCP_SERVER, "Error creating listener socket (%s)", strerror(errno));
        return false;
    }

    int opt = 1;
    setsockopt(tcp_server.listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGI(TAG_TCP_SERVER, "Created listener socket");

    // Bind listener socket
    int err = bind(tcp_server.listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG_TCP_SERVER, "Error binding socket (%s)", strerror(errno));
        close(tcp_server.listen_sock);
        return false;
    }
    ESP_LOGI(TAG_TCP_SERVER, "Listener socket bound to port %d", COMMS_TCP_PORT);

    // Start listening for connections
    err = listen(tcp_server.listen_sock, 10);     // 10 is the max clients we can have connected over wifi
    if (err != 0)
    {
        ESP_LOGE(TAG_TCP_SERVER, "Error occured during listen (%s)", strerror(errno));
        close(tcp_server.listen_sock);
        return false;
    }
    ESP_LOGI(TAG_TCP_SERVER, "Started listening for connections");

    // Start TCP server task
    xTaskCreate(tcp_server_task, "tcp_server", 2048, NULL, 10, NULL);

    return true;
}

// Start the TCP client
bool tcp_client_start()
{
    // Initial client state
    strcpy(tcp_client.node_name, NODE_SSID);
    tcp_client.server.open = false;

    // We are currently hardcoding each node's parent in the build config, so we already know the parent's name
    // A better implementation would be to have the server transmit its name to the client upon connection
    strcpy(tcp_client.server.name, NODE_PARENT);
   
    
    xTaskCreate(tcp_client_task, "tcp_client", 2048, NULL, 10, NULL);

    return true;
}