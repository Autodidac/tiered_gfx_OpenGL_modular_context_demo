@echo off
setlocal
set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Release
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_msvc.ps1" -Configuration "%CONFIG%"
exit /b %errorlevel%
