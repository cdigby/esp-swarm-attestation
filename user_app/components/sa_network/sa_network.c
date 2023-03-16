#include "sa_network.h"

static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;

static bool connection_lost = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base != WIFI_EVENT)
    {
        return;
    }

    switch (event_id)
    {

        case WIFI_EVENT_STA_START:
        {
            // Wifi started, attempt to connect to parent
            if (strcmp(NODE_PARENT, "") != 0)   // Do nothing if no parent is specified
            {
                ESP_LOGI(TAG_NETWORK, "Attempt connection to %s", NODE_PARENT);
                wifi_config_t cfg =
                {
                    .sta =
                    {
                        .ssid = NODE_PARENT,
                        .password = NODE_PASSWORD,
                    },
                };
                esp_wifi_set_config(WIFI_IF_STA, &cfg);
                esp_wifi_connect();
            }
        }
        break;

        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(TAG_NETWORK, "Connected to parent: %s", NODE_PARENT);
            connection_lost = false;
        }
        break;

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            // Connection to parent failed or was lost, delay and then try again
            if (strcmp(NODE_PARENT, "") != 0)
            {
                if (connection_lost == false)   // Flag so this message doesn't spam logs
                {
                    ESP_LOGW(TAG_NETWORK, "Connection to %s failed or was otherwise lost", NODE_PARENT);
                    connection_lost = true;
                }
                
                vTaskDelay(2000);
                esp_wifi_connect();
            }
        }
        break;

        case WIFI_EVENT_AP_STACONNECTED:
        {
            ESP_LOGI(TAG_NETWORK, "A node just connected to our AP");
        }
        break;

        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            ESP_LOGW(TAG_NETWORK, "A node just disconnected from our AP");
        }
        break;
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base != IP_EVENT)
    {
        return;
    }

    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            
        }
        break;

        case IP_EVENT_STA_LOST_IP:
        {
            
        }
        break;

        case IP_EVENT_AP_STAIPASSIGNED:
        {

        }
        break;
    }
}

// Return the gateway ip of the parent node
// If not connected, 0 will be returned
uint32_t sa_network_get_gateway_ip()
{
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(sta_netif, &ip_info);

    return ip_info.gw.addr;
}

void sa_network_init()
{
    // Init NVS flash
    nvs_flash_init();

    // Create event loop, register event handlers
    // Event handlers get a pointer to the network state when they are called
    esp_event_loop_create_default();
    esp_event_handler_instance_t wifi_event_context;
    esp_event_handler_instance_t ip_event_context;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_event_context);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, &ip_event_context);

    // Create default net interfaces
    esp_netif_init();
    ap_netif = esp_netif_create_default_wifi_ap();
    sta_netif = esp_netif_create_default_wifi_sta();

    // The default IP address for the access point is 192.168.4.1, but we need to change the 3rd number to be unique for each node:
    // If we leave everything as default, the TCP client will try to connect to 192.168.4.1 and end up connecting to the TCP server running on the same node.
    // If we only change the 4th number, a node could be using 192.168.4.2 for its access point and then be assigned the same IP for its station by its parent node,
    // which breaks TCP sockets (connect() results in the error: Software caused connection abort).
    // 
    // By changing the 3rd number according to the node ID, each node is guaranteed to be able to freely assign IPs in the range 192.168.NODE_ID.1-255
    // A better solution will be needed if we want more than 255 nodes on the network.
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192,168,NODE_ID,1);
	IP4_ADDR(&ip_info.gw, 192,168,NODE_ID,1);
	IP4_ADDR(&ip_info.netmask, 255,255,255,0);
	esp_netif_dhcps_stop(ap_netif);
	esp_netif_set_ip_info(ap_netif, &ip_info);
	esp_netif_dhcps_start(ap_netif);

    ESP_LOGI(TAG_NETWORK, "ap ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR, IP2STR(&ip_info.ip), IP2STR(&ip_info.netmask), IP2STR(&ip_info.gw));
    
    // Configure and start wifi
    // wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_init_config_t wifi_init_cfg;
    wifi_init_config_default(&wifi_init_cfg);
    esp_wifi_init(&wifi_init_cfg);

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    wifi_config_t ap_cfg =
    {
        .ap =
        {
            .ssid = NODE_SSID,
            .password = NODE_PASSWORD,
            .ssid_len = strlen(NODE_SSID),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 10,
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();

    ESP_LOGI(TAG_NETWORK, "Started access point: %s", NODE_SSID);
}