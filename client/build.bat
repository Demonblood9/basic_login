@echo off
REM Build script for Modern Secure Login Client
REM This script attempts to build using available compilers

echo Modern Secure Login Client - Build Script
echo ==========================================
echo.

REM Try MSVC first (preferred for GDI+)
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found MSVC cl.exe, building with modern features...
    cl.exe /EHsc /DUNICODE /D_UNICODE /Fe:SecureLoginClient.exe main.cpp /link winhttp.lib comctl32.lib gdi32.lib gdiplus.lib user32.lib msimg32.lib /MANIFESTINPUT:app.manifest
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo Build successful! Output: SecureLoginClient.exe
        echo.
        echo Features enabled:
        echo   - Modern Windows 11 UI
        echo   - Rounded corners and shadows
        echo   - Smooth hover effects
        echo   - GDI+ anti-aliasing
        goto :success
    ) else (
        echo Build failed with MSVC
        goto :error
    )
)

REM Try MinGW
where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Found MinGW g++, building...
    echo Note: MinGW build may have limited modern UI features
    g++ -std=c++11 -municode -mwindows -O2 -Wall main.cpp -o SecureLoginClient.exe -lwinhttp -lcomctl32 -lgdi32 -lgdiplus -lmsimg32
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo Build successful! Output: SecureLoginClient.exe
        goto :success
    ) else (
        echo Build failed with MinGW
        goto :error
    )
)

REM No compiler found
echo ERROR: No C++ compiler found!
echo.
echo Please install one of the following:
echo   - Visual Studio 2019/2022: https://visualstudio.microsoft.com/ (Recommended)
echo   - MinGW-w64: https://www.mingw-w64.org/
echo.
goto :error

:success
echo.
echo You can now run: SecureLoginClient.exe
echo.
pause
exit /b 0

:error
echo.
pause
exit /b 1
