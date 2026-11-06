@echo off
setlocal EnableExtensions DisableDelayedExpansion
rem Publish this working tree; model/audio generation remains a separate step.
set "GH_PUSH_REMOTE=https://github.com/released/I94100BSP_VR_DSpotter.git"
set "GH_PUSH_PAUSE=1"
set "GH_PUSH_ENTERED=0"
set "GH_PUSH_EXIT=1"
if /I "%~1"=="--no-pause" set "GH_PUSH_PAUSE=0"
if /I "%~1"=="--help" goto help
if not "%~1"=="" if /I not "%~1"=="--no-pause" goto usage_error
if not "%~2"=="" goto usage_error

pushd "%~dp0"
if errorlevel 1 goto finish
set "GH_PUSH_ENTERED=1"
"%SystemRoot%\System32\where.exe" git >nul 2>nul
if errorlevel 1 (
    echo ERROR: Git for Windows is required. Install Git and run this file again.
    goto finish
)
if not exist ".git" (
    echo ERROR: Run this batch from the project root containing .git.
    goto finish
)
rem FIND normalizes Git's LF output for FINDSTR's exact-line matching.
git symbolic-ref --quiet --short HEAD | "%SystemRoot%\System32\find.exe" /v "" | "%SystemRoot%\System32\findstr.exe" /l /x /c:"main" >nul
if errorlevel 1 (
    echo ERROR: The active branch must be main.
    goto finish
)
rem Require one exact push destination, including any Git URL rewrite rules.
set "GH_PUSH_URL_COUNT=0"
for /f %%N in ('git remote get-url --push --all origin 2^>nul ^| "%SystemRoot%\System32\find.exe" /v /c ""') do set "GH_PUSH_URL_COUNT=%%N"
if not "%GH_PUSH_URL_COUNT%"=="1" goto wrong_remote
git remote get-url --push --all origin 2>nul | "%SystemRoot%\System32\find.exe" /v "" | "%SystemRoot%\System32\findstr.exe" /l /x /c:"%GH_PUSH_REMOTE%" >nul
if errorlevel 1 goto wrong_remote
for %%R in (MERGE_HEAD CHERRY_PICK_HEAD REVERT_HEAD) do (
    git rev-parse --verify --quiet %%R >nul 2>nul
    if not errorlevel 1 (
        echo ERROR: Finish the existing Git merge, cherry-pick or revert first.
        goto finish
    )
)
git var GIT_AUTHOR_IDENT >nul 2>nul
if errorlevel 1 (
    echo ERROR: Configure git user.name and user.email. See GITHUB_UPLOAD.md.
    goto finish
)

echo Destination: %GH_PUSH_REMOTE%
echo [1/3] Adding project changes...
git add -A
if errorlevel 1 goto finish
rem An ignored file may already have been force-added; stop before committing it.
git ls-files --cached --ignored --exclude-standard | "%SystemRoot%\System32\findstr.exe" /r /c:"." >nul
if not errorlevel 1 (
    echo ERROR: Ignored files are already tracked or staged. Review the list below.
    git ls-files --cached --ignored --exclude-standard
    goto finish
)
git diff --cached --quiet --exit-code
set "GH_PUSH_DIFF=%errorlevel%"
if "%GH_PUSH_DIFF%"=="0" goto push
if not "%GH_PUSH_DIFF%"=="1" goto finish
echo [2/3] Committing changes...
git --no-pager diff --cached --stat
if errorlevel 1 goto finish
git commit -m "Update bilingual voice project"
if errorlevel 1 goto finish

:push
echo [3/3] Pushing main to GitHub...
git push -u origin main
if errorlevel 1 (
    echo ERROR: Push failed. Local commits are retained; resolve the Git error and retry.
    goto finish
)
echo SUCCESS: main has been pushed.
set "GH_PUSH_EXIT=0"
goto finish

:wrong_remote
echo ERROR: origin must have exactly one push URL:
echo %GH_PUSH_REMOTE%
goto finish

:help
echo github_push.bat [--no-pause]
echo Adds all non-ignored changes, commits if needed, then pushes origin main.
echo Commit message: Update bilingual voice project
exit /b 0

:usage_error
echo Usage: github_push.bat [--no-pause]
exit /b 2

:finish
if "%GH_PUSH_ENTERED%"=="1" popd
if not "%GH_PUSH_EXIT%"=="0" echo FAILED: Upload did not complete. Review the Git error above.
if "%GH_PUSH_PAUSE%"=="1" pause
exit /b %GH_PUSH_EXIT%
