[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
 [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
if(Get-Process -Name Mesen -ErrorAction SilentlyContinue){throw 'Close other Mesen instances before capture.'}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='85AD6B'
$env:NBA95_VEC_EXITS='85AD77,85AF5B'
$env:NBA95_VEC_READS='0000-00FF,0800-0AFF,1600-18FF,3400-4AFF'
$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='formation_override'
$env:NBA95_VEC_MAX='10'
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='12000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_VEC_SHARED_EXITS='0'
$env:NBA95_VEC_PREGAME='0'
$env:NBA95_VEC_FORCE_PLAY_REQUEST='0'
$env:NBA95_VEC_FORCE_SUB_REQUEST='0'
$script=(Resolve-Path "$PSScriptRoot\mesen_formation_override.lua").Path
$arguments=@('--testrunner','--timeout=180',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){throw 'Native formation-override capture failed.'}
$complete=(Get-Content -Raw -LiteralPath $sentinel).Trim()
if($complete-ne 'label=formation_override vectors=10 orphan_exits=0 shared_exit_callbacks=0'){throw "Incomplete formation-override capture: $complete"}
$metaPath=Join-Path $OutputDir 'formation_override.meta.json'
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$meta|Add-Member -NotePropertyName controlled -NotePropertyValue $true
$meta|Add-Member -NotePropertyName matrix -NotePropertyValue 'override10'
$meta|Add-Member -NotePropertyName protocol -NotePropertyValue '10 genuine AD6B calls: plays6..9 x slots2/7 plus0958=0 and095A=-1 skip controls. Only documented WRAM is controlled at reached entries: coherent C2/96/9E/E0/E2 actor/context and roster pointer words, anchors46F5/4775, gates093E/0968/0936/0954/0958/095A/09A2/0996/0998/099C/0948/097C/005C, current actor position/velocity/timer/team/flags, all actor identity/controller/target words. Actor records and changed input WRAM restored after native return snapshot. No CPU/ROM/stack/RNG edits.'
$meta|Add-Member -NotePropertyName rom_file_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath $RomPath).Hash.ToLowerInvariant())
foreach($name in @('formation_override.vectors.jsonl','formation-override-cases.jsonl','formation-override-pcs.jsonl')){
 $property=$name.Replace('.','_').Replace('-','_')+'_sha256'
 $meta|Add-Member -NotePropertyName $property -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $OutputDir $name)).Hash.ToLowerInvariant())
}
$meta|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $metaPath -Encoding utf8
$complete
