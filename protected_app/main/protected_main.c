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

    // Initialise fake app memory
    init_fake_app_memory();

    uint8_t k[16];
    for (int i = 0; i < 16; i++)
    {
        k[i] = i;
    }

    uint8_t result[32];
    memset(result, 0, 32);
    compute_software_state(k, 16, result);

    printf("computed hmac!\n");
    for (int i = 0; i < 32; i++)
    {
        printf("%d ", result[i]);
    }
    printf("\n");


    // Initialise protected environment
    ret = esp_priv_access_init(user_app_exception_handler);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_PROTECTED, "Failed to initialize protected environment %d\n", ret);
    }

    // esp_priv_access_set_periph_perm(PA_GPIO, PA_WORLD_1, PA_PERM_ALL);


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
