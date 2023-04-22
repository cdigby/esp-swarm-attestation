#include "sa_comms.h"

static tcp_server_t tcp_server;
static tcp_client_t tcp_client;

// Get the current time in ms
static int64_t sa_comms_get_time_ms()
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    return (int64_t)(((int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec) / 1000);
}

static void sa_comms_drop_connection(tcp_conn_t *conn)
{
    close(conn->sock);
    conn->open = false;

    // Free any data pointers
    comms_cmd_t cmd;
    while (xQueueReceive(conn->cmd_queue, &cmd, 0) == pdTRUE)
    {
        if (cmd.data_len > 0)
        {
            free(cmd.data);
        }
    }

    xQueueReset(conn->cmd_queue);
}

static bool sa_comms_send(tcp_conn_t *conn, uint8_t *data, size_t len)
{
    if (xSemaphoreTake(conn->sock_mutex, portMAX_DELAY) == pdTRUE)
    {
        ssize_t sent = send(conn->sock, data, len, 0);
        if (sent == -1)
        {
            sa_comms_drop_connection(conn);
            ESP_LOGW(TAG_COMMS, "Connection with %s dropped due to socket error (%s)", conn->name, strerror(errno));
            xSemaphoreGive(conn->sock_mutex);
            return false;
        }
        else if (sent < len)
        {
            sa_comms_drop_connection(conn);
            ESP_LOGW(TAG_COMMS, "Connection with %s dropped due to incomplete send", conn->name);
            xSemaphoreGive(conn->sock_mutex);
            return false;
        }

        xSemaphoreGive(conn->sock_mutex);
        return true;
    }
    else
    {
        ESP_LOGE(TAG_COMMS, "Could not acquire socket mutex for %s, this probably indicates a bug", conn->name);
        return false;
    }
}

static bool sa_comms_recv(tcp_conn_t *conn, uint8_t *rx_buf, size_t len)
{
    if (xSemaphoreTake(conn->sock_mutex, portMAX_DELAY) == pdTRUE)
    {
        ssize_t rlen = recv(conn->sock, rx_buf, len, MSG_WAITALL);
        if (rlen == -1)
        {
            sa_comms_drop_connection(conn);
            ESP_LOGW(TAG_COMMS, "Connection with %s dropped due to socket error (%s)", conn->name, strerror(errno));
            xSemaphoreGive(conn->sock_mutex);
            return false;
        }
        else if (rlen < len)
        {
            sa_comms_drop_connection(conn);
            ESP_LOGW(TAG_COMMS, "Connection with %s dropped due to incomplete receive", conn->name);
            xSemaphoreGive(conn->sock_mutex);
            return false;
        }

        xSemaphoreGive(conn->sock_mutex);
        return true;
    }
    else
    {
        ESP_LOGE(TAG_COMMS, "Could not acquire socket mutex for %s, this probably indicates a bug", conn->name);
        return false;
    }
}

