Cls

function Get-Terrain  {
    Switch ( $args[0] ) {
        0 { [string]$overmap_terrain = "field" }
        1 { [string]$overmap_terrain = "forest" }
        2 { [string]$overmap_terrain = "forest_thick" }
        3 { [string]$overmap_terrain = "forest_water" }
        4 { [string]$overmap_terrain = "lake_surface" }
        5 { [string]$overmap_terrain = "lake_shore" }
        9 { [string]$overmap_terrain = "lake_shore" }
        Default { [string]$overmap_terrain = "empty_rock" }
    }
    return $overmap_terrain
}

function Compress-RLE ($s) {
    [string]$ret = ""

    [string]$prev = $s.Substring(0,1)
    [int]$cnt = 1
    [int]$max = [int]($s.Length) - 1

    for([int]$i = $cnt; [int]$i -le $max; $i++) {
        $c = $s.Substring( $i, 1 )
        if( $c -eq $prev ) {
            [int]$cnt = $cnt + 1
        } else {
            [string]$ret += "[`"$(Get-Terrain($prev))`",$($cnt)],"
            #[string]$ret += "$(Get-Terrain($prev))`t$cnt`n"
            [string]$prev = $c
            $cnt = 1;
        }
    }
    [string]$ret += "[`"$(Get-Terrain($prev))`",$($cnt)],"
    #[string]$ret += "$(Get-Terrain($prev))`t$cnt`n"

    [string]$ret = $ret.Substring(0, $ret.Length - 1)
    return $ret
}

#[string]$input_file = "C:\Projects\~\MA\MA_overmap\MA_overmap_small.tsv"
[string]$input_file = "C:\Projects\~\MA\MA_overmap\MA_overmap.tsv"
[string]$output_path = "C:\Projects\~\MA\MA_overmap\MA_overmap\"

[array]$input_lines = Get-Content $input_file -TotalCount 1

[array]$input_line_first = $input_lines[0].Split("`t")

[int]$om_y_prev = $input_line_first[0]
[int]$om_x_prev = $input_line_first[1]

[string]$output_file = $output_path +"overmap_" + $om_x_prev + "_" + $om_y_prev + ".omap"

[string]$land_use_codes = ""

Write-Host "Overmap changed to "$om_y_prev`,$om_x_prev

ForEach ($input_line In [System.IO.File]::ReadLines($input_file)) {

    [array]$input_array = $input_line.Split("`t")
   
    [int]$om_y = $input_array[0]
    [int]$om_x = $input_array[1]
    [int]$omt_y = $input_array[2]
    [int]$omt_x = $input_array[3]
    [string]$land_use_code = $input_array[4]

    if( ( $om_y_prev -ne $om_y ) -or ( $om_x_prev -ne $om_x ) ) {
        Write-Host "Overmap changed from "$om_y_prev`,$om_x_prev" to "$om_y`,$om_x
        [string]$output_file = $output_path +"overmap_" + $om_x_prev + "_" + $om_y_prev + ".omap"
        [string]$rle = Compress-RLE($land_use_codes)
        #[string]$rle = $rle.Substring(0, $rle.Length - 1)
        
        Write-Output [`{`"type`":`"overmap`"`,`"om_pos`":[$om_x_prev`,$om_y_prev]`,`"layers`":[ | Out-File -FilePath $output_file -Encoding ascii
        Write-Output $rle | Out-File -FilePath $output_file -Append -Encoding ascii
        Write-Output ]`}] | Out-File -FilePath $output_file -Append -Encoding ascii

        [int]$om_y_prev = $om_y
        [int]$om_x_prev = $om_x
        [string]$land_use_codes = $land_use_code
        [string]$land_use_code = ""
    } else {
        [string]$land_use_codes += $land_use_code
    }
}

[string]$output_file = $output_path +"overmap_" + $om_x_prev + "_" + $om_y_prev + ".omap"

Write-Output [`{`"type`":`"overmap`"`,`"om_pos`":[$om_x_prev`,$om_y_prev]`,`"layers`":[ | Out-File -FilePath $output_file -Encoding ascii
Write-Output $rle | Out-File -FilePath $output_file -Append -Encoding ascii
Write-Output ]`}] | Out-File -FilePath $output_file -Append -Encoding ascii
