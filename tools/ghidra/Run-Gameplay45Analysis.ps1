[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$GhidraHome,
    [Parameter(Mandatory = $true)][string]$JdkHome,
    [string]$CaptureDirectory
)

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectDirectory = Join-Path $root 'ghidra-projects'
$analysisDirectory = Join-Path $root '.analysis\gameplay45\ghidra'
if (-not $CaptureDirectory) {
    $CaptureDirectory = Join-Path $root '.analysis\cpu-ai-extended-20260823'
}
$liveTrace = Join-Path $CaptureDirectory 'exec_live.txt'
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
foreach ($path in @($headless, $RomPath, $liveTrace)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required analysis input not found: $path"
    }
}
New-Item -ItemType Directory -Force -Path $projectDirectory,$analysisDirectory | Out-Null
$env:JAVA_HOME = $JdkHome
$rom = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))
$bank = New-Object byte[] 0x8000
[Array]::Copy($rom, 0, $bank, 0, $bank.Length)
$bankPath = Join-Path $analysisDirectory 'bank_80.bin'
[IO.File]::WriteAllBytes($bankPath, $bank)
& $headless $projectDirectory 'NbaLive95Gameplay45Bank80' `
    -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
    -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
    -postScript 'DumpGameplay45.java' $analysisDirectory $liveTrace
if ($LASTEXITCODE -ne 0) { throw 'Ghidra gameplay-45 analysis failed' }
& $headless $projectDirectory 'NbaLive95Gameplay45Bank80Services' `
    -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
    -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
    -postScript 'DumpGameplay45Services.java' $analysisDirectory $liveTrace
if ($LASTEXITCODE -ne 0) { throw 'Ghidra gameplay-45 service analysis failed' }
Write-Host "Gameplay-45 analysis written to: $analysisDirectory" -ForegroundColor Green
