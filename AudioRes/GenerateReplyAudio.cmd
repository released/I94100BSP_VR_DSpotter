@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0ReplyPhrases\GeneratePhrases.ps1" %*
set "reply_audio_exit=%errorlevel%"
pause
exit /b %reply_audio_exit%