// Process the incoming command for conn, returning true
// It is assumed that conn->sock has been polled and is ready to read
// If an error occurs, conn will be dropped and false will be returned
static bool sa_comms_cmd_process_incoming(tcp_conn_t *conn, uint8_t *rx_buf)
{
    // Receive command code
    if (sa_comms_recv(conn, rx_buf, 1) == false) return false;

    // Process
    bool success = true;
    uint8_t cmd_code = rx_buf[0];
    switch (cmd_code)
    {
        case CMD_HEARTBEAT_REQUEST:
        {
            ESP_LOGI(TAG_COMMS, "Got heartbeat request from %s", conn->name);
            comms_cmd_t cmd =
            {
                .cmd_code = CMD_HEARTBEAT_RESPONSE,
                .data_len = 0,
                .data = NULL,
            };

            xQueueSendToBack(conn->cmd_queue, &cmd, 0);
        }
        break;

        case CMD_HEARTBEAT_RESPONSE:
        {
            ESP_LOGI(TAG_COMMS, "Got heartbeat response from %s", conn->name);
            conn->heartbeat = true;
        }
        break;

        case CMD_PRINT_MESSAGE:
        {
            if (sa_comms_recv(conn, rx_buf, 1) == false) return false;
            uint8_t msg_len = rx_buf[0];

            if (sa_comms_recv(conn, rx_buf, msg_len) == false) return false;
            ESP_LOGI(TAG_COMMS, "Received message: \"%s\" from %s", rx_buf, conn->name);
        }
        break;

        case CMD_SIMPLE_ATTEST:
        {
            if (sa_comms_recv(conn, rx_buf, SIMPLE_MSG_LEN + SIMPLE_HMAC_LEN) == false) return false;
            ESP_LOGI(TAG_COMMS, "Received SIMPLE attestation request from %s", conn->name);

            // Make syscall - execute algorithm in protected space
            uint8_t msg[SIMPLE_MSG_LEN];
            uint8_t h[SIMPLE_HMAC_LEN];
            memcpy(msg, rx_buf, SIMPLE_MSG_LEN);
            memcpy(h, rx_buf + SIMPLE_MSG_LEN, SIMPLE_HMAC_LEN);
            simple_prover(msg, h, conn->sock, conn->sock_mutex);
            ESP_LOGI(TAG_COMMS, "Processed SIMPLE attestation request from %s", conn->name);
        }
        break;

        case CMD_CLOSE_CONN:
        {
            sa_comms_drop_connection(conn);
            ESP_LOGI(TAG_COMMS, "Connection with %s closed gracefully", conn->name);
        }
        break;
    }

    return success;
}

// Process the next outgoing command for conn, if one is available, returning true
// If an error occurs, conn will be dropped and false will be returned
static bool sa_comms_cmd_process_outgoing(tcp_conn_t *conn)
{
    comms_cmd_t cmd;
    if (xQueueReceive(conn->cmd_queue, &cmd, 0) == pdFALSE) return true; // Nothing to process

    // Process
    bool success = true;
    switch (cmd.cmd_code)
    {
        case CMD_HEARTBEAT_REQUEST:
        {   
            success = sa_comms_send(conn, &cmd.cmd_code, 1);
            if (success == false) break;

            ESP_LOGI(TAG_COMMS, "Sent heartbeat request to %s", conn->name);
            conn->heartbeat = false;
        }
        break;

        case CMD_HEARTBEAT_RESPONSE:
        {
            success = sa_comms_send(conn, &cmd.cmd_code, 1);
            ESP_LOGI(TAG_COMMS, "Sent heartbeat response to %s", conn->name);
        }
        break;

        case CMD_PRINT_MESSAGE:
        {
            success = sa_comms_send(conn, &cmd.cmd_code, 1);
            if (success == false) break;

            success = sa_comms_send(conn, &cmd.data_len, 1);
            if (success == false) break;

            success = sa_comms_send(conn, cmd.data, cmd.data_len);
            ESP_LOGI(TAG_COMMS, "Sent message \"%s\" to %s", cmd.data, conn->name);
        }
        break;
    }

    // Cleanup
    if (cmd.data_len > 0)
    {
        free(cmd.data);
    }
    
    return success;
}

