@echo off
setlocal

set PROTO_PATH=.
set OUT_PATH=..\..\HouseholdRPG\Assets\Scripts\Protocol

echo Game.proto 생성 중...
protoc.exe --proto_path=%PROTO_PATH% --csharp_out=%OUT_PATH% Protocol.proto
if errorlevel 1 ( echo [오류] Game.proto 실패 & pause & exit /b 1 )

echo.
echo 완료.
pause