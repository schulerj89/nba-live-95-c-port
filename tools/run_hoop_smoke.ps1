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
& (Join-Path $PSScriptRoot 'build_vector_probe.ps1') -Name hoop_raster_probe
if ($LASTEXITCODE -ne 0) { throw 'Raster probe build failed.' }
$smokeArgs = @((Join-Path $PSScriptRoot 'test_hoop_smoke.py'), '--rom', $RomPath)
if ($OutputDirectory) { $smokeArgs += @('--output', $OutputDirectory) }
if ($ThroughMenus) { $smokeArgs += '--through-menus' }
& python @smokeArgs
if ($LASTEXITCODE -ne 0) { throw 'Hoop smoke failed; see its retained output directory.' }
