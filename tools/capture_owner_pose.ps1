param([Parameter(Mandatory=$true)][string]$OutputDir,
      [switch]$Controlled, [int]$Frames=6000)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_VECTOR_DRIVER=(Resolve-Path "$PSScriptRoot/mesen_func_vectors.lua").Path
$env:NBA95_POSE_CONTROL=if($Controlled){'1'}else{'0'}
$env:NBA95_VEC_ENTRY='86E4F5'
$env:NBA95_VEC_EXITS='86E518,86E51F,86E534,86E544'
$env:NBA95_VEC_READS='0096-0097'
$env:NBA95_VEC_WRITES='0096-0097'
$env:NBA95_VEC_LABEL='driver'
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_MAX='100000'
$env:NBA95_VEC_FRAMES="$Frames"
$env:NBA95_VEC_SHARED_EXITS='1'
$env:NBA95_VEC_PREGAME='0'
$env:NBA95_VEC_DELAY='0'
$mesen=(Get-Command Mesen.exe).Source
$p=Start-Process -FilePath $mesen -ArgumentList @('--testrunner','--timeout=300',
    '"F:/Games/SNES/NBA Live 95 (USA).sfc"',
    ('"'+$PSScriptRoot+'/mesen_owner_pose_capture.lua"')) -PassThru -WindowStyle Hidden
$p.WaitForExit()
if($p.ExitCode -ne 0 -or !(Test-Path "$OutputDir/capture_complete.txt")) {
    throw "ROM capture did not finish successfully: $OutputDir"
}
Get-Content "$OutputDir/capture_complete.txt"
Get-Content "$OutputDir/pose_progress.txt"
