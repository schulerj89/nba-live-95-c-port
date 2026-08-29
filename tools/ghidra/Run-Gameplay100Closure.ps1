[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$GhidraHome,
    [Parameter(Mandatory = $true)][string]$JdkHome
)

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $root 'ghidra-projects'
$analysisRoot = Join-Path $root '.analysis'
$outputDirectory = Join-Path $analysisRoot 'gameplay100-closure-ghidra'
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
foreach ($path in @($headless, $RomPath, $analysisRoot)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required analysis input not found: $path"
    }
}
New-Item -ItemType Directory -Force -Path $projectDirectory,$outputDirectory | Out-Null
$env:JAVA_HOME = $JdkHome
$romBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))

foreach ($bank in @('00', '80', '81', '82', '83', '84')) {
    $bankOffset = (([Convert]::ToInt32($bank, 16) -band 0x7F) * 0x8000)
    $bankPath = Join-Path $outputDirectory ("bank_{0}.bin" -f $bank)
    $bankBytes = New-Object byte[] 0x8000
    [Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
    [IO.File]::WriteAllBytes($bankPath, $bankBytes)
    & $headless $projectDirectory ("NbaLive95Gameplay100Closure{0}" -f $bank) `
        -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
        -postScript 'DumpGameplay100Closure.java' $outputDirectory $bank $analysisRoot
    if ($LASTEXITCODE -ne 0) {
        throw "Gameplay-100 Ghidra closure failed for bank $bank"
    }
}

Write-Host "Gameplay-100 closure written to: $outputDirectory" -ForegroundColor Green
