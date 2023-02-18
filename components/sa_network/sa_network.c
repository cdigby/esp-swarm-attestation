#include "sa_network.h"

static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;

static bool connected_to_parent;

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
            // START SCANNING
            if (strcmp(NODE_PARENT, "") != 0)   // Only scan if a parent is specified
            {
                esp_wifi_scan_start(NULL, false);
                ESP_LOGI(TAG_NETWORK, "Start scan");
            }
        }
        break;

        case WIFI_EVENT_SCAN_DONE:
        {
            // LOOK FOR OUR PARENT AND CONNECT TO IT, OTHERWISE SCAN AGAIN
            ESP_LOGI(TAG_NETWORK, "Scan complete");

            // Find out how many access ponts were found
            uint16_t scan_count = 0;
            esp_wifi_scan_get_ap_num(&scan_count);

            // Get scan records
            wifi_ap_record_t *records = malloc(sizeof(wifi_ap_record_t) * scan_count);
            esp_wifi_scan_get_ap_records(&scan_count, records);

            // Look for our parent
            bool found = false;
            for (int i = 0; i < scan_count; i++)
            {
                if (strcmp((char*)records[i].ssid, NODE_PARENT) == 0)
                {
                    // Connect
                    ESP_LOGI(TAG_NETWORK, "Found parent node: %s", NODE_PARENT);
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
                    found = true;
                    break;
                }
            }

            if (found == false)
            {
                esp_wifi_scan_start(NULL, false);
                ESP_LOGI(TAG_NETWORK, "Parent not found, scan again");
            }

            free(records);
        }
        break;

        case WIFI_EVENT_STA_CONNECTED:
        {
            // REGISTER SOMEWHERE THAT WE HAVE CONNECTED TO OUR PARENT
        }
        break;

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            // PARENT LOST, START SCANNING AGAIN
            if (strcmp(NODE_PARENT, "") != 0)
            {
                ESP_LOGW(TAG_NETWORK, "Lost connection to %s, restart scanning", NODE_PARENT);
                esp_wifi_scan_start(NULL, false);
                ESP_LOGI(TAG_NETWORK, "Start scan");
            }
        }
        break;

        case WIFI_EVENT_AP_STACONNECTED:
        {
            // A NODE JUST CONNECTED TO US, REGISTER IT
        }
        break;

        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            // A NODE JUST DISCONNECTED FROM US, UNREGISTER IT
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
            connected_to_parent = true;
        }
        break;

        case IP_EVENT_STA_LOST_IP:
        {
            connected_to_parent = false;
        }
        break;

        case IP_EVENT_AP_STAIPASSIGNED:
        {

        }
        break;
    }
}

// If connected to parent, return 0 and store the parent's gateway address in result
// If not connected, return -1
int sa_network_get_gateway_addr(uint32_t *result)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(sta_netif, &ip_info);

    if (connected_to_parent == true)
    {
        *result = ip_info.gw.addr;
        return 0;
    }
    
    return -1;
}

void sa_network_init()
{
    connected_to_parent = false;

    // Init NVS flash
    nvs_flash_init();

    // Create event loop, register event handlers
    // Event handlers get a pointer to the network state when they are called
    esp_event_loop_create_default();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL);

    // Create default net interfaces
    esp_netif_init();
    ap_netif = esp_netif_create_default_wifi_ap();
    sta_netif = esp_netif_create_default_wifi_sta();
    
    // Configure and start wifi
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);
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