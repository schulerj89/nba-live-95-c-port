[CmdletBinding()]
param(
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$GhidraHome = 'C:\Users\joshs\Downloads\ghidra_11.3_PUBLIC_20250205\ghidra_11.3_PUBLIC',
    [string]$JdkHome = 'C:\Users\joshs\Downloads\jdk-21\jdk-21.0.6+7'
)

$ErrorActionPreference = 'Stop'

$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $projectRoot 'ghidra-projects'
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'

if (-not (Test-Path -LiteralPath $headless -PathType Leaf)) {
    throw "Ghidra headless analyzer not found: $headless"
}
if (-not (Test-Path -LiteralPath (Join-Path $JdkHome 'bin\java.exe') -PathType Leaf)) {
    throw "JDK not found: $JdkHome"
}
if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf)) {
    throw "ROM file not found: $RomPath"
}

New-Item -ItemType Directory -Force -Path $projectDirectory | Out-Null
$env:JAVA_HOME = $JdkHome

Write-Host "Running Ghidra Headless Analysis for SNES NBA Live '95..." -ForegroundColor Cyan

& $headless $projectDirectory 'NbaLive95LoRom' `
    -import $RomPath `
    -overwrite `
    -loader BinaryLoader `
    -loader-baseAddr 0x0000 `
    -processor '65816:LE:24:default' `
    -scriptPath $PSScriptRoot `
    -preScript 'SnesEntryPoints.java'

if ($LASTEXITCODE -ne 0) {
    throw "Ghidra analysis exited with code $LASTEXITCODE"
}

Write-Host "Ghidra analysis complete in: $projectDirectory" -ForegroundColor Green
