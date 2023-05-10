#include "protected_main.h"

IRAM_ATTR void user_app_exception_handler(void *arg)
{
    // Perform actions when user app exception happens
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

    // Initialisation for protected comms
    sa_protected_comms_init();

    // Initialise network, start wifi access point
    sa_network_init();

    // Boot user app
    ret = esp_priv_access_user_boot();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_PROTECTED, "Failed to boot user app %d\n", ret);
    }

    // Main loop for protected app: additional kernel functions could go here in the future
    // app_main() must never return
    while (1)
    {
        vTaskDelay(1000);
    }
}
