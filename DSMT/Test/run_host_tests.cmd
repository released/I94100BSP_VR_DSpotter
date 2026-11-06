@echo off
setlocal
pushd "%~dp0..\.."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto failed
if not exist "DSMT\Test\Build" mkdir "DSMT\Test\Build"
cl.exe /nologo /TC /W4 /WX /FI"%~dp0SDKTestTypes.h" /I"DNN_VR" /Fe:"DSMT\Test\Build\host_tests.exe" /FoDSMT\Test\Build\ DSMT\Test\test_aircon.c DNN_VR\AirconModels.c DNN_VR\AirconCommands.c DNN_VR\AirconVR.c DNN_VR\AirconFeedback.c
if errorlevel 1 goto failed
"DSMT\Test\Build\host_tests.exe" "CMDSet\ReleaseV005\AirconModels.bundle.bin"
if errorlevel 1 goto failed
cl.exe /nologo /TC /W4 /WX /D_CRT_SECURE_NO_WARNINGS /I"DNN_VR" /Fe:"DSMT\Test\Build\audio_tests.exe" /FoDSMT\Test\Build\ DSMT\Test\test_audio.c DNN_VR\AirconAudio.c DNN_VR\AirconModels.c
if errorlevel 1 goto failed
"DSMT\Test\Build\audio_tests.exe" "CMDSet\ReleaseV005\AirconReplies.bin" "CMDSet\ReleaseV005\AirconDataFlash.bin"
if errorlevel 1 goto failed
cl.exe /nologo /TC /W4 /WX /FI"%~dp0PlaybackTestSDK.h" /I"DNN_VR" /I"Src\app\inc" /Fe:"DSMT\Test\Build\playback_tests.exe" /FoDSMT\Test\Build\ DSMT\Test\test_playback.c Play\PlayNuOne.c
if errorlevel 1 goto failed
"DSMT\Test\Build\playback_tests.exe"
if errorlevel 1 goto failed
popd
exit /b 0
:failed
set "aircon_test_exit=%errorlevel%"
popd
exit /b %aircon_test_exit%
