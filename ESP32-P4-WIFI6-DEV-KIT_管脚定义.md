# ESP32-P4-WIFI6-DEV-KIT 管脚定义

本文档依据 Waveshare `ESP32-P4-WIFI6-DEV-KIT` 原理图整理，主控为 **ESP32-P4NRW32X**，无线协处理器为 **ESP32-C6FH8**。

- 原理图来源：[ESP32-P4-WIFI6-DEV-KIT.pdf](https://files.waveshare.net/wiki/ESP32-P4-WIFI6-DEV-KIT/ESP32-P4-WIFI6-DEV-KIT.pdf)
- 整理日期：2026-08-20
- 电平：除特别说明外，ESP32-P4 与 ESP32-C6 的 GPIO 均为 **3.3 V 逻辑**，不可直接输入 5 V。
- 命名方向：TX/RX、输入/输出均以对应 ESP32 芯片为参照。

> 注意：本文记录的是开发板上的实际连线和板级复用关系，不替代 ESP32-P4 数据手册中的上电、驱动能力、GPIO Matrix、内部上下拉和复位状态说明。

## 1. 功能总览

| 功能 | ESP32-P4 管脚 | 说明 |
|---|---|---|
| 板载 ESP32-C6（Wi-Fi 6） | GPIO14-GPIO19、GPIO6、GPIO54 | SDIO 数据/时钟/命令，以及 C6 下载与使能控制 |
| 以太网 RMII | GPIO28-GPIO31、GPIO34、GPIO35、GPIO49-GPIO52 | 板载 PHY 与 RJ45 |
| MicroSD | GPIO39-GPIO45 | 4-bit SD 总线及卡电源控制 |
| 音频 Codec ES8311 | GPIO7-GPIO13 | I2C + I2S |
| 功放控制 | GPIO53 | 高电平工作，低电平关闭；耳机插入后硬件会强制关闭功放 |
| 调试串口 | GPIO37、GPIO38 | 板载 CH343P USB 转串口 |
| BOOT | GPIO35 | 按键按下接地，同时参与自动下载电路；还与以太网 TXD1 共用 |
| RESET | ESP_EN | 低电平复位，不是普通 GPIO |
| USB 1.1 | GPIO24、GPIO25 | 分别为 USB1P1_N、USB1P1_P，同时重复引到 P6 |
| USB 2.0 PHY | 专用 USB_DM、USB_DP | 原理图网络名 USBD_N、USBD_P，经 USB 选择开关接板载 USB 系统 |
| MIPI CSI | 专用 CSI D-PHY 管脚 | 2-lane，相机接口 J4 |
| MIPI DSI | 专用 DSI D-PHY 管脚 | 2-lane，显示接口 J3 |
| 32.768 kHz RTC 晶振 | GPIO0、GPIO1 | 原理图中已接 Y2，不建议作普通 GPIO |
| 外部 SPI Flash | 专用 FLASH_* 管脚 | 板载 GD25Q128ESIG，不可作为普通 GPIO |

## 2. ESP32-P4 GPIO 完整占用表

“空闲/引出”表示原理图未连接板载功能，但仍应结合 ESP32-P4 数据手册确认启动和电气限制。

| GPIO | 开发板实际连接 | 状态/建议 |
|---:|---|---|
| 0 | 32.768 kHz 晶振 Y2 一端；P6-22 | RTC 已占用，不建议作 GPIO |
| 1 | 32.768 kHz 晶振 Y2 另一端；P6-19 | RTC 已占用，不建议作 GPIO |
| 2 | P6-20 | 空闲/引出 |
| 3 | P6-18 | 空闲/引出 |
| 4 | P6-15 | 空闲/引出 |
| 5 | P6-13 | 空闲/引出 |
| 6 | ESP32-C6 的 GPIO2（C6 下载/启动控制）；P6-16 | 板载功能占用，谨慎复用 |
| 7 | ESP_I2C_SDA；P6-4 | 板载 Codec、CSI、DSI、J10 共用 I2C |
| 8 | ESP_I2C_SCL；P6-6 | 板载 Codec、CSI、DSI、J10 共用 I2C |
| 9 | ES8311 DSDIN | 音频 I2S 占用 |
| 10 | ES8311 LRCK | 音频 I2S 占用 |
| 11 | ES8311 ASDOUT | 音频 I2S 占用 |
| 12 | ES8311 SCLK/BCLK | 音频 I2S 占用 |
| 13 | ES8311 MCLK | 音频 I2S 占用 |
| 14 | ESP32-C6 SDIO DATA0 | Wi-Fi 占用 |
| 15 | ESP32-C6 SDIO DATA1 | Wi-Fi 占用 |
| 16 | ESP32-C6 SDIO DATA2 | Wi-Fi 占用 |
| 17 | ESP32-C6 SDIO DATA3 | Wi-Fi 占用 |
| 18 | ESP32-C6 SDIO CLK | Wi-Fi 占用 |
| 19 | ESP32-C6 SDIO CMD | Wi-Fi 占用 |
| 20 | P6-14 | 空闲/引出 |
| 21 | P6-12 | 空闲/引出 |
| 22 | P6-9 | 空闲/引出 |
| 23 | P6-8 | 空闲/引出 |
| 24 | USB1P1_N；P6-24，同时经 0 Ω 电阻引到 P6-28 | USB 共享，避免外接重负载 |
| 25 | USB1P1_P；P6-25，同时经 0 Ω 电阻引到 P6-27 | USB 共享，避免外接重负载 |
| 26 | P6-32 | 空闲/引出 |
| 27 | P6-35 | 空闲/引出 |
| 28 | Ethernet CRS_DV/RXDV | 以太网占用 |
| 29 | Ethernet RXD0 | 以太网占用 |
| 30 | Ethernet RXD1 | 以太网占用 |
| 31 | Ethernet MDC | 以太网占用 |
| 32 | I3C 接口 J9-4；P6-23 | 共享引出，可在不用 J9 时复用 |
| 33 | I3C 接口 J9-3；P6-30 | 共享引出，可在不用 J9 时复用 |
| 34 | Ethernet TXD0 | 以太网占用 |
| 35 | Ethernet TXD1、BOOT 按键、自动下载电路 | 启动关键管脚，不建议复用 |
| 36 | P6-21 | 空闲/引出 |
| 37 | 调试 UART TX；P6-5 | 板载 CH343P 的 RXD 接收此信号 |
| 38 | 调试 UART RX；P6-7 | 板载 CH343P 的 TXD 驱动此信号 |
| 39 | MicroSD D0 | MicroSD 占用 |
| 40 | MicroSD D1 | MicroSD 占用 |
| 41 | MicroSD D2 | MicroSD 占用 |
| 42 | MicroSD D3/CD | MicroSD 占用 |
| 43 | MicroSD CLK | MicroSD 占用 |
| 44 | MicroSD CMD | MicroSD 占用 |
| 45 | MicroSD 电源开关控制；P6-37 | 低电平使能 SD 卡 3.3 V 电源，谨慎复用 |
| 46 | P6-33 | 空闲/引出 |
| 47 | P6-38 | 空闲/引出 |
| 48 | P6-34 | 空闲/引出 |
| 49 | Ethernet TXEN | 以太网占用 |
| 50 | Ethernet 50 MHz 参考时钟 | 以太网占用 |
| 51 | Ethernet PHY RESET | 以太网占用 |
| 52 | Ethernet MDIO | 以太网占用 |
| 53 | 音频功放 PA_CTRL；P6-36 | 高电平工作、低电平关断 |
| 54 | ESP32-C6 CHIP_PU；P6-29 | C6 使能/复位控制，谨慎复用 |

## 3. P6 40-pin 扩展排针

P6 为 2×20 排针。下表按原理图针号排列；`NC` 表示原理图未连接。

| 偶数针 | 信号 | 奇数针 | 信号 |
|---:|---|---:|---|
| 2 | ESP_3V3 | 1 | VCC_5V |
| 4 | GPIO7 / ESP_I2C_SDA | 3 | GND |
| 6 | GPIO8 / ESP_I2C_SCL | 5 | GPIO37 / DEBUG_UART_TX |
| 8 | GPIO23 | 7 | GPIO38 / DEBUG_UART_RX |
| 10 | GND | 9 | GPIO22 |
| 12 | GPIO21 | 11 | GND |
| 14 | GPIO20 | 13 | GPIO5 |
| 16 | GPIO6 / C6_IO2 | 15 | GPIO4 |
| 18 | GPIO3 | 17 | GND |
| 20 | GPIO2 | 19 | GPIO1 / RTC_XTAL |
| 22 | GPIO0 / RTC_XTAL | 21 | GPIO36 |
| 24 | GPIO24 / USB1P1_N | 23 | GPIO32 / I3C |
| 26 | GND | 25 | GPIO25 / USB1P1_P |
| 28 | USB1P1_N（经 0 Ω） | 27 | USB1P1_P（经 0 Ω） |
| 30 | GPIO33 / I3C | 29 | GPIO54 / C6_CHIP_PU |
| 32 | GPIO26 | 31 | GND |
| 34 | GPIO48 | 33 | GPIO46 |
| 36 | GPIO53 / PA_CTRL | 35 | GPIO27 |
| 38 | GPIO47 | 37 | GPIO45 / SD_PWR_EN_N |
| 40 | GND | 39 | NC |

### 默认板载功能全部启用时，优先选择的扩展 GPIO

`GPIO2、GPIO3、GPIO4、GPIO5、GPIO20、GPIO21、GPIO22、GPIO23、GPIO26、GPIO27、GPIO36、GPIO46、GPIO47、GPIO48`

GPIO32/GPIO33 也可以使用，但会与 J9 I3C 接口共享。GPIO37/GPIO38 会与调试串口共享。GPIO24/GPIO25、GPIO6、GPIO45、GPIO53、GPIO54 均与板载硬件存在明确复用关系。

## 4. ESP32-C6 与 ESP32-P4 连接

ESP32-C6 用作无线协处理器。ESP32-P4 通过 4-bit SDIO 与其通信，并控制 C6 的下载启动和芯片使能。

| ESP32-P4 | ESP32-C6 | 用途 |
|---:|---|---|
| GPIO14 | SDIO_DATA0 | SDIO 数据 0 |
| GPIO15 | SDIO_DATA1 | SDIO 数据 1 |
| GPIO16 | SDIO_DATA2 | SDIO 数据 2 |
| GPIO17 | SDIO_DATA3 | SDIO 数据 3 |
| GPIO18 | SDIO_CLK | SDIO 时钟 |
| GPIO19 | SDIO_CMD | SDIO 命令 |
| GPIO6 | GPIO2 | C6 启动/下载模式控制 |
| GPIO54 | CHIP_PU | C6 使能/复位，低电平复位 |

### P2：ESP32-C6 调试排针

| P2 针号 | 信号 |
|---:|---|
| 1 | C6_U0TXD |
| 2 | C6_U0RXD |
| 3 | GND |
| 4 | C6_GPIO9 |

## 5. MicroSD

| ESP32-P4 | SD 卡信号 | SD 卡座针号 |
|---:|---|---:|
| GPIO41 | D2 | 1 |
| GPIO42 | D3/CD | 2 |
| GPIO44 | CMD | 3 |
| GPIO43 | CLK | 5 |
| GPIO39 | D0 | 7 |
| GPIO40 | D1 | 8 |
| GPIO45 | SD1_VDD 开关控制 | - |

GPIO45 控制 P 沟道 MOSFET Q1：**GPIO45 为低电平时 SD1_VDD 上电**。数据线带 51 kΩ 上拉/下拉配置电阻，复用这些管脚可能影响 SD 卡启动和通信。

## 6. 音频接口

### ES8311 Codec

| ESP32-P4 | ES8311 | 用途 |
|---:|---|---|
| GPIO7 | CDATA | I2C SDA |
| GPIO8 | CCLK | I2C SCL |
| GPIO13 | MCLK | I2S 主时钟 |
| GPIO12 | SCLK/DMIC_SCL | I2S 位时钟 BCLK |
| GPIO11 | ASDOUT | Codec ADC 输出到 ESP32-P4 |
| GPIO10 | LRCK | I2S 左右声道时钟 WS/LRCK |
| GPIO9 | DSDIN | ESP32-P4 输出到 Codec DAC |

### 功放与耳机

| ESP32-P4 | 信号 | 有效电平 |
|---:|---|---|
| GPIO53 | PA_CTRL | 高电平开启，低电平关断 |

耳机插座的 `EARPHONE_DETECT` 通过硬件与门控制功放：插入耳机后，无论 GPIO53 状态如何，功放都会被关闭。板载麦克风为模拟麦克风，直接接 ES8311 的 MIC1P/MIC1N，不占用额外 GPIO。

## 7. 以太网 RMII

| ESP32-P4 | PHY 信号 | 用途 |
|---:|---|---|
| GPIO49 | TXEN | 发送使能 |
| GPIO35 | TXD1 | 发送数据 1；同时共享 BOOT |
| GPIO34 | TXD0 | 发送数据 0 |
| GPIO50 | 50M_CLK | RMII 50 MHz 参考时钟 |
| GPIO28 | CRS_DV/RXDV | 接收数据有效 |
| GPIO29 | RXD0 | 接收数据 0 |
| GPIO30 | RXD1 | 接收数据 1 |
| GPIO31 | MDC | PHY 管理时钟 |
| GPIO52 | MDIO | PHY 管理数据 |
| GPIO51 | RESET | PHY 复位 |

原理图中的 `PHY_AD0`、`PHY_AD3`、`PHY_TESTON`、`TXER`、`COL`、`RXER` 等还连接了 PHY 启动配置电阻，软件初始化以板载 PHY 的实际配置为准。

## 8. USB、下载串口和按键

### 调试 UART

| ESP32-P4 | 方向 | 板载 CH343P |
|---:|---|---|
| GPIO37 | TX | CH343P RXD |
| GPIO38 | RX | CH343P TXD |

CH343P 的 DTR/RTS 通过晶体管自动控制 `ESP_EN` 和 `GPIO35`，用于自动进入下载模式。

### USB

| ESP32-P4 端 | 原理图网络 | 说明 |
|---|---|---|
| GPIO24 | USB1P1_N | USB 1.1 D-，同时引到 P6-24/P6-28 |
| GPIO25 | USB1P1_P | USB 1.1 D+，同时引到 P6-25/P6-27 |
| USB_DM（专用） | USBD_N | USB 2.0 D- |
| USB_DP（专用） | USBD_P | USB 2.0 D+ |

`USBD_N/P` 经过 FSUSB42 USB 选择开关，选择信号为 `USB_SEL`：原理图标注 `H -> 通道 2，L -> 通道 1`。

### 按键

| 按键 | 信号 | 按下状态 |
|---|---|---|
| BOOT / Key2 | GPIO35 | 接地，低电平 |
| RESET / Key1 | ESP_EN | 接地，低电平复位 |

## 9. MIPI 与低速扩展接口

### J4：MIPI CSI

| 针号 | 信号 |
|---:|---|
| 1 | CSI_D0_N |
| 2 | CSI_D0_P |
| 3 | GND |
| 4 | CSI_D1_N |
| 5 | CSI_D1_P |
| 6 | GND |
| 7 | CSI_CLK_N |
| 8 | CSI_CLK_P |
| 9 | GND |
| 10 | CSI_IO0（10 kΩ 上拉） |
| 11 | CSI_IO1（默认无上拉，R55 NC） |
| 12 | GPIO8 / ESP_I2C_SCL |
| 13 | GPIO7 / ESP_I2C_SDA |
| 14 | GND |
| 15 | ESP_3V3 |

`CSI_IO0` 和 `CSI_IO1` 在原理图中没有继续连接到 ESP32-P4 GPIO，仅作为接口辅助线并配置了可选电阻。

### J3：MIPI DSI

| 针号 | 信号 |
|---:|---|
| 1 | DSI_D1_N |
| 2 | DSI_D1_P |
| 3 | GND |
| 4 | DSI_CLK_N |
| 5 | DSI_CLK_P |
| 6 | GND |
| 7 | DSI_D0_N |
| 8 | DSI_D0_P |
| 9 | GND |
| 10 | GPIO8 / ESP_I2C_SCL |
| 11 | GPIO7 / ESP_I2C_SDA |
| 12 | GND |
| 13 | GND |
| 14 | GND |
| 15 | ESP_3V3 |

### J9：I3C 扩展

| 针号 | 信号 |
|---:|---|
| 1 | GND |
| 2 | ESP_3V3 |
| 3 | GPIO33，2.2 kΩ 上拉 |
| 4 | GPIO32，2.2 kΩ 上拉 |

### J10：I2C 扩展

| 针号 | 信号 |
|---:|---|
| 1 | GND |
| 2 | ESP_3V3 |
| 3 | GPIO7 / ESP_I2C_SDA |
| 4 | GPIO8 / ESP_I2C_SCL |

## 10. ESP-IDF 管脚宏参考

下面的宏与本原理图连线一致，可作为 BSP 头文件的起点。是否启用某个功能，应由工程配置决定。

```c
#pragma once

#include "driver/gpio.h"

/* Shared I2C */
#define BOARD_I2C_SDA_GPIO          GPIO_NUM_7
#define BOARD_I2C_SCL_GPIO          GPIO_NUM_8

/* ES8311 / I2S */
#define BOARD_I2S_DOUT_GPIO         GPIO_NUM_9
#define BOARD_I2S_LRCK_GPIO         GPIO_NUM_10
#define BOARD_I2S_DIN_GPIO          GPIO_NUM_11
#define BOARD_I2S_BCLK_GPIO         GPIO_NUM_12
#define BOARD_I2S_MCLK_GPIO         GPIO_NUM_13
#define BOARD_PA_CTRL_GPIO          GPIO_NUM_53

/* ESP32-C6 SDIO */
#define BOARD_C6_SDIO_D0_GPIO       GPIO_NUM_14
#define BOARD_C6_SDIO_D1_GPIO       GPIO_NUM_15
#define BOARD_C6_SDIO_D2_GPIO       GPIO_NUM_16
#define BOARD_C6_SDIO_D3_GPIO       GPIO_NUM_17
#define BOARD_C6_SDIO_CLK_GPIO      GPIO_NUM_18
#define BOARD_C6_SDIO_CMD_GPIO      GPIO_NUM_19
#define BOARD_C6_BOOT_GPIO          GPIO_NUM_6
#define BOARD_C6_CHIP_PU_GPIO       GPIO_NUM_54

/* MicroSD */
#define BOARD_SD_D0_GPIO            GPIO_NUM_39
#define BOARD_SD_D1_GPIO            GPIO_NUM_40
#define BOARD_SD_D2_GPIO            GPIO_NUM_41
#define BOARD_SD_D3_GPIO            GPIO_NUM_42
#define BOARD_SD_CLK_GPIO           GPIO_NUM_43
#define BOARD_SD_CMD_GPIO           GPIO_NUM_44
#define BOARD_SD_POWER_EN_N_GPIO    GPIO_NUM_45

/* Ethernet RMII */
#define BOARD_ETH_CRS_DV_GPIO       GPIO_NUM_28
#define BOARD_ETH_RXD0_GPIO         GPIO_NUM_29
#define BOARD_ETH_RXD1_GPIO         GPIO_NUM_30
#define BOARD_ETH_MDC_GPIO          GPIO_NUM_31
#define BOARD_ETH_TXD0_GPIO         GPIO_NUM_34
#define BOARD_ETH_TXD1_GPIO         GPIO_NUM_35
#define BOARD_ETH_TX_EN_GPIO        GPIO_NUM_49
#define BOARD_ETH_REF_CLK_GPIO      GPIO_NUM_50
#define BOARD_ETH_RESET_GPIO        GPIO_NUM_51
#define BOARD_ETH_MDIO_GPIO         GPIO_NUM_52

/* Debug UART and buttons */
#define BOARD_DEBUG_UART_TX_GPIO    GPIO_NUM_37
#define BOARD_DEBUG_UART_RX_GPIO    GPIO_NUM_38
#define BOARD_BOOT_BUTTON_GPIO      GPIO_NUM_35
#define BOARD_RESET_SIGNAL          "ESP_EN"

/* USB 1.1 GPIO pair */
#define BOARD_USB1P1_N_GPIO         GPIO_NUM_24
#define BOARD_USB1P1_P_GPIO         GPIO_NUM_25

/* I3C header */
#define BOARD_I3C_LINE0_GPIO        GPIO_NUM_33
#define BOARD_I3C_LINE1_GPIO        GPIO_NUM_32
```

## 11. 复用注意事项

1. **不要把 GPIO14-GPIO19 当作普通扩展口使用**，否则会破坏 ESP32-C6 的 SDIO 通信，导致 Wi-Fi 不可用。
2. **GPIO35 同时承担 BOOT、自动下载和以太网 TXD1**。外部电路改变其上电电平可能导致无法启动或下载。
3. **GPIO7/GPIO8 是全板共享 I2C**。新增设备时应确认地址不冲突，并计入板载 2.2 kΩ 上拉。
4. **GPIO24/GPIO25 在 P6 上重复出现两次**，不是四个独立信号；连接外设可能影响 USB。
5. **GPIO45 为低有效的 SD 卡电源控制**。错误驱动会让 SD 卡掉电。
6. **GPIO53 控制扬声器功放**。需要降低爆音时，应先静音 Codec，再拉低 PA_CTRL。
7. **GPIO54 控制 ESP32-C6 的 CHIP_PU**。拉低会复位并关闭无线协处理器。
8. 使用 GPIO37/GPIO38 连接外部串口设备时，要考虑板载 CH343P 仍然并联在信号线上。
9. MIPI CSI/DSI、USB 2.0 和外部 Flash 使用专用管脚，不能按普通 GPIO 的方式重新分配。

