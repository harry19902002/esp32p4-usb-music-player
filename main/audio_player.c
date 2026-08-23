/* audio_player - ES8311 codec audio playback via I2S
 * Board: Waveshare ESP32-P4-WIFI6-DEV-KIT
 *
 * Pin mapping (matches board schematic):
 *   I2C:  SDA=GPIO7, SCL=GPIO8   (ES8311 CDATA/CCLK)
 *   I2S:  MCLK=GPIO13, BCLK=GPIO12, WS=GPIO10, DOUT=GPIO9 (to codec DAC)
 *         DIN=GPIO11 (from codec ADC, unused for playback)
 *   PA:   GPIO53 (active high, NS4150B amplifier enable)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_codec_dev_defaults.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_vol.h"
#include "audio_player.h"

static const char *TAG = "audio";

/* ---- Board pin definitions (Waveshare ESP32-P4-WIFI6-DEV-KIT) ---- */
#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_IO      GPIO_NUM_7
#define I2C_SCL_IO      GPIO_NUM_8
#define I2S_PORT        I2S_NUM_0
#define I2S_MCLK_IO     GPIO_NUM_13
#define I2S_BCLK_IO     GPIO_NUM_12
#define I2S_WS_IO       GPIO_NUM_10
#define I2S_DOUT_IO     GPIO_NUM_9
#define I2S_DIN_IO      GPIO_NUM_11
#define PA_CTRL_IO      GPIO_NUM_53

#define DEFAULT_SAMPLE_RATE     16000   /* WAV 默认采样率, 会根据文件头调整 */
#define MCLK_MULTIPLE           256
#define VOLUME                  70      /* 0-100 */

static i2s_chan_handle_t tx_handle = NULL;
static esp_codec_dev_handle_t codec_handle = NULL;

/* WAV 文件头解析 */
typedef struct {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t data_size;     /* PCM 数据长度 */
    uint32_t data_offset;   /* data 块偏移 */
} wav_info_t;

static esp_err_t parse_wav_header(FILE *f, wav_info_t *info)
{
    uint8_t hdr[44];
    if (fread(hdr, 1, 44, f) != 44) {
        ESP_LOGE(TAG, "Failed to read WAV header");
        return ESP_FAIL;
    }

    /* RIFF 校验 */
    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "Not a valid WAV file (RIFF/WAVE signature)");
        return ESP_FAIL;
    }

    /* 标准 PCM: fmt 块在偏移 12 */
    if (memcmp(hdr + 12, "fmt ", 4) != 0) {
        ESP_LOGE(TAG, "Unsupported WAV (no fmt chunk at offset 12)");
        return ESP_FAIL;
    }
    uint16_t audio_format = *(uint16_t *)(hdr + 20);
    if (audio_format != 1) {
        ESP_LOGE(TAG, "Unsupported audio format: %d (only PCM supported)", audio_format);
        return ESP_FAIL;
    }

    info->channels = *(uint16_t *)(hdr + 22);
    info->sample_rate = *(uint32_t *)(hdr + 24);
    info->bits_per_sample = *(uint16_t *)(hdr + 34);

    /* 查找 data 块 (标准 44 字节头时在 36) */
    if (memcmp(hdr + 36, "data", 4) == 0) {
        info->data_offset = 44;
        info->data_size = *(uint32_t *)(hdr + 40);
    } else {
        /* 非标准头: 偏移 36 处可能是 LIST/其他块, 从 36 开始扫描块 */
        uint32_t pos = 36;
        uint8_t chunk[8];
        while (fseek(f, pos, SEEK_SET) == 0 && fread(chunk, 1, 8, f) == 8) {
            uint32_t chunk_size = *(uint32_t *)(chunk + 4);
            if (memcmp(chunk, "data", 4) == 0) {
                info->data_offset = pos + 8;
                info->data_size = chunk_size;
                break;
            }
            pos += 8 + chunk_size + (chunk_size & 1);   /* 块对齐 */
            if (pos > 1024 * 1024) {
                break;
            }
        }
        if (info->data_size == 0) {
            ESP_LOGE(TAG, "No data chunk found in WAV");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "WAV: %u Hz, %d ch, %d bit, data=%u bytes (offset %u)",
             info->sample_rate, info->channels, info->bits_per_sample,
             info->data_size, info->data_offset);
    return ESP_OK;
}

