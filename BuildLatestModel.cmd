@echo off
setlocal
if not exist "%~dp0DSMT\AirconBundleBuilder.exe" (
    echo ERROR: DSMT\AirconBundleBuilder.exe is missing.
    echo Restore the supplied executable together with this project.
    pause
    exit /b 2
)
"%~dp0DSMT\AirconBundleBuilder.exe" --pause %*
exit /b %errorlevel%
