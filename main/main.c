#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-P4 U盘音乐播放器 - 工程初始化完成");
    ESP_LOGI(TAG, "TODO: USB Host + U盘读取 + 音频播放");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
