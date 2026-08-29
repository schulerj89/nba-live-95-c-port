param([Parameter(Mandatory=$true)][string]$OutputDir)
$ErrorActionPreference='Stop';New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path;$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='879B38';$env:NBA95_VEC_EXITS='879BC8'
$env:NBA95_VEC_READS='0900-09D7,34EB-3F4A';$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='dead-ball-reset';$env:NBA95_VEC_MAX='2';$env:NBA95_VEC_DRIVE='1';$env:NBA95_CPU_VS_CPU='1';$env:NBA95_VEC_FRAMES='5000'
$script=(Resolve-Path "$PSScriptRoot/mesen_dead_ball_reset_control.lua").Path
$p=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList @('--testrunner','--timeout=300','"F:/Games/SNES/NBA Live 95 (USA).sfc"',('"'+$script+'"')) -PassThru -WindowStyle Hidden
$p.WaitForExit();if($p.ExitCode-ne 0-or!(Test-Path "$OutputDir/capture_complete.txt")){throw 'dead-ball reset capture failed'};Get-Content "$OutputDir/capture_complete.txt"
