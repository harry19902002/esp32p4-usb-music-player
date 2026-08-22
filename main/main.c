/* board_test - ESP32-P4 board-level test
 * Board: Waveshare ESP32-P4-WIFI6-DEV-KIT
 *
 * Features:
 *   1. Print chip info and ESP-IDF version at startup
 *   2. Toggle GPIO20 (P6-14, connect external LED to observe)
 *   3. Read onboard BOOT button (GPIO35), log state changes
 *   4. Configure PA enable GPIO53 (default off, reserved for audio test)
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_idf_version.h"
#include "driver/gpio.h"

static const char *TAG = "board_test";

/* Onboard resources (see pin definition doc) */
#define BOOT_BTN_GPIO     GPIO_NUM_35    /* BOOT button, pulled low when pressed */
#define PA_CTRL_GPIO      GPIO_NUM_53    /* PA enable, active high */
/* Test LED: no user LED on board, connect external LED between P6-14 (GPIO20) and GND */
#define TEST_LED_GPIO     GPIO_NUM_20

static void print_chip_info(void)
{
    esp_chip_info_t info;
    uint32_t flash_size = 0;
    esp_chip_info(&info);
    esp_flash_get_size(NULL, &flash_size);   /* use esp_flash_get_size for flash size */
    ESP_LOGI(TAG, "chip: %d cores, %u MB flash, features: 0x%x",
             info.cores,
             (unsigned int)flash_size / (1024 * 1024),
             (unsigned int)info.features);
    ESP_LOGI(TAG, "ESP-IDF version: %s", esp_get_idf_version());
}

void app_main(void)
{
    print_chip_info();

    /* Output pins: test LED + PA enable (default off) */
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << TEST_LED_GPIO) | (1ULL << PA_CTRL_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_cfg);
    gpio_set_level(TEST_LED_GPIO, 0);
    gpio_set_level(PA_CTRL_GPIO, 0);

    /* Input pin: BOOT button */
    gpio_config_t in_cfg = {
        .pin_bit_mask = 1ULL << BOOT_BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_cfg);

    ESP_LOGI(TAG, "test started: GPIO20 toggling, press BOOT to see log");
    int last_btn = gpio_get_level(BOOT_BTN_GPIO);

    while (1) {
        /* Blink */
        gpio_set_level(TEST_LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(TEST_LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(200));

        /* Report button state change */
        int btn = gpio_get_level(BOOT_BTN_GPIO);
        if (btn != last_btn) {
            ESP_LOGI(TAG, "BOOT button: %s", btn ? "released" : "pressed");
            last_btn = btn;
        }
    }
}
