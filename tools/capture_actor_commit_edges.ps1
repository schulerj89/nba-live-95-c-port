[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
 [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='8596B5'
$env:NBA95_VEC_EXITS='85990F,859961'
$env:NBA95_VEC_READS='0000-00FF,0800-0AFF,1600-18FF,3400-4AFF'
$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='actor_commit_edges'
$env:NBA95_VEC_MAX='56'
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='4000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_VEC_SHARED_EXITS='0'
$script=(Resolve-Path "$PSScriptRoot\mesen_actor_commit_edges.lua").Path
$arguments=@('--testrunner','--timeout=180',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){throw 'Native actor-edge capture failed.'}
$complete=(Get-Content -Raw -LiteralPath $sentinel).Trim()
if($complete-ne 'label=actor_commit_edges vectors=56 orphan_exits=0 shared_exit_callbacks=0'){throw "Incomplete actor-edge capture: $complete"}
$metaPath=Join-Path $OutputDir 'actor_commit_edges.meta.json'
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$meta|Add-Member -NotePropertyName controlled -NotePropertyValue $true
$meta|Add-Member -NotePropertyName protocol -NotePropertyValue '56 genuine96B5 entries with nativeC6=2; actor XY/fractions/velocities/mode/timer controlled; grounded/no landing action and7E=2 isolate clamp from stochastic facing; restore actor after recorded990F/9961 exit. No ROM/CPU edits.'
$meta|Add-Member -NotePropertyName rom_file_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath $RomPath).Hash.ToLowerInvariant())
$meta|Add-Member -NotePropertyName vectors_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $OutputDir 'actor_commit_edges.vectors.jsonl')).Hash.ToLowerInvariant())
$meta|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $metaPath -Encoding utf8
$complete
