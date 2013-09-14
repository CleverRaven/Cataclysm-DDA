@echo off
SETLOCAL
echo Generating "version.h"...
for /F "tokens=*" %%i in ('git describe --tags --always --dirty --match "[0-9]*.[0-9]*"') do set VERSION=%%i
>..\version.h echo #define VERSION "%VERSION%"