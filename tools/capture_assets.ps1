param(
    [Parameter(Mandatory = $true)]
    [string]$RomPath,
    [string]$MesenPath = '',
    [ValidateSet('all', 'intro_capture', 'title_capture', 'setup_capture', 'setup_transition')]
    [string]$CaptureName = 'all'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Analysis = Join-Path $Root '.analysis'

if (!(Test-Path -LiteralPath $RomPath)) {
    throw "ROM not found: $RomPath"
}
if ([string]::IsNullOrEmpty($MesenPath)) {
    $MesenCommand = Get-Command Mesen.exe -ErrorAction SilentlyContinue
    if ($MesenCommand) {
        $MesenPath = $MesenCommand.Source
    }
}
if ([string]::IsNullOrEmpty($MesenPath) -or !(Test-Path -LiteralPath $MesenPath)) {
    throw 'Mesen.exe was not found. Pass -MesenPath explicitly.'
}

$Captures = @(
    @{ Name = 'intro_capture'; Script = 'mesen_intro_capture.lua' },
    @{ Name = 'title_capture'; Script = 'mesen_title_capture.lua' },
    @{ Name = 'setup_capture'; Script = 'mesen_setup_capture.lua' },
    @{ Name = 'setup_transition'; Script = 'mesen_setup_transition_capture.lua' }
)
if ($CaptureName -ne 'all') {
    $Captures = @($Captures | Where-Object Name -eq $CaptureName)
}

foreach ($Capture in $Captures) {
    $Output = Join-Path $Analysis $Capture.Name
    New-Item -ItemType Directory -Force -Path $Output | Out-Null
    $env:NBA95_CAPTURE_DIR = $Output -replace '\\', '/'
    $Script = Join-Path (Join-Path $Root 'tools') $Capture.Script
    $Complete = Join-Path $Output 'capture_complete.txt'
    Remove-Item -LiteralPath $Complete -ErrorAction SilentlyContinue
    Write-Host "Capturing $($Capture.Name)..." -ForegroundColor Cyan
    $Process = Start-Process -FilePath $MesenPath `
        -ArgumentList @("`"$RomPath`"", "`"$Script`"") `
        -PassThru -WindowStyle Hidden
    $Deadline = (Get-Date).AddMinutes(5)
    while (!(Test-Path -LiteralPath $Complete) -and !$Process.HasExited -and
           (Get-Date) -lt $Deadline) {
        Start-Sleep -Milliseconds 250
        $Process.Refresh()
    }
    if (!$Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
    }
    if (!(Test-Path -LiteralPath $Complete)) {
        throw "Mesen capture $($Capture.Name) did not produce its completion marker."
    }
}

Remove-Item Env:NBA95_CAPTURE_DIR -ErrorAction SilentlyContinue
Write-Host 'All asset captures completed.' -ForegroundColor Green
