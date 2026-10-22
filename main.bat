@echo off

setlocal

chcp 65001>nul
cd /d "%~dp0"

g++ -std=gnu++2c "main.cpp" -o "%TEMP%\vscode-cpp.exe" -latomic
"%TEMP%\vscode-cpp.exe"
del "%TEMP%\vscode-cpp.exe"

endlocal
