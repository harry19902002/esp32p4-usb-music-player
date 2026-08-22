# 直接读取 COM3 原始字节
$port = New-Object System.IO.Ports.SerialPort("COM3", 115200, "None", 8, "One")
$port.ReadTimeout = 1000
$port.Open()

# 尝试通过 DTR/RTS 复位
$port.DtrEnable = $true
$port.RtsEnable = $true
Start-Sleep -Milliseconds 100
$port.RtsEnable = $false
Start-Sleep -Milliseconds 100
$port.DtrEnable = $false
Start-Sleep -Milliseconds 100
$port.DtrEnable = $true
Start-Sleep -Milliseconds 500

$bytes = New-Object System.Collections.Generic.List[byte]
$deadline = (Get-Date).AddSeconds(8)
while ((Get-Date) -lt $deadline) {
    try {
        $b = $port.ReadByte()
        if ($b -ge 0) { $bytes.Add($b) }
    } catch {
        Start-Sleep -Milliseconds 100
    }
}
$port.Close()
$hex = ($bytes | ForEach-Object { $_.ToString("X2") }) -join " "
$ascii = -join ($bytes | ForEach-Object { if ($_ -ge 32 -and $_ -le 126) { [char]$_ } else { "." } })
"BYTES=$($bytes.Count)" | Out-File -FilePath "C:\Users\35310\WorkBuddy\esp32USBmusic\boot_raw.txt" -Encoding utf8
"HEX: $hex" | Out-File -FilePath "C:\Users\35310\WorkBuddy\esp32USBmusic\boot_raw.txt" -Append -Encoding utf8
"ASCII: $ascii" | Out-File -FilePath "C:\Users\35310\WorkBuddy\esp32USBmusic\boot_raw.txt" -Append -Encoding utf8
"done"
