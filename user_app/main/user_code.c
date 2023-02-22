#include "user_code.h"

void user_main()
{
    ESP_LOGI(TAG_USER, "Userspace start");

    // Wifi is initialised before user app boot, so we can start comms immediately
    comms_start();

    while (1)
    {
        // Loop here as user_main() must not return
        vTaskDelay(1000);
    }
}
