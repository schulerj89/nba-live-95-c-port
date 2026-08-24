[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$GhidraHome,
    [Parameter(Mandatory = $true)][string]$JdkHome
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $projectRoot 'ghidra-projects'
$analysisDirectory = Join-Path $projectRoot '.analysis\player_intro_ghidra'
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
if (-not (Test-Path -LiteralPath $headless -PathType Leaf)) {
    throw "Ghidra headless not found: $headless"
}
if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf)) {
    throw "ROM not found: $RomPath"
}
New-Item -ItemType Directory -Force -Path $projectDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $analysisDirectory | Out-Null
$env:JAVA_HOME = $JdkHome
$romBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))

foreach ($bank in @('80', '81', '83', '84', '86', '87')) {
    $bankOffset = (([Convert]::ToInt32($bank, 16) -band 0x7F) * 0x8000)
    $bankPath = Join-Path $analysisDirectory ("bank_{0}.bin" -f $bank)
    $bankBytes = New-Object byte[] 0x8000
    [Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
    [IO.File]::WriteAllBytes($bankPath, $bankBytes)
    & $headless $projectDirectory ("NbaLive95PlayerIntro{0}" -f $bank) `
        -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
        -postScript 'DumpPlayerIntroduction.java' $analysisDirectory $bank
    if ($LASTEXITCODE -ne 0) {
        throw "Ghidra failed for bank $bank (exit $LASTEXITCODE)"
    }
}

Write-Host "Player introduction analysis written to: $analysisDirectory" -ForegroundColor Green
