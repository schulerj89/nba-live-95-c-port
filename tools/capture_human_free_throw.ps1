[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$OutputDir,
    [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc',
    [int]$Calls=2000,
    [ValidateRange(1,1000)][int]$FirstPressDelay=60
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='879CBF,87A018'
$env:NBA95_VEC_EXITS='87A017,87A045'
$env:NBA95_VEC_READS='0000-00FF,08D0-0A0F,3435-3F4A,46EB-48FF,492F-493F'
$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='human-free-throw'
$env:NBA95_VEC_MAX="$Calls"
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='12000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_HUMAN_FT_FIRST_DELAY="$FirstPressDelay"
$env:NBA95_VEC_SHARED_EXITS='1'
$script=(Resolve-Path "$PSScriptRoot\mesen_human_free_throw_control.lua").Path
$arguments=@('--testrunner','--timeout=300',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source `
    -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){
    throw "Human free-throw capture failed: $OutputDir"
}
$complete=Get-Content -Raw -LiteralPath $sentinel
if($complete -notmatch '^label=human-free-throw vectors=(\d+) orphan_exits=0 shared_exit_callbacks=0\s*$'){
    throw "Human free-throw capture reported incomplete call pairing: $complete"
}
if([int]$Matches[1] -lt 1200){throw "Human free-throw capture ended before the durable sequence"}
$vectors=Join-Path $OutputDir 'human-free-throw.vectors.jsonl'
$metaPath=Join-Path $OutputDir 'human-free-throw.meta.json'
if(!(Test-Path -LiteralPath $vectors)-or!(Test-Path -LiteralPath $metaPath)){
    throw 'Human free-throw capture omitted vectors or metadata.'
}
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$meta|Add-Member -NotePropertyName rom_file_sha256 -NotePropertyValue `
    ((Get-FileHash -Algorithm SHA256 -LiteralPath $RomPath).Hash.ToLowerInvariant())
$meta|Add-Member -NotePropertyName vectors_sha256 -NotePropertyValue `
    ((Get-FileHash -Algorithm SHA256 -LiteralPath $vectors).Hash.ToLowerInvariant())
$meta|Add-Member -NotePropertyName first_press_delay -NotePropertyValue $FirstPressDelay
$meta|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $metaPath -Encoding utf8
$complete
