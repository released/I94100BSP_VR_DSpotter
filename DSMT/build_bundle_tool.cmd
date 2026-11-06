@echo off
setlocal
set "airconCompiler=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%airconCompiler%" set "airconCompiler=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
if not exist "%airconCompiler%" (
    echo ERROR: .NET Framework C# compiler not found. End users can use the supplied executable.
    exit /b 2
)
"%airconCompiler%" /nologo /target:exe /platform:anycpu /optimize+ /r:System.Web.Extensions.dll /r:System.IO.Compression.dll /out:"%~dp0AirconBundleBuilder.exe" "%~dp0AirconBundleBuilder.cs"
exit /b %errorlevel%
