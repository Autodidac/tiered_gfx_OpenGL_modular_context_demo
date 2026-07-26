@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0open_msvc.ps1"
exit /b %errorlevel%
