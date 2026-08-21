# ESP32-P4 U盘音乐播放器

基于 **ESP-IDF** 的 ESP32-P4 USB 音乐播放器（初始工程骨架）。

当前状态：最小可编译工程，后续将在此基础上实现 USB Host 读取 U 盘并播放音频。

## 硬件说明

- **ESP32-P4** 开发板（RISC-V 双核 400MHz，带 USB-OTG）
- U 盘（FAT32 格式）
- I2S DAC 模块（如 MAX98357A / PCM5102）+ 扬声器

> **注意**：ESP32-P4 没有传统 I2S 外设，音频需通过 **PDM 接口**（`i2s_std` 不可用），或使用外部 USB/SPI 音频方案。

## 构建与烧录

需要 **ESP-IDF v5.4+**（ESP32-P4 支持的最低版本）：

```bash
. $IDF_PATH/export.sh
idf.py set-target esp32p4
idf.py build
idf.py -p COM3 flash monitor
```

## 后续计划

- [ ] USB Host + Mass Storage 读取 U 盘
- [ ] FATFS 文件系统挂载
- [ ] WAV / MP3 解码播放
- [ ] PDM / 外部 DAC 音频输出
- [ ] 按键控制（上一曲 / 下一曲 / 播放暂停）
