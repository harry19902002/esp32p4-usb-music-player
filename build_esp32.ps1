# ESP-IDF 构建辅助脚本（PowerShell 原生，避免 Git Bash 路径转换）
$ErrorActionPreference = "Continue"
$log = "C:\Users\35310\WorkBuddy\esp32USBmusic\build_log.txt"

# 设置环境
$env:IDF_PATH = "C:\Users\35310\esp\esp-idf"
$env:IDF_TOOLS_PATH = "C:\Users\35310\.espressif"
$env:PYTHONUTF8 = "1"
# 禁用 configdep (Windows 上会因文件锁定报 Permission denied)
$env:IDF_CONFIGDEP_ENABLE = "0"

# 加载 export.ps1
Push-Location $env:IDF_PATH
& "$env:IDF_PATH\export.ps1" | Out-Null
Pop-Location

# 执行传入的命令 (禁用 ccache + configdep, 串行编译降低 Defender 锁冲突)
$cmd = $args -join " "
Write-Output "执行: idf.py $cmd" | Out-File -FilePath $log -Encoding utf8
if ($cmd -match "build") {
    idf.py -j 1 --no-ccache -DCONFIGDEP_ENABLE=False $args 2>&1 | Out-File -FilePath $log -Append -Encoding utf8
} else {
    idf.py --no-ccache -DCONFIGDEP_ENABLE=False $args 2>&1 | Out-File -FilePath $log -Append -Encoding utf8
}
$code = $LASTEXITCODE
Write-Output "EXIT=$code" | Out-File -FilePath $log -Append -Encoding utf8
exit $code
