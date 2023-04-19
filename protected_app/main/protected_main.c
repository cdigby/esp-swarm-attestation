#include "protected_main.h"

IRAM_ATTR void user_app_exception_handler(void *arg)
{
    // Perform actions when user app exception happens
    // ESP_LOGE(TAG_PROTECTED, "USER APP EXCEPTION OCCURRED\n");
}

// Protected app entry point
void app_main()
{
    esp_err_t ret;

    // Initialise protected environment
    ret = esp_priv_access_init(user_app_exception_handler);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_PROTECTED, "Failed to initialize protected environment %d\n", ret);
    }

    // Initialise fake app memory for attestation algorithms
    init_fake_app_memory();

    // Boot user app
    ret = esp_priv_access_user_boot();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_PROTECTED, "Failed to boot user app %d\n", ret);
    }

    // Main loop for protected app
    while (1)
    {
        vTaskDelay(1000);
    }
}
