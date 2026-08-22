param(
    [Parameter(Mandatory = $true)]
    [string]$RomPath,
    [string]$MesenPath = '',
    [string]$AnalysisPath = '',
    [ValidateSet('all', 'intro_capture', 'title_capture', 'setup_capture',
                 'setup_transition', 'setup_rules', 'setup_options',
                 'setup_option_values', 'setup_main', 'team_select_logos')]
    [string]$CaptureName = 'all'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Analysis = if ([string]::IsNullOrEmpty($AnalysisPath)) {
    Join-Path $Root '.analysis'
} else {
    $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($AnalysisPath)
}

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
    @{ Name = 'setup_transition'; Script = 'mesen_setup_transition_capture.lua' },
    @{ Name = 'setup_rules'; Script = 'mesen_setup_menus_capture.lua';
       Env = @{ NBA95_CAPTURE_MENU = 'rules'; NBA95_CAPTURE_SCROLL = '1' };
       Required = @('menu_vram.bin', 'menu_cgram.bin', 'menu_oam.bin',
                    'open_transition_vram.bin', 'open_transition_cgram.bin',
                    'open_transition_vram_writes.txt',
                    'open_transition_ppu_states.txt',
                    'return_transition_vram.bin', 'return_transition_cgram.bin',
                    'return_transition_vram_writes.txt',
                    'return_transition_ppu_states.txt') },
    @{ Name = 'setup_options'; Script = 'mesen_setup_menus_capture.lua';
       Env = @{ NBA95_CAPTURE_MENU = 'options' };
       Required = @('menu_vram.bin', 'menu_cgram.bin', 'menu_oam.bin',
                    'open_transition_vram.bin',
                    'open_transition_cgram.bin', 'open_transition_vram_writes.txt',
                    'open_transition_ppu_states.txt',
                    'return_transition_vram.bin', 'return_transition_cgram.bin',
                    'return_transition_vram_writes.txt',
                    'return_transition_ppu_states.txt') },
    @{ Name = 'setup_option_values'; Script = 'mesen_setup_menus_capture.lua';
       Env = @{ NBA95_CAPTURE_MENU = 'options'; NBA95_CAPTURE_VALUES = '1' };
       Required = @('menu_vram.bin', 'menu_cgram.bin',
                    'options_mode_off_vram.bin', 'options_mode_mono_vram.bin',
                    'options_crowd_off_vram.bin', 'options_slow_on_vram.bin',
                    'options_shot_cpu_vram.bin',
                    'options_assistance_on_vram.bin') },
    @{ Name = 'setup_main'; Script = 'mesen_setup_main_capture.lua';
       Required = @('row0_step1_vram.bin', 'row0_step2_vram.bin',
                    'row0_step3_vram.bin', 'row1_step1_vram.bin',
                    'row1_step2_vram.bin', 'row2_step1_vram.bin',
                    'row2_step2_vram.bin', 'row3_step1_vram.bin',
                    'row3_step2_vram.bin', 'row3_step3_vram.bin') },
    @{ Name = 'team_select_logos'; Script = 'mesen_team_select_capture.lua';
       Env = @{ NBA95_TEAM_CONFIRM = 'start'; NBA95_TEAM_LOGOS = '1';
                NBA95_TEAM_LOGO_STEP = '90' };
       Required = @(0..28 | ForEach-Object {
           @("team_{0:D2}_vram.bin" -f $_, "team_{0:D2}_cgram.bin" -f $_,
             "team_{0:D2}_oam.bin" -f $_)
       }) }
)
if ($CaptureName -ne 'all') {
    $Captures = @($Captures | Where-Object Name -eq $CaptureName)
}

try {
foreach ($Capture in $Captures) {
    $Output = Join-Path $Analysis $Capture.Name
    New-Item -ItemType Directory -Force -Path $Output | Out-Null
    $env:NBA95_CAPTURE_DIR = $Output -replace '\\', '/'
    foreach ($Name in @('NBA95_CAPTURE_MENU', 'NBA95_CAPTURE_SCROLL',
                         'NBA95_CAPTURE_VARIANTS', 'NBA95_CAPTURE_VALUES',
                         'NBA95_CAPTURE_CALLS', 'NBA95_TEAM_CONFIRM',
                         'NBA95_TEAM_LOGOS', 'NBA95_TEAM_LOGO_STEP')) {
        [Environment]::SetEnvironmentVariable($Name, $null, 'Process')
    }
    if ($Capture.Env) {
        foreach ($Pair in $Capture.Env.GetEnumerator()) {
            [Environment]::SetEnvironmentVariable($Pair.Key, $Pair.Value, 'Process')
        }
    }
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
    if ($Capture.Required) {
        foreach ($RelativePath in $Capture.Required) {
            $RequiredPath = Join-Path $Output $RelativePath
            if (!(Test-Path -LiteralPath $RequiredPath) -or
                (Get-Item -LiteralPath $RequiredPath).Length -eq 0) {
                throw "Mesen capture $($Capture.Name) did not produce $RelativePath."
            }
            $ExpectedLength = switch -Regex ($RelativePath) {
                '_vram\.bin$' { 0x10000; break }
                '_cgram\.bin$' { 0x200; break }
                '_oam\.bin$' { 0x220; break }
                default { 0 }
            }
            $ActualLength = (Get-Item -LiteralPath $RequiredPath).Length
            if ($ExpectedLength -ne 0 -and $ActualLength -ne $ExpectedLength) {
                throw "Mesen capture $($Capture.Name) produced $RelativePath with " +
                      "$ActualLength bytes; expected $ExpectedLength."
            }
        }
    }
}
} finally {
    Remove-Item Env:NBA95_CAPTURE_DIR -ErrorAction SilentlyContinue
    Remove-Item Env:NBA95_CAPTURE_MENU,Env:NBA95_CAPTURE_SCROLL,Env:NBA95_CAPTURE_VARIANTS,Env:NBA95_CAPTURE_VALUES,Env:NBA95_CAPTURE_CALLS,Env:NBA95_TEAM_CONFIRM,Env:NBA95_TEAM_LOGOS,Env:NBA95_TEAM_LOGO_STEP -ErrorAction SilentlyContinue
}
Write-Host 'All asset captures completed.' -ForegroundColor Green
