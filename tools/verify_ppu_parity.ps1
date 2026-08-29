param(
    [Parameter(Mandatory=$true)][string]$RomPath,
    [string]$OutputDir = ''
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$rom = [IO.Path]::GetFullPath($RomPath)
if (!(Test-Path -LiteralPath $rom)) { throw "ROM not found: $rom" }
$mesen = (Get-Command Mesen.exe -ErrorAction Stop).Source
if ([string]::IsNullOrEmpty($OutputDir)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $OutputDir = Join-Path $root ".analysis\ppu-parity-$stamp"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$capture = (Resolve-Path $OutputDir).Path
$env:NBA95_CAPTURE_DIR = $capture
$env:NBA95_CPU_VS_CPU = '1'
$env:NBA95_TIPOFF_FRAMES = '990'
$env:NBA95_PPU_AUDIT_FROM = '988'
$env:NBA95_PPU_AUDIT_TO = '989'
$env:NBA95_SCREENSHOT_EVERY = '1000'
$lua = (Resolve-Path (Join-Path $PSScriptRoot 'mesen_tipoff_capture.lua')).Path.Replace('\','/')
$arguments = @('--testrunner','--timeout=300',('"'+$rom.Replace('\','/')+'"'),('"'+$lua+'"'))
$process = Start-Process -FilePath $mesen -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
if ($process.ExitCode -ne 0 -or !(Test-Path (Join-Path $capture 'capture_complete.txt'))) {
    throw 'Native PPU scanout capture failed.'
}

& (Join-Path $root 'build.ps1') -RomPath $rom
if ($LASTEXITCODE -ne 0) { throw 'Port build failed.' }
& (Join-Path $PSScriptRoot 'build_vector_probe.ps1') -Name ppu_snapshot_probe
if ($LASTEXITCODE -ne 0) { throw 'PPU snapshot probe build failed.' }
$probe = Join-Path $root 'build\ppu_snapshot_probe.exe'
$output = Join-Path $capture 'c_scanout_0989.bmp'
$report = Join-Path $capture 'ppu-parity-report.json'
& python (Join-Path $PSScriptRoot 'test_ppu_snapshot_parity.py') `
    --probe $probe --native-dir $capture --prefix scanout --frame 989 `
    --raster-log (Join-Path $capture 'ppu_raster_writes.txt') `
    --out $output --report $report
if ($LASTEXITCODE -ne 0) { throw 'PPU scanout parity failed.' }
Write-Host "PPU parity evidence: $report" -ForegroundColor Green
