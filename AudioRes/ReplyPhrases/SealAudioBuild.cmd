@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0SealAudioBuild.ps1"
set "audio_seal_exit=%errorlevel%"
pause
exit /b %audio_seal_exit%
