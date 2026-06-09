@echo off
REM ===========================================================================
REM  Pred Ai BMW - BeamMP Dedicated Server launcher (Windows)
REM  Double-click this file, OR run it from a terminal in this folder.
REM  Keep this .bat next to BeamMP-Server.exe and ServerConfig.toml.
REM ===========================================================================
title Pred Ai BMW - BeamMP Server
cd /d "%~dp0"

echo ===========================================================
echo   Pred Ai BMW - BeamMP Dedicated Server
echo   Folder: %cd%
echo ===========================================================
echo.

REM --- Locate the server binary (name can vary slightly by release) ---------
set "SERVER_EXE="
if exist "BeamMP-Server.exe"     set "SERVER_EXE=BeamMP-Server.exe"
if exist "Server.exe"            set "SERVER_EXE=Server.exe"
for %%f in (BeamMP-Server*.exe) do if not defined SERVER_EXE set "SERVER_EXE=%%f"

if not defined SERVER_EXE (
  echo [SETUP] No BeamMP server .exe found in this folder.
  echo         Download the dedicated SERVER from https://beammp.com/
  echo         ^("Host a Server" / Downloads -- NOT the game launcher^)
  echo         and place BeamMP-Server.exe right here next to this script.
  echo.
  pause
  exit /b 1
)

if not exist "ServerConfig.toml" (
  echo [SETUP] ServerConfig.toml not found.
  echo         The server will generate a fresh one on first launch.
  echo         After it does, paste your AuthKey into it and re-run.
  echo.
)

echo Launching %SERVER_EXE% ...
echo (Watch the console below for [ERROR] / [WARN] lines.)
echo.
"%SERVER_EXE%"

echo.
echo ===========================================================
echo  Server process exited. Scroll up and check for:
echo    [ERROR] = must fix (bad AuthKey, broken mod, port in use)
echo    [WARN]  = usually safe, but read it
echo ===========================================================
pause
