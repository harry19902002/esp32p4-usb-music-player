# 捕获 ESP32-P4 完整启动日志 (使用标准 esptool 复位时序)
$port = New-Object System.IO.Ports.SerialPort("COM3", 115200, "None", 8, "One")
$port.ReadTimeout = 1000
$port.Open()

# 标准复位时序:
# DTR=EN 控制, RTS=BOOT 控制 (CH343P 经晶体管)
# 1) EN=0, BOOT=0 -> 进入下载模式 (复位)
# 2) EN=1 -> 释放复位
# 3) BOOT=1 -> 正常运行
$port.DtrEnable = $true   # EN low
$port.RtsEnable = $true   # BOOT low
Start-Sleep -Milliseconds 100
$port.DtrEnable = $false  # EN high (释放复位, 开始启动)
Start-Sleep -Milliseconds 100
$port.RtsEnable = $false  # BOOT high (正常运行模式)
Start-Sleep -Milliseconds 200

# 读取启动日志
$sb = New-Object System.Text.StringBuilder
$deadline = (Get-Date).AddSeconds(10)
while ((Get-Date) -lt $deadline) {
    try {
        $line = $port.ReadLine()
        [void]$sb.AppendLine($line)
    } catch {
        Start-Sleep -Milliseconds 50
    }
}
$port.Close()
$sb.ToString() | Out-File -FilePath "C:\Users\35310\WorkBuddy\esp32USBmusic\boot_full.txt" -Encoding utf8
"done"
