#include "user_code.h"

void user_main()
{
    ESP_LOGI(TAG_USER, "Userspace start\n");

    while (1)
    {
        usr_test_syscall(NULL);
        vTaskDelay(1000);
    }
}
