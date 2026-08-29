param([Parameter(Mandatory=$true)][string]$OutputDir,
      [int]$Calls=50, [int]$Frames=4000)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_VEC_ENTRY='87A47A'
$env:NBA95_VEC_EXITS='87A845'
# Ghidra-owned direct page, gameplay state/actor records, pointer list and
# temporary presentation queues. Avoid a whole-WRAM snapshot per call.
$env:NBA95_VEC_READS='0000-0AFF,3400-4053,7E44-7E59,8EC4-8F3F'
$env:NBA95_VEC_WRITES='0000-0AFF,3400-4053,8EC4-8F3F'
$env:NBA95_VEC_LABEL='draw-prepare'
$env:NBA95_VEC_MAX="$Calls"
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES="$Frames"
$env:NBA95_VEC_DELAY='0'
$driver=(Resolve-Path "$PSScriptRoot/mesen_func_vectors.lua").Path
$arguments=@('--testrunner','--timeout=300',
    '"F:/Games/SNES/NBA Live 95 (USA).sfc"',('"'+$driver+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source `
    -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
if($process.ExitCode -ne 0 -or !(Test-Path "$OutputDir/capture_complete.txt")) {
    throw "Draw-preparation capture failed: $OutputDir"
}
Get-Content "$OutputDir/capture_complete.txt"
