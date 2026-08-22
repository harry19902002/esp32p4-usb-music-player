# 捕获 ESP32-P4 启动日志
# 流程: 打开 COM3 -> 通过 DTR/RTS 复位板子 -> 读取串口 10 秒
$port = New-Object System.IO.Ports.SerialPort("COM3", 115200, "None", 8, "One")
$port.ReadTimeout = 2000
$port.Open()

# 通过 DTR/RTS 触发复位 (类似 esptool 的 reset)
# ESP32-P4 使用 CH343P, DTR/RTS 控制自动下载电路 (EN + BOOT)
$port.DtrEnable = $false
$port.RtsEnable = $true
Start-Sleep -Milliseconds 100
$port.RtsEnable = $false
Start-Sleep -Milliseconds 100
$port.DtrEnable = $true
Start-Sleep -Milliseconds 200

# 持续读取
$sb = New-Object System.Text.StringBuilder
$deadline = (Get-Date).AddSeconds(10)
while ((Get-Date) -lt $deadline) {
    try {
        $line = $port.ReadLine()
        [void]$sb.AppendLine($line)
    } catch {
        # 超时继续读
    }
}
$port.Close()
$sb.ToString() | Out-File -FilePath "C:\Users\35310\WorkBuddy\esp32USBmusic\boot_log.txt" -Encoding utf8
"done"
