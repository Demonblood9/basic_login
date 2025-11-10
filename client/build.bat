@echo off
REM Build script for Windows Secure Login Client
REM This script attempts to build using available compilers

echo Secure Login Client - Build Script
echo ===================================
echo.

REM Try MinGW first
where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found MinGW g++, building...
    g++ -std=c++11 -municode -mwindows -O2 -Wall main.cpp -o SecureLoginClient.exe -lwinhttp -lcomctl32 -lgdi32
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo Build successful! Output: SecureLoginClient.exe
        goto :success
    ) else (
        echo Build failed with MinGW
        goto :error
    )
)

REM Try MSVC
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found MSVC cl.exe, building...
    cl.exe /EHsc /Fe:SecureLoginClient.exe main.cpp /link winhttp.lib comctl32.lib gdi32.lib user32.lib
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo Build successful! Output: SecureLoginClient.exe
        goto :success
    ) else (
        echo Build failed with MSVC
        goto :error
    )
)

REM No compiler found
echo ERROR: No C++ compiler found!
echo.
echo Please install one of the following:
echo   - MinGW-w64: https://www.mingw-w64.org/
echo   - Visual Studio: https://visualstudio.microsoft.com/
echo.
goto :error

:success
echo.
echo You can now run: SecureLoginClient.exe
pause
exit /b 0

:error
echo.
pause
exit /b 1
