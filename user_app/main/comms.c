#include "comms.h"

static tcp_host_t tcp_host;
// static tcp_client_t tcp_client;

static void tcp_host_task(void *pvParameters)
{
    ESP_LOGI(TAG_TCP_HOST, "Started TCP host task");

    // Socket options
    int opt_keep_alive = 1;
    int opt_keep_idle = TCP_HOST_KEEPALIVE_IDLE;
    int opt_keep_interval = TCP_HOST_KEEPALIVE_INTERVAL;
    int opt_keep_count = TCP_HOST_KEEPALIVE_COUNT;
    int opt_no_delay = 1;

    while (1)
    {
        // TRY AND ACCEPT A NEW CONNECTION

        // LOOK FOR NEW COMMANDS FROM CLIENTS
            // for each open client, recv

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