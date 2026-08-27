param([Parameter(Mandatory=$true)][string]$OutputDir,[int]$Frames=600,[switch]$Controlled,[int]$Variant=-1,[switch]$LaunchControlled)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TIP_CONTROL=if($Controlled -or $LaunchControlled){'1'}else{'0'}
$env:NBA95_TIP_LAUNCH_CONTROL=if($LaunchControlled){'1'}else{'0'}
$env:NBA95_TIP_VARIANT="$Variant"
$env:NBA95_VECTOR_DRIVER=(Resolve-Path "$PSScriptRoot/mesen_func_vectors.lua").Path
$env:NBA95_VEC_ENTRY='86F34F';$env:NBA95_VEC_EXITS='86F439,86F40A,86F43A'
$env:NBA95_VEC_READS='0096-0097';$env:NBA95_VEC_WRITES='0096-0097'
$env:NBA95_VEC_LABEL='driver';$env:NBA95_VEC_DRIVE='1';$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_MAX='100000';$env:NBA95_VEC_FRAMES="$Frames"
$env:NBA95_VEC_SHARED_EXITS='1';$env:NBA95_VEC_PREGAME='0';$env:NBA95_VEC_DELAY='0'
$p=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList @('--testrunner','--timeout=300',
    '"F:/Games/SNES/NBA Live 95 (USA).sfc"',('"'+$PSScriptRoot+'/mesen_tipoff_flow.lua"')) -PassThru -WindowStyle Hidden
$p.WaitForExit()
if($p.ExitCode -ne 0 -or !(Test-Path "$OutputDir/capture_complete.txt")){throw 'Tip flow capture failed'}
Write-Output "Native tip flow capture completed: $OutputDir"
