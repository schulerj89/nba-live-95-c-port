[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$OutputRoot,
    [int]$Calls = 400,
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc'
)
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $OutputRoot) {
    throw "Output root must be new: $OutputRoot"
}

$driver = (Resolve-Path "$PSScriptRoot\mesen_func_vectors.lua").Path
$mesen = (Get-Command Mesen.exe).Source
$specs = @(
    @{ Label='gameplay-resource-allocator'; Entry='80A34E'; Exits='80A3B1,80A443' },
    @{ Label='gameplay-resource-slot-select'; Entry='80A444'; Exits='80A4AC,80A4B4' },
    @{ Label='gameplay-screen-axis'; Entry='80A732'; Exits='80A75D' },
    @{ Label='gameplay-screen-axis-clamp'; Entry='80A75E'; Exits='80A77B,80A780' },
    @{ Label='gameplay-world-project'; Entry='80A781'; Exits='80A7C4' }
)

foreach ($spec in $specs) {
    $dir = Join-Path $OutputRoot $spec.Label
    New-Item -ItemType Directory -Force $dir | Out-Null
    $env:NBA95_CAPTURE_DIR = (Resolve-Path $dir).Path
    $env:NBA95_VEC_ENTRY = $spec.Entry
    $env:NBA95_VEC_EXITS = $spec.Exits
    $env:NBA95_VEC_READS = '0000-07FF'
    $env:NBA95_VEC_WRITES = '0000-07FF'
    $env:NBA95_VEC_LABEL = $spec.Label
    $env:NBA95_VEC_MAX = "$Calls"
    $env:NBA95_VEC_DRIVE = '1'
    $env:NBA95_CPU_VS_CPU = '1'
    $env:NBA95_VEC_FRAMES = '6000'
    $env:NBA95_VEC_DELAY = '0'
    $arguments = @('--testrunner','--timeout=300',
        ('"' + $RomPath.Replace('\','/') + '"'), ('"' + $driver + '"'))
    $process = Start-Process -FilePath $mesen -ArgumentList $arguments `
        -PassThru -WindowStyle Hidden
    $process.WaitForExit()
    $sentinel = Join-Path $dir 'capture_complete.txt'
    if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $sentinel)) {
        throw "Presentation capture failed for $($spec.Label): $dir"
    }
    Get-Content $sentinel
}
