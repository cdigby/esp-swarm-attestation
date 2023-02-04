#include "protected_main.h"

IRAM_ATTR void user_app_exception_handler(void *arg)
{
    // Perform actions when user app exception happens
    ESP_LOGE(TAG_KERNEL, "USER APP EXCEPTION OCCURRED\n");
}

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // START SCANNING
        esp_wifi_scan_start(NULL, false);
        ESP_LOGI(TAG_KERNEL, "Start scan");
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        // LOOK FOR OUR PARENT AND CONNECT TO IT, OTHERWISE SCAN AGAIN
        ESP_LOGI(TAG_KERNEL, "Scan complete");

        // Find out how many access ponts were found
        uint16_t scan_count = 0;
        esp_wifi_scan_get_ap_num(&scan_count);

        // Get scan records
        wifi_ap_record_t *records = malloc(sizeof(wifi_ap_record_t) * scan_count);
        esp_wifi_scan_get_ap_records(&scan_count, records);

        // Look for our parent
        for (int i = 0; i < scan_count; i++)
        {
            ESP_LOGI(TAG_KERNEL, "found ap: %s", records[i].ssid);
        }

        free(records);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        // REGISTER SOMEWHERE THAT WE HAVE CONNECTED TO OUR PARENT
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // PARENT LOST, START SCANNING AGAIN?
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

// Protected app entry point
void app_main()
{
    esp_err_t ret;

    // Initialise protected environment
    ret = esp_priv_access_init(user_app_exception_handler);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_KERNEL, "Failed to initialize protected environment %d\n", ret);
    }

    // esp_priv_access_set_periph_perm(PA_GPIO, PA_WORLD_1, PA_PERM_ALL);

    //Setup wifi
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
    esp_wifi_start();

    // Boot user app
    ret = esp_priv_access_user_boot();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_KERNEL, "Failed to boot user app %d\n", ret);
    }

    // Main kernel loop
    while (1)
    {
        // ESP_LOGI(TAG_KERNEL, "kernel test\n");
        vTaskDelay(10);
    }
}
