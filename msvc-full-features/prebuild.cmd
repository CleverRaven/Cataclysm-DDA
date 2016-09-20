@echo off
SETLOCAL

cd ..\src\lua
echo Generating lua bindings
lua generate_bindings.lua
cd ..\..\msvc-full-features
echo Done

echo Generating "version.h"...
for /F "tokens=*" %%i in ('git describe --tags --always --dirty --match "[0-9]*.*"') do set VERSION=%%i
echo VERSION defined as %VERSION%
>..\src\version.h echo #define VERSION "%VERSION%"
