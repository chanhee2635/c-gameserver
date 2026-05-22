@echo off
echo [1/2] Generating protobuf files...
powershell -ExecutionPolicy Bypass -File "%~dp0GenProtocol.ps1"
if %errorlevel% neq 0 ( echo Failed. & pause & exit /b 1 )

echo [2/2] Generating packet handlers...
powershell -ExecutionPolicy Bypass -File "%~dp0GenPackets.ps1"
if %errorlevel% neq 0 ( echo Failed. & pause & exit /b 1 )

pause
