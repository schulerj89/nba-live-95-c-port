[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$GhidraHome,
    [Parameter(Mandatory = $true)][string]$JdkHome,
    [string]$CaptureDirectory
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $projectRoot 'ghidra-projects'
$analysisDirectory = Join-Path $projectRoot '.analysis\tipoff_ghidra'
if (-not $CaptureDirectory) {
    $CaptureDirectory = Join-Path $projectRoot '.analysis\tipoff-focused-20260823'
}
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
if (-not (Test-Path -LiteralPath $headless -PathType Leaf)) { throw "Ghidra headless not found: $headless" }
if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf)) { throw "ROM not found: $RomPath" }
$traces = @('formation', 'jump_ball', 'possession', 'live') | ForEach-Object {
    $path = Join-Path $CaptureDirectory ("exec_{0}.txt" -f $_)
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Mesen trace not found: $path" }
    $path
}
New-Item -ItemType Directory -Force -Path $projectDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $analysisDirectory | Out-Null
$env:JAVA_HOME = $JdkHome
$romBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))

foreach ($bank in @('80', '82', '85', '86', '87')) {
    $bankOffset = (([Convert]::ToInt32($bank, 16) -band 0x7F) * 0x8000)
    $bankPath = Join-Path $analysisDirectory ("bank_{0}.bin" -f $bank)
    $bankBytes = New-Object byte[] 0x8000
    [Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
    [IO.File]::WriteAllBytes($bankPath, $bankBytes)
    & $headless $projectDirectory ("NbaLive95Tipoff{0}" -f $bank) `
        -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
        -postScript 'DumpTipoff.java' $analysisDirectory $bank $traces[0] `
            $traces[1] $traces[2] $traces[3]
    if ($LASTEXITCODE -ne 0) { throw "Ghidra tip-off analysis failed for bank $bank" }
}

Write-Host "Tip-off analysis written to: $analysisDirectory" -ForegroundColor Green
