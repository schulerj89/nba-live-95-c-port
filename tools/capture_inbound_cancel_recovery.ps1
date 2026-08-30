[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
 [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='86F43A'
$env:NBA95_VEC_EXITS='86F439,86F653'
$env:NBA95_VEC_READS='009E-009F'
$env:NBA95_VEC_WRITES='009E-009F'
$env:NBA95_VEC_LABEL='inbound-cancel-driver'
$env:NBA95_VEC_MAX='100000'
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='15000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_VEC_SHARED_EXITS='1'
$script=(Resolve-Path "$PSScriptRoot\mesen_inbound_cancel_recovery.lua").Path
$arguments=@('--testrunner','--timeout=180',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'inbound-cancel-recovery-complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){throw 'Native canceled-transfer capture failed.'}
if((Get-Content -Raw -LiteralPath $sentinel).Trim()-ne 'controlled_cases=4'){throw 'Expected all four cancellation cases.'}
$vectors=Join-Path $OutputDir 'inbound-cancel-recovery.jsonl'
[ordered]@{oracle='Mesen 2.1.1 native ROM execution';controlled=$true;
 entry_pc='86F43A';exit_pc='86F58F';cases=4;
 protocol='Genuine already-arrived Exhibition F43A entries; replace only09B8/0946 with canceled, valid, noncanonical-nonzero and inactive cases; full0000-4AFF entry/exit memory, restore controlled words atF58F. No CPU/ROM writes.';
 rom_file_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $RomPath).Hash.ToLowerInvariant();
 vectors_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $vectors).Hash.ToLowerInvariant()
}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $OutputDir 'inbound-cancel-recovery.meta.json') -Encoding utf8
Get-Content -LiteralPath $sentinel
