#include "user_code.h"

void user_main()
{
    ESP_LOGI(TAG_USER, "Userspace start");

    // Wifi is initialised before user app boot, so we can start tcp host immediately
    tcp_host_start();

    tcp_client_start();

    while (1)
    {
        vTaskDelay(1000);
    }
}
