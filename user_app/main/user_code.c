#include "user_code.h"

#define TEST_MSG "Hello!"

void user_main()
{
    ESP_LOGI(TAG_USER, "Userspace start");

    // Wifi is initialised before user app boot, so we can start comms immediately
    comms_start();

    while (1)
    {
        // User code main loop

        comms_cmd_t cmd;
        cmd.cmd_code = CMD_PRINT_MESSAGE;
        cmd.data_len = strlen(TEST_MSG) + 1;
        cmd.data = malloc(cmd.data_len);
        strcpy((char*)cmd.data, TEST_MSG);

        comms_broadcast(&cmd);

        vTaskDelay(10000);
    }
}