#include "network.h"

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // START SCANNING
        if (strcmp(NODE_PARENT, "") != 0)   // Only scan if a parent is specified
        {
            esp_wifi_scan_start(NULL, false);
            ESP_LOGI(TAG_NETWORK, "Start scan");
        }
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
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
                // strcpy((char*)cfg.sta.ssid, NODE_PARENT);
                // strcpy((char*)cfg.sta.password, NODE_PASSWORD);
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
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        // REGISTER SOMEWHERE THAT WE HAVE CONNECTED TO OUR PARENT
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // PARENT LOST, START SCANNING AGAIN
        if (strcmp(NODE_PARENT, "") != 0)
        {
            ESP_LOGW(TAG_NETWORK, "Lost connection to %s, restart scanning", NODE_PARENT);
            esp_wifi_scan_start(NULL, false);
            ESP_LOGI(TAG_NETWORK, "Start scan");
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        // A NODE JUST CONNECTED TO US, REGISTER IT
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        // A NODE JUST DISCONNECTED FROM US, UNREGISTER IT
    }
}

void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP)
    {

    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED)
    {

    }
}

void initialise_wifi()
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL);

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