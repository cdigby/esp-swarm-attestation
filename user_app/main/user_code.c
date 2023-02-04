#include "user_code.h"

// #define WIFI_CONNECTED_BIT BIT0
// #define WIFI_FAIL_BIT      BIT1

// static EventGroupHandle_t wifi_event_group;
// static const char *TAG = "user_example_common";

// static int retries = 0;

// void event_handler(void* arg, esp_event_base_t event_base,
//                                 int32_t event_id, void* event_data)
// {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         esp_wifi_connect();
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         if (retries < 5) {
//             esp_wifi_connect();
//             ESP_LOGE(TAG, "Retry to connect to the AP");
//             retries++;
//         } else {
//             ESP_LOGE(TAG, "Failed to connect to the AP");
//             xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
//         }
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
//     }
// }

void user_main()
{
    ESP_LOGI(TAG_USER, "Userspace start");

    // //Setup wifi
    // nvs_flash_init();
    // esp_netif_init();
    // esp_event_loop_create_default();

    // esp_event_handler_instance_t instance_any_id;
    // esp_event_handler_instance_t instance_got_ip;

    // esp_event_handler_instance_register(
    //     WIFI_EVENT,
    //     ESP_EVENT_ANY_ID,
    //     &event_handler,
    //     NULL,
    //     &instance_any_id,
    // );

    // esp_event_handler_instance_register(
    //     IP_EVENT,
    //     IP_EVENT_STA_GOT_IP,
    //     &event_handler,
    //     NULL,
    //     &instance_got_ip
    // );

    // esp_wifi_set_mode(WIFI_MODE_APSTA);

    // wifi_config_t sta_config =
    // {
    //     .sta =
    //     {
    //         .
    //     }
    // };

    // esp_wifi_set_config(ESP_IF_WIFI_STA, sta_conf);
    // esp_wifi_set_config(ESP_IF_WIFI_AP, ap_conf);
    
    // esp_wifi_scan_start(NULL, true);

    // uint16_t scan_num = 0;
    // // esp_wifi_scan_get_ap_num(&scan_num);

    // ESP_LOGI(TAG_USER, "FOUND %u APS", scan_num);

    while (1)
    {
        // usr_test_syscall(NULL);
        vTaskDelay(1000);
    }
}
