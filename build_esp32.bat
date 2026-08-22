@echo off
REM ESP-IDF 构建辅助脚本（由 Git Bash 调用，规避 MSYS 路径转换）
call C:\Users\35310\esp\esp-idf\export.bat > NUL 2>&1
idf.py %*