esp_err_t audio_player_init(void)
{
    /* 1. 功放使能 (GPIO53) - 默认关闭, 播放前开启 */
    gpio_config_t pa_cfg = {
        .pin_bit_mask = BIT64(PA_CTRL_IO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pa_cfg);
    gpio_set_level(PA_CTRL_IO, 0);
    ESP_LOGI(TAG, "PA control GPIO%d initialized (off)", PA_CTRL_IO);

    /* 2. I2S 通道初始化 (master, TX 输出到 codec) */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(DEFAULT_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_IO,
            .bclk = I2S_BCLK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DOUT_IO,
            .din = I2S_DIN_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    ESP_LOGI(TAG, "I2S initialized (MCLK=%d BCLK=%d WS=%d DOUT=%d)", I2S_MCLK_IO, I2S_BCLK_IO, I2S_WS_IO, I2S_DOUT_IO);

    /* 3. I2C 总线 + ES8311 codec */
    i2c_master_bus_handle_t i2c_bus_handle = NULL;
    i2c_master_bus_config_t i2c_mst_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_cfg, &i2c_bus_handle));

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = I2C_PORT,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus_handle,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(ctrl_if);

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_PORT,
        .tx_handle = tx_handle,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);
    assert(data_if);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    assert(gpio_if);

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .master_mode = false,           /* I2S 是 master, codec 是 slave */
        .use_mclk = true,               /* 使用板载 MCLK (GPIO13) */
        .pa_pin = PA_CTRL_IO,           /* 功放使能脚 */
        .pa_reverted = false,           /* 高电平使能 */
        .hw_gain = {
            .pa_voltage = 5.0,
            .codec_dac_voltage = 3.3,
        },
        .mclk_div = MCLK_MULTIPLE,
    };
    const audio_codec_if_t *es8311_if = es8311_codec_new(&es8311_cfg);
    assert(es8311_if);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es8311_if,
        .data_if = data_if,
    };
    codec_handle = esp_codec_dev_new(&dev_cfg);
    assert(codec_handle);

    /* 打开 codec 设备 (初始配置) */
    esp_codec_dev_sample_info_t sample_cfg = {
        .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
        .channel = 2,
        .channel_mask = 0x03,
        .sample_rate = DEFAULT_SAMPLE_RATE,
        .mclk_multiple = MCLK_MULTIPLE,
    };
    if (esp_codec_dev_open(codec_handle, &sample_cfg) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Open codec device failed");
        return ESP_FAIL;
    }
    if (esp_codec_dev_set_out_vol(codec_handle, VOLUME) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Set output volume failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "ES8311 codec initialized, volume=%d", VOLUME);
    return ESP_OK;
}

esp_err_t audio_player_play_wav(const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", file_path);
        return ESP_FAIL;
    }

    wav_info_t wav;
    if (parse_wav_header(f, &wav) != ESP_OK) {
        fclose(f);
        return ESP_FAIL;
    }

    /* 只支持 16-bit PCM */
    if (wav.bits_per_sample != 16) {
        ESP_LOGE(TAG, "Unsupported bits per sample: %d (only 16-bit)", wav.bits_per_sample);
        fclose(f);
        return ESP_FAIL;
    }

    /* 根据 WAV 头重新配置 codec 采样率/声道 */
    esp_codec_dev_sample_info_t sample_cfg = {
        .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
        .channel = wav.channels > 1 ? 2 : 1,
        .channel_mask = wav.channels > 1 ? 0x03 : 0x01,
        .sample_rate = wav.sample_rate,
        .mclk_multiple = MCLK_MULTIPLE,
    };
    if (esp_codec_dev_open(codec_handle, &sample_cfg) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to reconfigure codec for %u Hz", wav.sample_rate);
        fclose(f);
        return ESP_FAIL;
    }

    /* 开启功放 */
    gpio_set_level(PA_CTRL_IO, 1);
    ESP_LOGI(TAG, "PA enabled, playing %s (%u Hz, %u ch)", file_path, wav.sample_rate, wav.channels);

    /* 定位到数据区, 分块读取写入 I2S */
    fseek(f, wav.data_offset, SEEK_SET);
    uint8_t *buf = malloc(4096);
    if (!buf) {
        ESP_LOGE(TAG, "No memory for audio buffer");
        gpio_set_level(PA_CTRL_IO, 0);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    uint32_t remaining = wav.data_size;
    size_t bytes_read, bytes_written;
    while (remaining > 0) {
        size_t to_read = remaining > 4096 ? 4096 : remaining;
        bytes_read = fread(buf, 1, to_read, f);
        if (bytes_read == 0) {
            ESP_LOGW(TAG, "Unexpected EOF, played %u/%u bytes", wav.data_size - remaining, wav.data_size);
            break;
        }
        /* I2S 写入 (阻塞直到写完) */
        esp_err_t ret = i2s_channel_write(tx_handle, buf, bytes_read, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
            break;
        }
        remaining -= bytes_read;
    }

    free(buf);

    /* 播放完成, 关闭功放 */
    gpio_set_level(PA_CTRL_IO, 0);
    ESP_LOGI(TAG, "Playback finished: %s", file_path);
    fclose(f);
    return ESP_OK;
}
