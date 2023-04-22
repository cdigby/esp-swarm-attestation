#include "user_code.h"

// #define TEST_MSG "Hello!"

void user_main()
{
    ESP_LOGI(TAG_USER, "Userspace start");

    // Initialise network, start wifi access point
    sa_network_init();

    // Wifi is initialised before user app boot, so we can start comms immediately
    sa_comms_init();

    while (1)
    {
        // User code main loop

        // comms_cmd_t cmd;
        // cmd.cmd_code = CMD_PRINT_MESSAGE;
        // cmd.data_len = strlen(TEST_MSG) + 1;
        // cmd.data = malloc(cmd.data_len);
        // strcpy((char*)cmd.data, TEST_MSG);

        // sa_comms_broadcast(&cmd);

        vTaskDelay(10000);
    }
}