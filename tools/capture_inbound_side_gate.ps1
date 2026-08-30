[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
      [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='86F61F'
$env:NBA95_VEC_EXITS='86F648,86F653'
$env:NBA95_VEC_READS='009E-009F'
$env:NBA95_VEC_WRITES='009E-009F'
$env:NBA95_VEC_LABEL='inbound-side-driver'
$env:NBA95_VEC_MAX='100000'
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='30000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_VEC_SHARED_EXITS='1'
$script=(Resolve-Path "$PSScriptRoot\mesen_inbound_side_gate.lua").Path
$arguments=@('--testrunner','--timeout=300',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'inbound-side-gate-complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){throw "Incomplete native side-gate capture: $OutputDir"}
if((Get-Content -Raw -LiteralPath $sentinel).Trim()-ne 'controlled_cases=40'){throw 'Expected all 40 controlled cases.'}
$vectors=Join-Path $OutputDir 'inbound-side-gate.jsonl'
[ordered]@{oracle='Mesen 2.1.1 native ROM execution';controlled=$true;
 entry_pc='86F61F';exits=@('86F648','86F653');cases=40;
 protocol='Genuine Exhibition gate calls; temporarily replace context +0A, owner +04, receiver +04 and owner ID/+6E, restore at branch exit; no CPU or ROM writes.';
 rom_file_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $RomPath).Hash.ToLowerInvariant();
 vectors_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $vectors).Hash.ToLowerInvariant()
}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $OutputDir 'inbound-side-gate.meta.json') -Encoding utf8
Get-Content -LiteralPath $sentinel
