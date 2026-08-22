# ESP-IDF 构建辅助脚本（PowerShell 原生，避免 Git Bash 路径转换）
$ErrorActionPreference = "Continue"
$log = "C:\Users\35310\WorkBuddy\esp32USBmusic\build_log.txt"

# 设置环境
$env:IDF_PATH = "C:\Users\35310\esp\esp-idf"
$env:IDF_TOOLS_PATH = "C:\Users\35310\.espressif"
$env:PYTHONUTF8 = "1"

# 加载 export.ps1
Push-Location $env:IDF_PATH
& "$env:IDF_PATH\export.ps1" | Out-Null
Pop-Location

# 执行传入的命令
$cmd = $args -join " "
Write-Output "执行: idf.py $cmd" | Out-File -FilePath $log -Encoding utf8
idf.py $args 2>&1 | Out-File -FilePath $log -Append -Encoding utf8
$code = $LASTEXITCODE
Write-Output "EXIT=$code" | Out-File -FilePath $log -Append -Encoding utf8
exit $code
