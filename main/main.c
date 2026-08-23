/* USB Host MSC - Read USB flash drive and list WAV files
 * Board: Waveshare ESP32-P4-WIFI6-DEV-KIT
 *
 * Features:
 *   1. USB Host Library init, MSC driver install
 *   2. When USB flash drive inserted, mount FATFS to /usb0
 *   3. List all .wav files in root directory
 *   4. BOOT button (GPIO35) toggles: list files again
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <inttypes.h>
#include <ctype.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "usb/msc_host_vfs.h"
#include "driver/gpio.h"

static const char *TAG = "usb_msc";

#define MNT_PATH         "/usb"     // Mount path prefix, devices mounted as /usb0, /usb1, ...
#define MAX_MSC_DEVICES  CONFIG_FATFS_VOLUME_COUNT

/* BOOT button (GPIO35), pulled low when pressed */
#define BOOT_BTN_GPIO    GPIO_NUM_35

/**
 * @brief MSC Device Entry
 */
typedef struct {
    uint8_t usb_addr;                     /*!< USB device address */
    msc_host_device_handle_t msc_device;  /*!< Handle of the MSC device */
    msc_host_vfs_handle_t vfs_handle;     /*!< VFS handle assigned to the MSC device */
} msc_dev_entry_t;

static msc_dev_entry_t *msc_devices[MAX_MSC_DEVICES] = {0};

/**
 * @brief Application Queue and its messages ID
 */
static QueueHandle_t app_queue;
typedef struct {
    enum {
        APP_DEVICE_CONNECTED,    // USB device connect event
        APP_DEVICE_DISCONNECTED, // USB device disconnect event
        APP_LIST_FILES,          // User pressed BOOT button, list files
    } id;
    union {
        uint8_t new_dev_address; // Address of new USB device for APP_DEVICE_CONNECTED event
        msc_host_device_handle_t device_handle; // Handle of removed USB device for APP_DEVICE_DISCONNECTED event
    } data;
} app_message_t;

/**
 * @brief Find a free slot in the device table.
 */
static inline int find_free_slot(void)
{
    for (int i = 0; i < MAX_MSC_DEVICES; i++) {
        if (msc_devices[i] == NULL) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Allocate a new MSC device and mount it to VFS.
 */
static esp_err_t allocate_new_msc_device(const app_message_t *msg, int *out_slot)
{
    int slot = find_free_slot();
    if (slot < 0) {
        ESP_LOGW(TAG, "No free slots for new MSC device (max %d)", MAX_MSC_DEVICES);
        return ESP_ERR_NOT_FOUND;
    }

    msc_devices[slot] = calloc(1, sizeof(msc_dev_entry_t));
    if (!msc_devices[slot]) {
        ESP_LOGE(TAG, "Failed to allocate memory for new MSC device entry");
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = msc_host_install_device(msg->data.new_dev_address, &msc_devices[slot]->msc_device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "msc_host_install_device failed: %s", esp_err_to_name(err));
        free(msc_devices[slot]);
        msc_devices[slot] = NULL;
        return err;
    }

    msc_devices[slot]->usb_addr = msg->data.new_dev_address;

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
        .allocation_unit_size = 8192,
    };

    char mount_path[16];
    snprintf(mount_path, sizeof(mount_path), MNT_PATH "%d", slot);

    err = msc_host_vfs_register(msc_devices[slot]->msc_device, mount_path, &mount_config, &msc_devices[slot]->vfs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "msc_host_vfs_register failed: %s (0x%X)", esp_err_to_name(err), err);
        ESP_ERROR_CHECK(msc_host_uninstall_device(msc_devices[slot]->msc_device));
        free(msc_devices[slot]);
        msc_devices[slot] = NULL;
        return err;
    }

    *out_slot = slot;
    return ESP_OK;
}

/**
 * @brief Find a slot by MSC device handle.
 */
