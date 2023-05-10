#include "user_code.h"

static const char *TAG_USER = "USERSPACE LOG";

void user_main()
{
    ESP_LOGI(TAG_USER, "Userspace start");

    // Wifi is initialised before user app boot, so we can start comms immediately
    sa_comms_init();

    // User app main loop: additional functionality could go here in the future
    // user_main() must never return
    while (1)
    {
        vTaskDelay(1000);
    }
}