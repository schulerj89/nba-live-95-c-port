[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$OutputDir,
    [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc'
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
if(Get-Process -Name Mesen -ErrorAction SilentlyContinue){throw 'Close other Mesen instances before capture.'}
$rom=(Resolve-Path -LiteralPath $RomPath).Path
$mesen=(Get-Command Mesen.exe).Source
New-Item -ItemType Directory -Path $OutputDir|Out-Null
$capture=(Resolve-Path -LiteralPath $OutputDir).Path
$script=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'mesen_cpu_defense_context.lua')).Path
$variables=@{
    NBA95_CAPTURE_DIR=$capture; NBA95_TOOL_DIR=$PSScriptRoot;
    NBA95_VEC_ENTRY='85B130'; NBA95_VEC_EXITS='85B176';
    NBA95_VEC_READS='009E-009F,07F6-07F7,0926-0927,0994-0995,46EB-486A';
    NBA95_VEC_WRITES='009E-009F,07F6-07F7,0926-0927,0994-0995,46EB-486A';
    NBA95_VEC_LABEL='cpu_defense_context'; NBA95_VEC_MAX='6';
    NBA95_VEC_DRIVE='1'; NBA95_CPU_VS_CPU='1'; NBA95_VEC_FRAMES='12000';
    NBA95_VEC_DELAY='0'; NBA95_VEC_SHARED_EXITS='0'; NBA95_VEC_PREGAME='0';
    NBA95_VEC_FORCE_PLAY_REQUEST='0'; NBA95_VEC_FORCE_SUB_REQUEST='0'
}
$previous=@{}
try{
    foreach($name in $variables.Keys){
        $previous[$name]=[Environment]::GetEnvironmentVariable($name,'Process')
        [Environment]::SetEnvironmentVariable($name,$variables[$name],'Process')
    }
    $arguments=@('--testrunner','--timeout=180',('"'+$rom.Replace('\','/')+'"'),('"'+$script+'"'))
    $process=Start-Process -FilePath $mesen -ArgumentList $arguments -PassThru -WindowStyle Hidden
    $process.WaitForExit()
}finally{
    foreach($name in $variables.Keys){
        [Environment]::SetEnvironmentVariable($name,$previous[$name],'Process')
    }
}
$sentinel=Join-Path $capture 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){
    throw "Native defense-context capture failed: exit=$($process.ExitCode)"
}
$complete=(Get-Content -Raw -LiteralPath $sentinel).Trim()
if($complete-ne 'label=cpu_defense_context vectors=6 orphan_exits=0 shared_exit_callbacks=0'){
    throw "Incomplete defense-context capture: $complete"
}
$metaPath=Join-Path $capture 'cpu_defense_context.meta.json'
$meta=Get-Content -Raw -LiteralPath $metaPath|ConvertFrom-Json
$meta|Add-Member -NotePropertyName controlled -NotePropertyValue $true
$meta|Add-Member -NotePropertyName oracle -NotePropertyValue 'original USA ROM executed by Mesen 2'
$meta|Add-Member -NotePropertyName protocol -NotePropertyValue 'Six genuine $85:B130 calls at the reached play-request boundary. Scores, raw period, opposing activity/mode, and RNG seed are controlled at entry and restored after the $85:B176 exit snapshot. No PC, stack, CPU-flag, or ROM edits.'
$meta|Add-Member -NotePropertyName rom_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash.ToLowerInvariant())
$meta|Add-Member -NotePropertyName mesen_sha256 -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath $mesen).Hash.ToLowerInvariant())
foreach($name in @('cpu_defense_context.vectors.jsonl','cpu-defense-context-cases.jsonl','cpu-defense-context-pcs.jsonl')){
    $property=$name.Replace('.','_').Replace('-','_')+'_sha256'
    $meta|Add-Member -NotePropertyName $property -NotePropertyValue ((Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $capture $name)).Hash.ToLowerInvariant())
}
$meta|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $metaPath -Encoding utf8
$complete
