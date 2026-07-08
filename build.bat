@echo off
echo C++ Proxy sunucusu derleniyor...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
rc resource.rc
cl main.cpp resource.res ws2_32.lib iphlpapi.lib shlwapi.lib wininet.lib advapi32.lib user32.lib shell32.lib /O2 /EHsc
if %ERRORLEVEL% equ 0 (
    echo.
    echo main.exe
) else (
    echo.
    echo error
)
pause
