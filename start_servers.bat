@echo off
title Server Launcher
echo ==========================================================
echo   GameServer Load-Test  -  Server Launcher
echo ==========================================================
echo   (Make sure Redis :6379 and MySQL :3306 are running.)
echo.

set "REL=C:\Projects\Server\Binary\Release"
set "LOGIN=C:\Projects\Server\LoginWebServer\bin\Release\net8.0"

echo Cleaning up any previous instances...
taskkill /IM GameServer.exe     /F >nul 2>&1
taskkill /IM GateServer.exe     /F >nul 2>&1
taskkill /IM DummyClient.exe    /F >nul 2>&1
taskkill /IM LoginWebServer.exe /F >nul 2>&1
timeout /t 1 /nobreak >nul

echo [1/3] Starting LoginWebServer (port 5245) ...
start "LoginWebServer" /D "%LOGIN%" "%LOGIN%\LoginWebServer.exe"
timeout /t 7 /nobreak >nul

echo [2/3] Starting GateServer (port 6666) ...
start "GateServer" /D "%REL%" "%REL%\GateServer.exe"
timeout /t 3 /nobreak >nul

echo [3/3] Starting GameServer (port 7777) ...
start "GameServer" /D "%REL%" "%REL%\GameServer.exe"

echo.
echo ==========================================================
echo   Three server windows opened.
echo   Wait until the GameServer window shows:
echo       === [Server Ready to Accept Connections] ===
echo.
echo   Then run the load generator yourself (separate window):
echo       %REL%\DummyClient.exe
echo   and type:
echo       setup 3600      (wait ~30s for account creation)
echo       +600            (repeat 6 times, ~10s apart)
echo   The GameServer panel Active count climbs to ~3,300.
echo ==========================================================
echo.
echo   (You can CLOSE THIS window - the 3 servers keep running.)
echo   (To stop a server, close its own black window.)
pause
