param (
	[switch]$FromMSBuild = $false
)

# https://stackoverflow.com/a/11213394
function IsFileLocked( [string] $filePath ) {
    Rename-Item $filePath $filePath -ErrorVariable errs -ErrorAction SilentlyContinue
    return ( $errs.Count -ne 0 )
}

function ExitWithError( [string] $msg ) {
	if ( $FromMSBuild ) {
		Write-Output "jsonlint : lint warning cddalint01 : $msg";
	} else {
		Write-Output $msg
	}
	Exit 0 # Return 0 so build continues
}

# Track execution time, if script is called from Visual Studio and it goes over max then bail out early
$timer = [Diagnostics.Stopwatch]::StartNew()
$timer_max_seconds = 30

$scriptDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
Set-Location -Path ( Join-Path -Path $scriptDir -ChildPath ".." )
$formatter = Join-Path ( Resolve-Path . ) "tools\format\json_formatter.exe"

if ( -not( Test-Path -Path $formatter -PathType Leaf ) ) {
	ExitWithError( "Linting JSON skipped: Formatter executable not found at '$formatter'." )
}

if ( IsFileLocked( $formatter ) ) {
	ExitWithError( "Linting JSON skipped: Formatter executable file is locked, likely formatter project is building." )
}

# Probe for files changed in common "upstream" branches, only style those
$gitChanged = @( git diff --name-only '*.json' )
foreach ( $probe in @( 'master/data/json', 'master/data/mods', 'origin/master/data/json', 'origin/master/data/mods', 'upstream/master/data/json', 'upstream/master/data/mods', 'fork/master/data/json', 'fork/master/data/mods' ) ) {
	$mergebase = Invoke-Expression "git merge-base $probe HEAD" 2>&1
	if ( -not( $? ) ) {
		continue # skip missing branches
	}
	$gitChanged = ( @( $gitChanged ) + @( git diff --name-only $mergebase '*.json' ) ) | Select-Object -Unique
	if ( -not( $? ) ) {
    Exit 0 # No point spamming "error" messages because no remote version of a file exists
	}
}

if( $gitChanged.count -eq 0 ) {
	ExitWithError( "Linting JSON skipped: git reported no changes in *.json files." )
}

$gitChanged | ForEach-Object -Begin { $i = 0 } -Process { $i = $i+1 } {
	if( $i -eq 1 -or $i % 100 -eq 0 -or $i -eq $gitChanged.count ) {
		Write-Output "File $i of $( $gitChanged.Count )"
	}
} {
	if ( $FromMSBuild ) { # parse and re-print in a format Visual Studio uses for error log
		if ( [math]::Floor( $timer.Elapsed.TotalSeconds ) -gt $timer_max_seconds ) {
			ExitWithError( "Linting JSON aborted: Took more than $timer_max_seconds seconds, run the lint script manually or raise build lint limit." )
		}

		$input = Invoke-Expression "$formatter $_" | Out-String
		$regex = new-object RegEx('(?mi)^(.*):(\d+):(\d+): ([^\n]+)$')
		foreach ( $res in $regex.matches( $input ) ) {
			$line = $res.Groups[2].Value;
			$col = $res.Groups[3].Value;
			$msg = $res.Groups[4].Value;
			$file = Join-Path ( Resolve-Path . ) $_
			Write-Output "$file($line,$col) : lint warning cddalint01 : $msg";
			break
		}
	} else {
		Invoke-Expression "$formatter $_"
	}
  Write-Output "JSON linting done"
} -End $null
