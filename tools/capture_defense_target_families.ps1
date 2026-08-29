param([Parameter(Mandatory=$true)][string]$OutputRoot,[int]$Frames=12000)
$ErrorActionPreference='Stop'
$driver=(Resolve-Path "$PSScriptRoot/mesen_func_vectors.lua").Path
$rom='F:/Games/SNES/NBA Live 95 (USA).sfc'
$mesen=(Get-Command Mesen.exe).Source
$families=@(
    @{Label='mode2_context3';Entry='86E6B7';Exits='86E82E'},
    @{Label='mode24_weakside';Entry='86E7B3';Exits='86E82E'},
    @{Label='mode24_position';Entry='86E96F';Exits='86E82E'},
    @{Label='mode6_target';Entry='86E9B3';Exits='86EA03'}
)
$processes=@()
foreach($family in $families) {
    $out=Join-Path $OutputRoot $family.Label
    New-Item -ItemType Directory -Force $out | Out-Null
    $env:NBA95_CAPTURE_DIR=(Resolve-Path $out).Path
    $env:NBA95_VEC_ENTRY=$family.Entry
    $env:NBA95_VEC_EXITS=$family.Exits
    $env:NBA95_VEC_READS='0000-4aff'
    $env:NBA95_VEC_WRITES='0000-4aff'
    $env:NBA95_VEC_LABEL=$family.Label
    $env:NBA95_VEC_MAX='200'
    $env:NBA95_VEC_DRIVE='1'
    $env:NBA95_CPU_VS_CPU='1'
    $env:NBA95_VEC_DELAY='0'
    $env:NBA95_VEC_FRAMES="$Frames"
    $env:NBA95_VEC_SHARED_EXITS='0'
    $processes += @{
        Label=$family.Label;Out=$out;Process=(Start-Process -FilePath $mesen `
            -ArgumentList @('--testrunner','--timeout=300',('"'+$rom+'"'),
                            ('"'+$driver+'"')) -PassThru -WindowStyle Hidden)
    }
}
foreach($item in $processes) {
    $item.Process.WaitForExit()
    if($item.Process.ExitCode -ne 0 -or
       !(Test-Path (Join-Path $item.Out 'capture_complete.txt'))) {
        throw "Defense target capture failed: $($item.Label)"
    }
    Get-Content (Join-Path $item.Out 'capture_complete.txt')
}