static int find_slot_by_handle(msc_host_device_handle_t handle)
{
    for (int i = 0; i < MAX_MSC_DEVICES; i++) {
        if (msc_devices[i] && msc_devices[i]->msc_device == handle) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Free resources of a specific MSC device.
 */
static void free_msc_device(int slot)
{
    if (slot < 0 || slot >= MAX_MSC_DEVICES || !msc_devices[slot]) {
        ESP_LOGE(TAG, "Invalid slot index for MSC device deallocation");
        return;
    }

    if (msc_devices[slot]->vfs_handle) {
        ESP_ERROR_CHECK(msc_host_vfs_unregister(msc_devices[slot]->vfs_handle));
    }
    if (msc_devices[slot]->msc_device) {
        ESP_ERROR_CHECK(msc_host_uninstall_device(msc_devices[slot]->msc_device));
    }

    free(msc_devices[slot]);
    msc_devices[slot] = NULL;
}

/**
 * @brief Check if filename ends with .wav (case-insensitive)
 */
static bool is_wav_file(const char *name)
{
    size_t len = strlen(name);
    if (len < 4) {
        return false;
    }
    const char *ext = name + len - 4;
    return (tolower(ext[0]) == '.' && tolower(ext[1]) == 'w' &&
            tolower(ext[2]) == 'a' && tolower(ext[3]) == 'v');
}

/**
 * @brief Print info of connected MSC device.
 */
static void print_device_info(msc_host_device_info_t *info)
{
    const size_t megabyte = 1024 * 1024;
    uint64_t capacity = ((uint64_t)info->sector_size * info->sector_count) / megabyte;

    printf("Device info:\n");
    printf("\t Capacity: %llu MB\n", capacity);
    printf("\t Sector size: %"PRIu32"\n", info->sector_size);
    printf("\t Sector count: %"PRIu32"\n", info->sector_count);
    printf("\t PID: 0x%04X \n", info->idProduct);
    printf("\t VID: 0x%04X \n", info->idVendor);
}

/**
 * @brief List all files (especially .wav) in a mounted USB device.
 */
static void list_files(int slot)
{
    char mount_path[16];
    snprintf(mount_path, sizeof(mount_path), MNT_PATH "%d", slot);

    ESP_LOGI(TAG, "=== Listing contents of %s ===", mount_path);
    struct dirent *d;
    DIR *dh = opendir(mount_path);
    if (!dh) {
        ESP_LOGE(TAG, "Failed to open directory: %s", mount_path);
        return;
    }

    int wav_count = 0;
    int total_count = 0;
    while ((d = readdir(dh)) != NULL) {
        if (d->d_name[0] == '.') {
            continue;   // Skip hidden files and . / ..
        }
        total_count++;
        bool is_wav = is_wav_file(d->d_name);
        if (is_wav) {
            wav_count++;
            ESP_LOGI(TAG, "  [WAV] %s/%s", mount_path, d->d_name);
        } else {
            ESP_LOGI(TAG, "  [    ] %s/%s", mount_path, d->d_name);
        }
    }
    closedir(dh);

    ESP_LOGI(TAG, "=== %s: %d files total, %d WAV file(s) found ===", mount_path, total_count, wav_count);
    if (wav_count == 0) {
        ESP_LOGW(TAG, "No .wav files found in %s, please copy WAV files to USB drive root", mount_path);
    }
}

/**
 * @brief List files on all mounted devices.
 */
static void list_files_all_devices(void)
{
    for (int i = 0; i < MAX_MSC_DEVICES; i++) {
        if (msc_devices[i]) {
            list_files(i);
        }
    }
}

/**
 * @brief MSC driver callback.
 */
static void msc_event_cb(const msc_host_event_t *event, void *arg)
{
    if (event->event == MSC_DEVICE_CONNECTED) {
        ESP_LOGI(TAG, "MSC device connected (usb_addr=%d)", event->device.address);
        app_message_t message = {
            .id = APP_DEVICE_CONNECTED,
            .data.new_dev_address = event->device.address,
        };
        xQueueSend(app_queue, &message, portMAX_DELAY);
    } else if (event->event == MSC_DEVICE_DISCONNECTED) {
        ESP_LOGI(TAG, "MSC device disconnected");
        app_message_t message = {
            .id = APP_DEVICE_DISCONNECTED,
            .data.device_handle = event->device.handle,
        };
        xQueueSend(app_queue, &message, portMAX_DELAY);
    } else {
        ESP_LOGW(TAG, "Unsupported MSC event: %d", event->event);
    }
}

/**
 * @brief BOOT button ISR handler.
 */
static void gpio_cb(void *arg)
{
    BaseType_t xTaskWoken = pdFALSE;
    app_message_t message = {
        .id = APP_LIST_FILES,
    };
    if (app_queue) {
        xQueueSendFromISR(app_queue, &message, &xTaskWoken);
    }
    if (xTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief USB task: install USB Host Lib + MSC driver, handle events.
 */
static void usb_task(void *args)
{
    const usb_host_config_t host_config = { .intr_flags = ESP_INTR_FLAG_LOWMED };
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    const msc_host_driver_config_t msc_config = {
        .create_backround_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .callback = msc_event_cb,
    };
    ESP_ERROR_CHECK(msc_host_install(&msc_config));

    bool has_clients = true;
    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            has_clients = false;
            if (usb_host_device_free_all() == ESP_OK) {
                break;
            }
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE && !has_clients) {
            break;
        }
    }

    vTaskDelay(10);
    ESP_LOGI(TAG, "Deinitializing USB");
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

void app_main(void)
{
    // Create application queue
    app_queue = xQueueCreate(5, sizeof(app_message_t));
    assert(app_queue);

    // Create USB task
    BaseType_t task_created = xTaskCreate(usb_task, "usb_task", 4096, NULL, 2, NULL);
    assert(task_created);

    // BOOT button: press to list files
    const gpio_config_t input_pin = {
        .pin_bit_mask = BIT64(BOOT_BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&input_pin));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LOWMED));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOOT_BTN_GPIO, gpio_cb, NULL));

    ESP_LOGI(TAG, "Waiting for USB flash drive to be connected...");
    ESP_LOGI(TAG, "Insert USB drive into the HOST USB port");

    while (1) {
        app_message_t msg;
        xQueueReceive(app_queue, &msg, portMAX_DELAY);

        if (msg.id == APP_DEVICE_CONNECTED) {
            int slot;
            esp_err_t res = allocate_new_msc_device(&msg, &slot);
            if (res != ESP_OK) {
                continue;
            }
            // Print device info
            msc_host_device_info_t info;
            ESP_ERROR_CHECK(msc_host_get_device_info(msc_devices[slot]->msc_device, &info));
            print_device_info(&info);

            // List files on all mounted devices
            list_files_all_devices();
        }
        if (msg.id == APP_DEVICE_DISCONNECTED) {
            int slot = find_slot_by_handle(msg.data.device_handle);
            if (slot >= 0) {
                ESP_LOGI(TAG, "USB drive disconnected, unmounting %s%d", MNT_PATH, slot);
                free_msc_device(slot);
            }
        }
        if (msg.id == APP_LIST_FILES) {
            ESP_LOGI(TAG, "BOOT button pressed, listing files...");
            list_files_all_devices();
        }
    }
}
