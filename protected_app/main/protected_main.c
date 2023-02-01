#include "protected_main.h"

IRAM_ATTR void user_app_exception_handler(void *arg)
{
    // Perform actions when user app exception happens
    ESP_LOGE(TAG_KERNEL, "USER APP EXCEPTION OCCURRED\n");
}

int sys_test_syscall(void *args)
{
    ESP_LOGI(TAG_KERNEL, "THIS IS A SYSCALL!\n");
    return 0;
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

    // Boot user app
    ret = esp_priv_access_user_boot();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_KERNEL, "Failed to boot user app %d\n", ret);
    }

    // Main kernel loop
    while (1)
    {
        ESP_LOGI(TAG_KERNEL, "kernel test\n");
        vTaskDelay(10);
    }
}
