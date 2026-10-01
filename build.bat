@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo Could not find:
    echo   %VCVARS%
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b 1

if not exist out mkdir out
del /q out\*.ilk out\*.pdb 2>nul

cl /nologo /std:c++20 /EHsc /permissive- /W4 /utf-8 /O2 ^
    /I src ^
    src\main.cpp src\renderer.cpp src\ui.cpp ^
    /Fo:out\ /Fe:out\hello.exe ^
    /link /SUBSYSTEM:WINDOWS /MACHINE:X64 /INCREMENTAL:NO ^
    user32.lib gdi32.lib ole32.lib uuid.lib d3d11.lib dxgi.lib d2d1.lib dwrite.lib dwmapi.lib

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo.
echo Built out\hello.exe
