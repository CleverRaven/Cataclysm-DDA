@ECHO OFF
CLS

DEL /F C:\Projects\CDDA\format\format_out.json
DEL /F C:\Projects\CDDA\format\format_err.json

rem FOR /R "C:\Projects\CDDA\format\json-xdda\npc" %%f IN (*.json) DO (
rem FOR /R "C:\Source\Repos\Cataclysm-DDA\data\json" %%f IN (*.json) DO (
FOR /R "C:\Source\Repos\Cataclysm-DDA\data\json\items\" %%f IN (*.json) DO (
rem FOR /R "C:\Source\Repos\Cataclysm-DDA\tools\format\examples\" %%f IN (*.json) DO (
ECHO Processing file: %%f
perl format.pl %%f 1>> C:\Projects\CDDA\format\format_out.json 2>> C:\Projects\CDDA\format\format_err.json
)
