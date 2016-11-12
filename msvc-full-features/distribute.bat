@echo off
mkdir distribution
xcopy dll\*.dll distribution\
xcopy /s /e /i ..\data distribution\data
xcopy /s /e /i ..\config distribution\config
xcopy /s /e /i ..\gfx distribution\gfx
xcopy /s /e /i ..\lang\mo distribution\lang\mo
xcopy /s /e /i ..\lua distribution\lua
copy ..\Cataclysm.exe distribution\
echo Distribution files has been put into `distribution\' directory.
pause
