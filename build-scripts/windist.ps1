if (Test-path bindist) {
  rm -Force -Recurse bindist
}

mkdir bindist
cp cataclysm-tiles.exe bindist/cataclysm-tiles.exe
cp cataclysm-tiles.stripped.pdb bindist/cataclysm-tiles.pdb
cp tools/format/json_formatter.exe bindist/json_formatter.exe
cp zzip.exe bindist/zzip.exe

# SDL dynamic-links mpg123 (LGPL). Stage the mpg123 family DLLs
# (libmpg123/out123/syn123) from wherever vcpkg/msbuild left them.
$dllSearchRoots = @(
	'.',
	'msvc-full-features',
	'msvc-full-features\vcpkg_installed\x64-windows-static\bin',
	'msvc-full-features\vcpkg_installed\x86-windows-static\bin'
)
$dllPatterns = @('mpg123*.dll', 'libmpg123*.dll', 'out123*.dll', 'libout123*.dll', 'syn123*.dll', 'libsyn123*.dll')
$staged = @{}
ForEach ($root in $dllSearchRoots) {
	if (Test-Path $root) {
		ForEach ($pattern in $dllPatterns) {
			$found = Get-ChildItem -Path $root -Filter $pattern -Recurse -ErrorAction SilentlyContinue
			ForEach ($dll in $found) {
				if (-not $staged.ContainsKey($dll.Name)) {
					cp $dll.FullName bindist/
					$staged[$dll.Name] = $true
				}
			}
		}
	}
}

# vcpkg nests the triplet dir (vcpkg_installed\<triplet>\<triplet>\share),
# so find the mpg123 copyright recursively rather than at a fixed depth.
if ($staged.Count -gt 0) {
	$copyright = Get-ChildItem -Path 'msvc-full-features\vcpkg_installed' -Recurse -Filter 'copyright' -ErrorAction SilentlyContinue |
		Where-Object { $_.FullName -match '[\\/]mpg123[\\/]copyright$' } |
		Select-Object -First 1
	if ($copyright) {
		cp $copyright.FullName bindist/LICENSE-mpg123.txt
	} else {
		throw "mpg123 DLL staged without its copyright; refusing to ship LGPL binary."
	}
}

# mpg123 is linked dynamically (LGPL), so the DLL must be packaged and stay
# replaceable. Empty staging means msbuild left it elsewhere; fail rather than
# ship without MP3.
if ($staged.Count -eq 0) {
	throw "Staged no mpg123 DLLs; dynamic mpg123 missing from package."
}

mkdir bindist/lang
cp -r lang/mo bindist/lang

$extras = "data", "doc", "gfx", "LICENSE.txt", "LICENSE-OFL-Terminus-Font.txt", "README.md", "VERSION.txt"
ForEach ($extra in $extras) {
	cp -r $extra bindist
}
Compress-Archive -Force -Path bindist/* -DestinationPath "cataclysmdda-0.J.zip"
