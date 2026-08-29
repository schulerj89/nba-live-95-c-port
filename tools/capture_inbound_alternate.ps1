param([Parameter(Mandatory=$true)][string]$OutputDir)
$ErrorActionPreference='Stop';New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path;$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='86F5D2';$env:NBA95_VEC_EXITS='86F60B,86F653'
$env:NBA95_VEC_READS='0000-00FF,0900-0AFF,34EB-3EEA';$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='inbound-alternate';$env:NBA95_VEC_MAX='2';$env:NBA95_VEC_DRIVE='1';$env:NBA95_CPU_VS_CPU='1';$env:NBA95_VEC_FRAMES='5000';$env:NBA95_VEC_SHARED_EXITS='1'
$script=(Resolve-Path "$PSScriptRoot/mesen_inbound_alternate_control.lua").Path
$p=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList @('--testrunner','--timeout=300','"F:/Games/SNES/NBA Live 95 (USA).sfc"',('"'+$script+'"')) -PassThru -WindowStyle Hidden
$p.WaitForExit();if($p.ExitCode-ne 0-or!(Test-Path "$OutputDir/capture_complete.txt")){throw 'alternate inbound capture failed'};Get-Content "$OutputDir/capture_complete.txt"
