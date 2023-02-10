#include "comms.h"

static tcp_host_t tcp_host;
// static tcp_client_t tcp_client;

// Process a command
// cmd is the command code
// sock is the socket the command was received from in case further data is required
// tx_buf and rx_buf are the buffers to use for sending and receoving data
static void comms_cmd_process(uint8_t cmd, int sock, uint8_t* tx_buf, uint8_t *rx_buf)
{

}

static void tcp_host_task(void *pvParameters)
{
    ESP_LOGI(TAG_TCP_HOST, "Started TCP host task");

    // Socket options
    int opt_keep_alive = 1;                                 // Keep socket alive
    int opt_keep_idle = TCP_HOST_KEEPALIVE_IDLE;            // Seconds before sending keepalive packets
    int opt_keep_interval = TCP_HOST_KEEPALIVE_INTERVAL;    // Interval in seconds between keepalive packets
    int opt_keep_count = TCP_HOST_KEEPALIVE_COUNT;          // Number of keepalive packets to send before dropping connection
    int opt_no_delay = 1;                                   // Send data as soon as it is available, without buffering (disable Nagle's Algorithm)

    while (1)
    {
        // 1) LOOK FOR NEW CONNECTIONS

        // Poll listener socket for waiting connections
        struct pollfd listener_poll = 
        {
            .fd = tcp_host.listen_sock,
            .events = POLLIN,
            .revents = 0,
        };

        poll(&listener_poll, 1, 0);
        if (listener_poll.revents & POLLIN)
        {
            // Check if a connection slot is free
            tcp_conn_t *new_conn = NULL;
            if (tcp_host.num_conns < TCP_HOST_MAX_CONNS)
            {
                for (int i = 0; i < TCP_HOST_MAX_CONNS; i++)
                {
                    if (tcp_host.conns[i].open == false)
                    {
                        new_conn = &tcp_host.conns[i];  // Get a pointer to the free slot
                    }
                }
            }

            if (new_conn != NULL)
            {
                // Try and accept a new connection
                struct sockaddr_storage client_addr;
                socklen_t addr_len = sizeof(client_addr);
                new_conn->sock = accept(tcp_host.listen_sock, (struct sockaddr *)&client_addr, &addr_len);

                if (new_conn->sock < 0)
                {
                    if (errno != EAGAIN)    // Ignore EAGAIN as this simply means no connection was available
                    {
                        ESP_LOGE(TAG_TCP_HOST, "Error acception incoming connection (%s)", strerror(errno));
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
                    recv(new_conn->sock, tcp_host.rx_buf, 1, MSG_WAITALL);
                    if (tcp_host.rx_buf[0] != CMD_NODE_NAME)
                    {
                        ESP_LOGE(TAG_TCP_HOST, "New client did not transmit CMD_NODE_NAME");
                        close(new_conn->sock);
                    }
                    else
                    {
                        recv(new_conn->sock, tcp_host.rx_buf, 1, MSG_WAITALL);
                        uint8_t name_len = tcp_host.rx_buf[0];

                        recv(new_conn->sock, tcp_host.rx_buf, name_len, MSG_WAITALL);
                        memcpy(new_conn->name, tcp_host.rx_buf, name_len);
                        new_conn->open = true;
                        tcp_host.num_conns += 1;
                    }
                }
            }
        }

        // 2) LOOK FOR PENDING COMMANDS FROM CLIENTS

        // Poll open connections for available data
        struct pollfd client_poll[tcp_host.num_conns];
        int j = 0;
        for (int i = 0; i < TCP_HOST_MAX_CONNS; i++)
        {
            if (tcp_host.conns[i].open == true)
            {
                client_poll[j].fd = tcp_host.conns[i].sock;
                client_poll[j].events = POLLIN;
                client_poll[j].revents = 0;
                j++;
            }
        }
        poll(client_poll, tcp_host.num_conns, 0);

        // Read available data
        for (int i = 0; i < tcp_host.num_conns; i++)
        {
            if (client_poll[i].revents & POLLIN)
            {
                recv(client_poll[i].fd, tcp_host.rx_buf, 1, MSG_WAITALL);
                comms_cmd_process(tcp_host.rx_buf[0], client_poll[i].fd, tcp_host.tx_buf, tcp_host.rx_buf);
            }
        }



        // SEND ANY COMMANDS WE HAVE QUEUED IF THE DESTINATION CLIENT IS CONNECTED (just one? or all of them?)

        // How do we know if a connection was closed?
        vTaskDelay(1);
    }
}

bool tcp_host_start()
{
    // Reset state
    tcp_host.state = TCP_HOST_IDLE;
    for (int i = 0; i < TCP_HOST_MAX_CONNS; i++)
    {
        tcp_host.conns[i].open = false;
    }
    tcp_host.num_conns = 0;

    // Create listener socket
    struct sockaddr_in listen_addr =
    {
        .sin_port = htons(COMMS_TCP_PORT),
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    tcp_host.listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (tcp_host.listen_sock < 0)
    {
        ESP_LOGE(TAG_TCP_HOST, "Error creating listener socket (%s)", strerror(errno));
        return false;
    }

    int opt = 1;
    setsockopt(tcp_host.listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGI(TAG_TCP_HOST, "Created listener socket");

    // Bind listener socket
    int err = bind(tcp_host.listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG_TCP_HOST, "Error binding socket (%s)", strerror(errno));
        close(tcp_host.listen_sock);
        return false;
    }
    ESP_LOGI(TAG_TCP_HOST, "Listener socket bound to port %d", COMMS_TCP_PORT);

    // Start listening for connections
    err = listen(tcp_host.listen_sock, 10);     // 10 is the max clients we can have connected over wifi
    if (err != 0)
    {
        ESP_LOGE(TAG_TCP_HOST, "Error occured during listen (%s)", strerror(errno));
        close(tcp_host.listen_sock);
        return false;
    }
    ESP_LOGI(TAG_TCP_HOST, "Started listening for connections");

    // Start TCP host task
    xTaskCreate(tcp_host_task, "tcp_host", 2048, NULL, 10, NULL);

    return true;
}