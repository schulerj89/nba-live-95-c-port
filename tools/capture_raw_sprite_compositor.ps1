param([Parameter(Mandatory=$true)][string]$OutputDir,[int]$Calls=500)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_VEC_ENTRY='80B344,80B348'
$env:NBA95_VEC_EXITS='80B37B,80B43F,80B44E,80B51A,80B529'
$env:NBA95_VEC_READS='0000-002F,05E5-05FF,2000-27FF,3425-3434'
$env:NBA95_VEC_WRITES='0000-002F,05E5-05FF,2000-27FF,3425-3434'
$env:NBA95_VEC_LABEL='raw-sprite-compositor'
$env:NBA95_VEC_MAX="$Calls"
$env:NBA95_VEC_DRIVE='1'; $env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='5000'; $env:NBA95_VEC_DELAY='0'
$driver=(Resolve-Path "$PSScriptRoot/mesen_func_vectors.lua").Path
$arguments=@('--testrunner','--timeout=300',
    '"F:/Games/SNES/NBA Live 95 (USA).sfc"',('"'+$driver+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source `
    -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
if($process.ExitCode -ne 0 -or !(Test-Path "$OutputDir/capture_complete.txt")) {
    throw "Raw sprite-compositor capture failed: $OutputDir"
}
Get-Content "$OutputDir/capture_complete.txt"
