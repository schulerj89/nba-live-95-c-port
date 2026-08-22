[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$GhidraHome,
    [Parameter(Mandatory = $true)][string]$JdkHome
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $projectRoot 'ghidra-projects'
$analysisDirectory = Join-Path $projectRoot '.analysis\gameplay_players_ghidra'
$animationTrace = Join-Path $projectRoot '.analysis\player-animation-discovery\player_exec_trace.txt'
$tracePath = if (Test-Path -LiteralPath $animationTrace) { $animationTrace } else {
    Join-Path $projectRoot '.analysis\player-obj-source-trace\player_exec_trace.txt'
}
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
if (-not (Test-Path -LiteralPath $headless -PathType Leaf)) { throw "Ghidra headless not found: $headless" }
if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf)) { throw "ROM not found: $RomPath" }
New-Item -ItemType Directory -Force -Path $projectDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $analysisDirectory | Out-Null
$env:JAVA_HOME = $JdkHome
$romBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))

foreach ($bank in @('80', '84', '85', '86', '87')) {
    $bankOffset = (([Convert]::ToInt32($bank, 16) -band 0x7F) * 0x8000)
    $bankPath = Join-Path $analysisDirectory ("bank_{0}.bin" -f $bank)
    $bankBytes = New-Object byte[] 0x8000
    [Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
    [IO.File]::WriteAllBytes($bankPath, $bankBytes)
    & $headless $projectDirectory ("NbaLive95GameplayPlayers{0}" -f $bank) `
        -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
        -postScript 'DumpGameplayPlayers.java' $analysisDirectory $bank $tracePath
    if ($LASTEXITCODE -ne 0) { throw "Ghidra failed for bank $bank (exit $LASTEXITCODE)" }
}

Write-Host "Gameplay player analysis written to: $analysisDirectory" -ForegroundColor Green
