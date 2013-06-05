@echo off
SETLOCAL
for /F "tokens=*" %%i in ('git describe --tags --always --dirty --match "[0-9]*.[0-9]*"') do set VERSION=%%i
>..\version.h echo #define VERSION "%VERSION%"