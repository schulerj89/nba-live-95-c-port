[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
 [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='85AD6B'
$env:NBA95_VEC_EXITS='85AD77,85AF5B'
$env:NBA95_VEC_READS='0000-00FF,0800-0AFF,1600-18FF,3400-4AFF'
$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='formation_anchors'
$env:NBA95_VEC_MAX='32'
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='12000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_VEC_SHARED_EXITS='0'
$script=(Resolve-Path "$PSScriptRoot\mesen_formation_anchor_cases.lua").Path
$arguments=@('--testrunner','--timeout=180',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){throw 'Native formation-anchor capture failed.'}
$complete=(Get-Content -Raw -LiteralPath $sentinel).Trim()
if($complete-ne 'label=formation_anchors vectors=32 orphan_exits=0 shared_exit_callbacks=0'){throw "Incomplete formation-anchor capture: $complete"}
$metaPath=Join-Path $OutputDir 'formation_anchors.meta.json'
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$meta|Add-Member -NotePropertyName controlled -NotePropertyValue $true
$meta|Add-Member -NotePropertyName protocol -NotePropertyValue '32 genuine AD6B calls; coherent actor/context DP words and E0 roster pointer per87:9127-9136, live context anchor sign, play, mirror and branch WRAM controlled before capture. Restore changed input WRAM and actor records after captured return. No CPU/ROM/stack/RNG edits.'
$meta|Add-Member -NotePropertyName rom_file_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath $RomPath).Hash.ToLowerInvariant())
$meta|Add-Member -NotePropertyName vectors_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $OutputDir 'formation_anchors.vectors.jsonl')).Hash.ToLowerInvariant())
$meta|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $metaPath -Encoding utf8
$complete
