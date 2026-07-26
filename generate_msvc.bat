@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0generate_msvc.ps1"
exit /b %errorlevel%
