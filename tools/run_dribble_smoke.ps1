param(
    [Parameter(Mandatory=$true)][string]$RomPath,
    [string]$OutputDirectory = '',
    [switch]$NoBuild,
    [switch]$TipoffOnly
)
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$NoBuild) {
    & (Join-Path $repoRoot 'build.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Game build failed.' }
}
foreach ($probe in @('ball_driver_owned_vector_probe', 'dribble_draw_vector_probe')) {
    & (Join-Path $PSScriptRoot 'build_vector_probe.ps1') -Name $probe
    if ($LASTEXITCODE -ne 0) { throw "Probe build failed: $probe" }
}
$smokeArgs = @((Join-Path $PSScriptRoot 'test_dribble_smoke.py'), '--rom', $RomPath)
if ($OutputDirectory) { $smokeArgs += @('--output', $OutputDirectory) }
if ($TipoffOnly) { $smokeArgs += '--tipoff-only' }
& python @smokeArgs
if ($LASTEXITCODE -ne 0) { throw 'Dribble smoke failed; see its retained output directory.' }
