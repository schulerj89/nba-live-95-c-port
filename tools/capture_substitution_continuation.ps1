[CmdletBinding()]
param(
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))) `
        '.analysis\substitution-continuation-native'
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
Remove-Item -LiteralPath (Join-Path $OutputDirectory 'capture_complete.txt') -Force -ErrorAction SilentlyContinue
$env:NBA95_CAPTURE_DIR = $OutputDirectory
$env:NBA95_VEC_ENTRY = '83ED73'
$env:NBA95_VEC_EXITS = '83EE4F'
$env:NBA95_VEC_READS = '08D0-0A0F,1800-18FF,3435-3FFF,46EB-4A7F,8E00-90FF'
$env:NBA95_VEC_WRITES = $env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL = 'foul_out_lineup_transaction'
$env:NBA95_VEC_MAX = '1'
$env:NBA95_VEC_DRIVE = '1'
$env:NBA95_CPU_VS_CPU = '1'
$env:NBA95_VEC_DELAY = '0'
$env:NBA95_VEC_FRAMES = '7200'
$driver = (Resolve-Path (Join-Path $PSScriptRoot 'mesen_func_vectors.lua')).Path
$mesen = (Get-Command Mesen.exe -ErrorAction Stop).Source
$process = Start-Process -FilePath $mesen -ArgumentList @(
    '--testrunner','--timeout=300',('"' + $RomPath + '"'),('"' + $driver + '"')) `
    -PassThru -WindowStyle Hidden
$process.WaitForExit()
if ($process.ExitCode -ne 0) { throw "Mesen substitution capture failed: $($process.ExitCode)" }
$sentinel = Join-Path $OutputDirectory 'capture_complete.txt'
if (-not (Test-Path -LiteralPath $sentinel)) { throw 'Mesen did not produce capture_complete.txt' }
Get-Content -LiteralPath $sentinel
