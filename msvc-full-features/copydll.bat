@echo off
pwd
mkdir dll
del dll\*.dll
xcopy /y ..\WinDepend\bin\%1\*.dll dll\
