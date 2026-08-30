[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
 [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='8792A8'
$env:NBA95_VEC_EXITS='8794A2'
$env:NBA95_VEC_READS='0000-1FFF,3400-4AFF'
$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='violation_oob'
$env:NBA95_VEC_MAX='46'
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='4000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_VEC_SHARED_EXITS='0'
$script=(Resolve-Path "$PSScriptRoot\mesen_violation_oob_matrix.lua").Path
$arguments=@('--testrunner','--timeout=180',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){throw 'Native OOB capture failed.'}
$complete=(Get-Content -Raw -LiteralPath $sentinel).Trim()
if($complete-ne 'label=violation_oob vectors=46 orphan_exits=0 shared_exit_callbacks=0'){throw "Incomplete OOB capture: $complete"}
$metaPath=Join-Path $OutputDir 'violation_oob.meta.json'
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$meta|Add-Member -NotePropertyName controlled -NotePropertyValue $true
$meta|Add-Member -NotePropertyName protocol -NotePropertyValue '46 genuine parent calls, input WRAM controlled at87:92A5; capture87:92A8 to94A2 including native9B38/9B41; no ROM/CPU writes.'
$meta|Add-Member -NotePropertyName rom_file_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath $RomPath).Hash.ToLowerInvariant())
$meta|Add-Member -NotePropertyName vectors_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $OutputDir 'violation_oob.vectors.jsonl')).Hash.ToLowerInvariant())
$meta|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $metaPath -Encoding utf8
$complete
