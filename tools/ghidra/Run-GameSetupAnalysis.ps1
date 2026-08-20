[CmdletBinding()]
param(
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$GhidraHome = 'C:\Users\joshs\Downloads\ghidra_11.3_PUBLIC_20250205\ghidra_11.3_PUBLIC',
    [string]$JdkHome = 'C:\Users\joshs\Downloads\jdk-21\jdk-21.0.6+7'
)

$ErrorActionPreference = 'Stop'

$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $projectRoot 'ghidra-projects'
$analysisDirectory = Join-Path $projectRoot '.analysis\setup_capture'
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'

if (-not (Test-Path -LiteralPath $headless -PathType Leaf)) { throw "Ghidra headless not found: $headless" }
if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf)) { throw "ROM not found: $RomPath" }

New-Item -ItemType Directory -Force -Path $projectDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $analysisDirectory | Out-Null
$env:JAVA_HOME = $JdkHome

# Entry points observed executing on the Game Setup screen (live Mesen trace).
$banks = @('80','81','83','87')

$romBytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))

foreach ($bank in $banks) {
    $bankIndex = [Convert]::ToInt32($bank, 16) -band 0x7F
    $bankOffset = $bankIndex * 0x8000
    if ($bankOffset + 0x8000 -gt $romBytes.Length) {
        Write-Warning "ROM too small for bank $bank"; continue
    }
    $bankPath = Join-Path $analysisDirectory ("bank_{0}.bin" -f $bank)
    $bankBytes = New-Object byte[] 0x8000
    [Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
    [System.IO.File]::WriteAllBytes($bankPath, $bankBytes)

    Write-Host "Analyzing bank `$$bank..." -ForegroundColor Cyan
    & $headless $projectDirectory ("NbaLive95Setup{0}" -f $bank) `
        -import $bankPath `
        -overwrite `
        -loader BinaryLoader `
        -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' `
        -scriptPath $PSScriptRoot `
        -noanalysis `
        -postScript 'DumpGameSetup.java' $analysisDirectory $bank
    if ($LASTEXITCODE -ne 0) { throw "Ghidra failed for bank $bank (exit $LASTEXITCODE)" }
}

Write-Host "Game Setup analysis written to: $analysisDirectory" -ForegroundColor Green
