[CmdletBinding()]
param(
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$GhidraHome = 'C:\Users\joshs\Downloads\ghidra_11.3_PUBLIC_20250205\ghidra_11.3_PUBLIC',
    [string]$JdkHome = 'C:\Users\joshs\Downloads\jdk-21\jdk-21.0.6+7',
    [int]$Bank = 0x82
)

$ErrorActionPreference = 'Stop'

$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $projectRoot 'ghidra-projects'
$analysisDirectory = Join-Path $projectRoot '.analysis'
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
New-Item -ItemType Directory -Force -Path $analysisDirectory | Out-Null
$env:JAVA_HOME = $JdkHome

# Ghidra's available 65816 language uses a 16-bit address space. Import one
# 32 KiB LoROM bank at $8000 so CPU addresses and branch/call targets remain
# readable. This also avoids analyzeHeadless.bat misparsing ROM paths that
# contain parentheses (the canonical dump is named "(USA).sfc").
$bankIndex = $Bank -band 0x7F
$bankOffset = $bankIndex * 0x8000
$bankPath = Join-Path $analysisDirectory ('bank_{0:X2}.bin' -f $Bank)
$romBytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))
if ($bankOffset + 0x8000 -gt $romBytes.Length) {
    throw ('ROM is too small to contain LoROM bank ${0:X2}' -f $Bank)
}
$bankBytes = New-Object byte[] 0x8000
[Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
[System.IO.File]::WriteAllBytes($bankPath, $bankBytes)

Write-Host "Running Ghidra Headless Analysis for SNES NBA Live '95..." -ForegroundColor Cyan

$projectName = 'NbaLive95Bank{0:X2}' -f $Bank
if ($Bank -eq 0x80) {
    & $headless $projectDirectory $projectName `
        -import $bankPath `
        -overwrite `
        -loader BinaryLoader `
        -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' `
        -scriptPath $PSScriptRoot `
        -noanalysis `
        -postScript 'DumpBank80IntroHelpers.java' $analysisDirectory
} elseif ($Bank -eq 0x87) {
    & $headless $projectDirectory $projectName `
        -import $bankPath `
        -overwrite `
        -loader BinaryLoader `
        -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' `
        -scriptPath $PSScriptRoot `
        -noanalysis `
        -postScript 'DumpBank87Title.java' $analysisDirectory
} else {
    & $headless $projectDirectory $projectName `
        -import $bankPath `
        -overwrite `
        -loader BinaryLoader `
        -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' `
        -scriptPath $PSScriptRoot `
        -preScript 'SnesEntryPoints.java' `
        -noanalysis `
        -postScript 'DumpEaIntro.java' $analysisDirectory
}

if ($LASTEXITCODE -ne 0) {
    throw "Ghidra analysis exited with code $LASTEXITCODE"
}

Write-Host "Ghidra analysis complete in: $projectDirectory" -ForegroundColor Green
if ($Bank -eq 0x80) {
    Write-Host "EA intro helper dump written to: $analysisDirectory\ea_intro_bank80_helpers.txt" -ForegroundColor Green
} elseif ($Bank -eq 0x87) {
    Write-Host "Post-EA title/audio dump written to: $analysisDirectory\post_ea_bank87.txt" -ForegroundColor Green
} else {
    Write-Host "EA intro dump written to: $analysisDirectory\ea_intro_ghidra.txt" -ForegroundColor Green
}
