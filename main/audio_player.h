#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 I2S 和 ES8311 codec (含功放使能 GPIO53)
 * @return ESP_OK 成功
 */
esp_err_t audio_player_init(void);

/**
 * @brief 播放一个 WAV 文件 (从 U 盘文件系统)
 * @param file_path 完整路径, 如 "/usb0/song.wav"
 * @note 阻塞直到播放完成
 */
esp_err_t audio_player_play_wav(const char *file_path);

#ifdef __cplusplus
}
#endif