// TCP server thread
static void tcp_server_task(void *pvParameters)
{
    int rlen = 0;
    ESP_LOGI(TAG_COMMS, "[server] Started TCP server task");

    // Socket options
    int opt_keep_alive = 1;                                 // Keep socket alive
    int opt_keep_idle = TCP_SERVER_KEEPALIVE_IDLE;            // Seconds before sending keepalive packets
    int opt_keep_interval = TCP_SERVER_KEEPALIVE_INTERVAL;    // Interval in seconds between keepalive packets
    int opt_keep_count = TCP_SERVER_KEEPALIVE_COUNT;          // Number of keepalive packets to send before dropping connection
    int opt_no_delay = 1;                                   // Send data as soon as it is available, without buffering (disable Nagle's Algorithm)

    while (1)
    {
        // 1) LISTEN FOR NEW CONNECTIONS

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
                        ESP_LOGE(TAG_COMMS, "[server] Error acception incoming connection (%s)", strerror(errno));
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
                    rlen = recv(new_conn->sock, tcp_server.rx_buf, 1, MSG_WAITALL);
                    if ((rlen != 1) || (tcp_server.rx_buf[0] != CMD_NODE_NAME))
                    {
                        ESP_LOGE(TAG_COMMS, "[server] New client did not transmit CMD_NODE_NAME");
                        close(new_conn->sock);
                    }
                    else
                    {
                        rlen = recv(new_conn->sock, tcp_server.rx_buf, 1, MSG_WAITALL);
                        if (rlen != 1)
                        {
                            ESP_LOGE(TAG_COMMS, "[server] New client did not transmit name length");
                            close(new_conn->sock);
                        }
                        else
                        {
                            uint8_t name_len = tcp_server.rx_buf[0];

                            rlen = recv(new_conn->sock, tcp_server.rx_buf, name_len, MSG_WAITALL);
                            if (rlen != name_len)
                            {
                                ESP_LOGE(TAG_COMMS, "[server] New client transmitted no or incomplete name");
                                close(new_conn->sock);
                            }
                            else
                            {
                                memcpy(new_conn->name, tcp_server.rx_buf, name_len);
                                new_conn->open = true;
                                new_conn->heartbeat = true;     // Assumed alive for the next heartbeat timeout
                                tcp_server.num_conns += 1;
                                ESP_LOGI(TAG_COMMS, "[server] New client: %s", new_conn->name);
                            }    
                        }
                    }
                }
            }
        }

        // 2) RECEIVE COMMANDS FROM CLIENTS

        // Poll open connections for available data
        struct pollfd client_poll[tcp_server.num_conns];
        tcp_conn_t *open_conns[tcp_server.num_conns];      // Array of pointers to open connections, in the order they are polled for data
        int j = 0;
        for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
        {
            tcp_conn_t *conn = &tcp_server.conns[i];
            if (conn->open == true)
            {
                open_conns[j] = conn;

                client_poll[j].fd = conn->sock;
                client_poll[j].events = POLLIN;
                client_poll[j].revents = 0;

                j++;
            }
        }
        poll(client_poll, tcp_server.num_conns, 0);

        // Receive data
        for (int i = 0; i < tcp_server.num_conns; i++)
        {
            if (client_poll[i].revents & POLLIN)
            {
                if (sa_comms_cmd_process_incoming(open_conns[i], tcp_server.rx_buf) == false)
                {
                    tcp_server.num_conns -= 1;  // If there was an error, the conn was dropped so update num_conns
                }
            }
        }

        // 3) SEND PENDING COMMANDS TO CLIENTS
        for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
        {
            tcp_conn_t *conn = &tcp_server.conns[i];
            if (conn->open == true)
            {
                if (sa_comms_cmd_process_outgoing(conn) == false)
                {
                    tcp_server.num_conns -= 1;
                }
            }
        }
        
        // 4) DETECT DROPPED CONNECTIONS
        if (sa_comms_get_time_ms() - tcp_server.heartbeat_timer > COMMS_HEARTBEAT_TIMEOUT_MS)
        {
            for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
            {
                tcp_conn_t *conn = &tcp_server.conns[i];

                // Do not send heartbeats to verifier
                if (strcmp(conn->name, "VERIFIER") == 0)
                {
                    continue;
                }

                if ((conn->open == true) && (conn->heartbeat == false))
                {
                    // No response to heartbeat, drop connection
                    sa_comms_drop_connection(conn);
                    ESP_LOGW(TAG_COMMS, "[server] Dropped %s as it did not respond to heartbeat within %dms", conn->name, COMMS_HEARTBEAT_TIMEOUT_MS);
                }
                else if ((conn->open == true) && (conn->heartbeat == true))
                {
                    // Send next heartbeat
                    comms_cmd_t cmd =
                    {
                        .cmd_code = CMD_HEARTBEAT_REQUEST,
                        .data_len = 0,
                        .data = NULL,
                    };

                    xQueueSendToBack(conn->cmd_queue, &cmd, 0);
                }
            }

            // Reset timer
            tcp_server.heartbeat_timer = sa_comms_get_time_ms();
        }


        vTaskDelay(1);
    }
}

