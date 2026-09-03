param(
    [Parameter(Mandatory=$true)][string]$RomPath,
    [string]$OutputDirectory = '',
    [switch]$NoBuild,
    [switch]$ThroughMenus
)
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$NoBuild) {
    & (Join-Path $repoRoot 'build.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Game build failed.' }
}
& (Join-Path $PSScriptRoot 'build_vector_probe.ps1') -Name oob_contract_probe
if ($LASTEXITCODE -ne 0) { throw 'OOB probe build failed.' }
$smokeArgs = @((Join-Path $PSScriptRoot 'test_oob_smoke.py'), '--rom', $RomPath)
if ($OutputDirectory) { $smokeArgs += @('--output', $OutputDirectory) }
if ($ThroughMenus) { $smokeArgs += '--through-menus' }
& python @smokeArgs
if ($LASTEXITCODE -ne 0) { throw 'Out-of-bounds smoke failed; see its retained output directory.' }
