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
$analysisDirectory = Join-Path $projectRoot '.analysis\cpu_gameplay_ghidra'
if (-not $CaptureDirectory) {
    $CaptureDirectory = Join-Path $projectRoot '.analysis\cpu-ai-extended-20260823'
}
$liveTrace = Join-Path $CaptureDirectory 'exec_live.txt'
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
if (-not (Test-Path -LiteralPath $headless -PathType Leaf)) { throw "Ghidra headless not found: $headless" }
if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf)) { throw "ROM not found: $RomPath" }
if (-not (Test-Path -LiteralPath $liveTrace -PathType Leaf)) { throw "Extended live trace not found: $liveTrace" }
New-Item -ItemType Directory -Force -Path $projectDirectory,$analysisDirectory | Out-Null
$env:JAVA_HOME = $JdkHome
$romBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))

foreach ($bank in @('83', '85', '86', '87')) {
    $bankOffset = (([Convert]::ToInt32($bank, 16) -band 0x7F) * 0x8000)
    $bankPath = Join-Path $analysisDirectory ("bank_{0}.bin" -f $bank)
    $bankBytes = New-Object byte[] 0x8000
    [Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
    [IO.File]::WriteAllBytes($bankPath, $bankBytes)
    & $headless $projectDirectory ("NbaLive95CpuGameplay{0}" -f $bank) `
        -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
        -postScript 'DumpCpuGameplay.java' $analysisDirectory $bank $liveTrace
    if ($LASTEXITCODE -ne 0) { throw "Ghidra CPU gameplay analysis failed for bank $bank" }
}

Write-Host "CPU gameplay analysis written to: $analysisDirectory" -ForegroundColor Green
