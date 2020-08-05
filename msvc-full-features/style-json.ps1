Write-Output "JSON formatting script begins."
$scriptDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
Set-Location -Path (Join-Path -Path $scriptDir -ChildPath "..")
$blacklist = Get-Content "json_blacklist" | Resolve-Path -Relative
$files = Get-ChildItem -Recurse -Include *.json "data" | Resolve-Path -Relative | ?{$blacklist -notcontains $_}
$files | ForEach-Object -Begin { $i = 0 } -Process { $i = $i+1 } { if( $i -eq 1 -or $i % 100 -eq 0 -or $i -eq $files.count ) { Write-Output "File $i of $($files.Count)" } } { Invoke-Expression ".\tools\format\json_formatter.exe $_" } -End $null
Write-Output "JSON formatting script ends."