// TCP client thread
static void tcp_client_task(void *pvParameters)
{
    ESP_LOGI(TAG_COMMS, "[client] Started TCP client task");

    // Socket options
    int opt_no_delay = 1;   // Send data as soon as it is available, without buffering (disable Nagle's Algorithm)

    while (1)
    {
        // Delay before attempting connection again
        vTaskDelay(1000);

        // Get the gateway ip
        uint32_t gw_ip = sa_network_get_gateway_ip();
        if (gw_ip == 0)
        {
            continue;   // Not connected, abort
        }

        // Create socket
        tcp_client.server.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (tcp_client.server.sock < 0)
        {
            ESP_LOGE(TAG_COMMS, "[client] Error creating socket (%s)", strerror(errno));
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

        ESP_LOGI(TAG_COMMS, "[client] Connecting to %s via %s:%d", tcp_client.server.name, inet_ntoa(gw_ip), COMMS_TCP_PORT);
        int err = connect(tcp_client.server.sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (err != 0)
        {
            ESP_LOGE(TAG_COMMS, "[client] Error connecting to server (%s)", strerror(errno));
            close(tcp_client.server.sock);
            continue;
        }

        tcp_client.server.open = true;
        tcp_client.server.heartbeat = true;     // Assumed alive for the next heartbeat timeout
        ESP_LOGI(TAG_COMMS, "[client] Connected to %s", tcp_client.server.name);

        // Send node name
        uint8_t name_len = strlen(tcp_client.node_name) + 1;    // +1 for NULL terminator

        tcp_client.tx_buf[0] = CMD_NODE_NAME;
        tcp_client.tx_buf[1] = name_len; 
        memcpy(tcp_client.tx_buf + 2, tcp_client.node_name, name_len);

        send(tcp_client.server.sock, tcp_client.tx_buf, 2 + name_len, 0);
        
        // Client loop
        while (1)
        {
            // 1) RECEIVE COMMANDS FROM SERVER

            // Poll server for available data
            struct pollfd server_poll = 
            {
                .fd = tcp_client.server.sock,
                .events = POLLIN,
                .revents = 0,
            };

            poll(&server_poll, 1, 0);

            // Receive data if available
            if (server_poll.revents & POLLIN)
            {
                if (sa_comms_cmd_process_incoming(&tcp_client.server, tcp_client.rx_buf) == false)
                {
                    break;  // Socket error, break out of loop
                }
            }

            // 3) SEND PENDING COMMANDS TO SERVER
            if (sa_comms_cmd_process_outgoing(&tcp_client.server) == false)
            {
                break;
            }

            // 4) DROP SERVER CONNECTION IF IT DID NOT RESPOND TO HEARTBEAT
            if (sa_comms_get_time_ms() - tcp_client.heartbeat_timer > COMMS_HEARTBEAT_TIMEOUT_MS)
            {   
                if (tcp_client.server.heartbeat == false)
                {
                    // No response to heartbeat, drop connection
                    ESP_LOGW(TAG_COMMS, "[client] Server did not respond to heartbeat within %dms", COMMS_HEARTBEAT_TIMEOUT_MS);
                    break;
                }
                else
                {
                    // Send next heartbeat
                    comms_cmd_t cmd =
                    {
                        .cmd_code = CMD_HEARTBEAT_REQUEST,
                        .data_len = 0,
                        .data = NULL,
                    };
                    xQueueSendToBack(tcp_client.server.cmd_queue, &cmd, 0);
                }

                // Reset timer
                tcp_client.heartbeat_timer = sa_comms_get_time_ms();
            }

            vTaskDelay(1);
        }

        // Cleanup after exiting loop
        sa_comms_drop_connection(&tcp_client.server);
        ESP_LOGW(TAG_COMMS, "[client] Connection with %s closed", tcp_client.server.name);
    }
}

// Start the TCP server
static bool tcp_server_start()
{
    // Initial server state
    for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
    {
        tcp_server.conns[i].open = false;
        tcp_server.conns[i].heartbeat = false;
        tcp_server.conns[i].cmd_queue = xQueueCreate(COMMS_QUEUE_LENGTH, sizeof(comms_cmd_t));
        tcp_server.conns[i].sock_mutex = xSemaphoreCreateMutex();
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
        ESP_LOGE(TAG_COMMS, "[server] Error creating listener socket (%s)", strerror(errno));
        return false;
    }

    int opt = 1;
    setsockopt(tcp_server.listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGI(TAG_COMMS, "[server] Created listener socket");

    // Bind listener socket
    int err = bind(tcp_server.listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG_COMMS, "[server] Error binding socket (%s)", strerror(errno));
        close(tcp_server.listen_sock);
        return false;
    }
    ESP_LOGI(TAG_COMMS, "[server] Listener socket bound to port %d", COMMS_TCP_PORT);

    // Start listening for connections
    err = listen(tcp_server.listen_sock, 10);     // 10 is the max clients we can have connected over wifi
    if (err != 0)
    {
        ESP_LOGE(TAG_COMMS, "[server] Error occured during listen (%s)", strerror(errno));
        close(tcp_server.listen_sock);
        return false;
    }
    ESP_LOGI(TAG_COMMS, "[server] Started listening for connections");

    // Start TCP server task
    xTaskCreate(tcp_server_task, "tcp_server", 2048, NULL, 10, NULL);

    return true;
}

// Start the TCP client
static void tcp_client_start()
{
    // Initial client state
    strcpy(tcp_client.node_name, NODE_SSID);
    tcp_client.server.open = false;
    tcp_client.server.heartbeat = false;
    tcp_client.server.cmd_queue = xQueueCreate(COMMS_QUEUE_LENGTH, sizeof(comms_cmd_t));
    tcp_client.server.sock_mutex = xSemaphoreCreateMutex();

    // We are currently hardcoding each node's parent in the build config, so we already know the parent's name
    // A better implementation would be to have the server transmit its name to the client upon connection
    strcpy(tcp_client.server.name, NODE_PARENT);
   
    
    xTaskCreate(tcp_client_task, "tcp_client", 2048, NULL, 10, NULL);
}

static void sa_comms_clone_cmd(comms_cmd_t *dest, comms_cmd_t *src)
{
    dest->cmd_code = src->cmd_code;
    dest->data_len = src->data_len;

    if (src->data_len > 0)
    {
        dest->data = malloc(src->data_len);
        memcpy(dest->data, src->data, src->data_len);
    }
    else
    {
        dest->data = NULL;
    }
}

// Start TCP server and client for comms
bool sa_comms_init()
{
    // Server start can fail (client start can't)
    // Return early if server start fails
    if (tcp_server_start() == false)
    {
        return false;
    }
    
    tcp_client_start();
    return true;
}

// Broadcast a command to all connections
// Data pointer is freed before returning
void sa_comms_broadcast(comms_cmd_t *cmd)
{
    // cmd may contain a pointer to additional data. This must not be freed until all instances of the command have been sent.
    // To reduce complexity, I am getting around this by making multiple copies of the data, although this is very inefficient.
    comms_cmd_t clone;

    // If client is connected to server, send to server
    if (tcp_client.server.open == true)
    {
        sa_comms_clone_cmd(&clone, cmd);
        xQueueSendToBack(tcp_client.server.cmd_queue, &clone, 0);
    }

    // Send to any clients connected to the server
    for (int i = 0; i < TCP_SERVER_MAX_CONNS; i++)
    {
        tcp_conn_t *conn = &tcp_server.conns[i];
        if (conn->open == true)
        {
            sa_comms_clone_cmd(&clone, cmd);
            xQueueSendToBack(conn->cmd_queue, &clone, 0);
        }
    }

    // Since we made copies of the command, free the original command's data pointer
    if (cmd->data_len > 0)
    {
        free(cmd->data);
    }
}