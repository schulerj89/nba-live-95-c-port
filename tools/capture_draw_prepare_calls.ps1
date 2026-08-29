param([Parameter(Mandatory=$true)][string]$OutputDir,[int]$Calls=2000)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_DRAW_CALLS="$Calls"
$env:NBA95_VECTOR_DRIVER=(Resolve-Path "$PSScriptRoot/mesen_func_vectors.lua").Path
$env:NBA95_VEC_ENTRY='888888'; $env:NBA95_VEC_EXITS='888889'
$env:NBA95_VEC_READS='0096-0097'; $env:NBA95_VEC_WRITES='0096-0097'
$env:NBA95_VEC_LABEL='driver'; $env:NBA95_VEC_MAX='100000'
$env:NBA95_VEC_DRIVE='1'; $env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='4000'; $env:NBA95_VEC_DELAY='0'
$script=(Resolve-Path "$PSScriptRoot/mesen_draw_prepare_calls.lua").Path
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source `
    -ArgumentList @('--testrunner','--timeout=300',
        '"F:/Games/SNES/NBA Live 95 (USA).sfc"',('"'+$script+'"')) `
    -PassThru -WindowStyle Hidden
$process.WaitForExit()
if($process.ExitCode -ne 0 -or
   !(Test-Path "$OutputDir/draw_calls_complete.txt")) {
    throw "Draw-call capture failed: $OutputDir"
}
Get-Content "$OutputDir/draw_calls_complete.txt"
