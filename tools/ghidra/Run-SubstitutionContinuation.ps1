[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$GhidraHome,
    [Parameter(Mandatory = $true)][string]$JdkHome,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectRoot '.analysis\substitution-continuation-ghidra'
}
$projectDirectory = Join-Path $projectRoot 'ghidra-projects'
$headless = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
if (-not (Test-Path -LiteralPath $headless -PathType Leaf)) { throw "Ghidra headless not found: $headless" }
if (-not (Test-Path -LiteralPath $RomPath -PathType Leaf)) { throw "ROM not found: $RomPath" }
New-Item -ItemType Directory -Force -Path $projectDirectory,$OutputDirectory | Out-Null
$env:JAVA_HOME = $JdkHome
$romBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))
foreach ($bank in @('83','85','86','87')) {
    $bankOffset = (([Convert]::ToInt32($bank, 16) -band 0x7f) * 0x8000)
    $bankPath = Join-Path $OutputDirectory ("bank_{0}.bin" -f $bank)
    $bankBytes = New-Object byte[] 0x8000
    [Array]::Copy($romBytes, $bankOffset, $bankBytes, 0, $bankBytes.Length)
    [IO.File]::WriteAllBytes($bankPath, $bankBytes)
    & $headless $projectDirectory ("NbaLive95Substitution{0}" -f $bank) `
        -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
        -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
        -postScript 'DumpSubstitutionContinuation.java' $OutputDirectory $bank
    if ($LASTEXITCODE -ne 0) { throw "Ghidra substitution dump failed for bank $bank" }
}
Write-Host "Substitution continuation dump written to: $OutputDirectory" -ForegroundColor Green
