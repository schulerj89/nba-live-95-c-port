param(
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$MesenPath,
    [Parameter(Mandatory=$true)][string]$OutputRoot,
    [ValidateSet('rules','options')][string]$Menu = 'rules',
    [switch]$SimulationThreeMinute,
    [switch]$HoldMenu,
    [switch]$RepeatVisit,
    [ValidateRange(-1,12)][int]$TargetRow = -1,
    [ValidateRange(0,2)][int]$TargetRights = 0
)
$ErrorActionPreference = 'Stop'
if ($TargetRow -lt 0 -and $TargetRights -ne 0) { throw 'TargetRights requires TargetRow.' }
if ($TargetRow -ge 0 -and ($HoldMenu -or $Menu -ne 'rules')) {
    throw 'TargetRow is a Rules UI snapshot journey and cannot be combined with HoldMenu.'
}
if ($TargetRow -ge 0 -and $TargetRow + $TargetRights -gt 13) {
    throw 'The final target pulse must finish before the native753 snapshot.'
}
if ($RepeatVisit -and ($Menu -ne 'rules' -or $TargetRow -ne 2 -or $TargetRights -ne 1)) {
    throw 'RepeatVisit currently specifies the bounded Rules row2/right1 journey.'
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$out = [IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
if (Test-Path -LiteralPath $out) { throw 'OutputRoot must be a new directory.' }
New-Item -ItemType Directory -Path $out | Out-Null
$saves = Join-Path $out 'isolated-saves'
New-Item -ItemType Directory -Path $saves | Out-Null
$installedMesen = if ($MesenPath) {
    (Resolve-Path -LiteralPath $MesenPath -ErrorAction Stop).Path
} else { (Get-Command Mesen.exe -ErrorAction Stop).Source }
$runtime = Join-Path $out 'portable-mesen'
New-Item -ItemType Directory -Path $runtime | Out-Null
$mesen = Join-Path $runtime 'Mesen.exe'
Copy-Item -LiteralPath $installedMesen -Destination $mesen
# Mesen 2.1.1 ConfigManager.ApplySetting does not implement string switches;
# ProcessSwitch also rejects path separators/colon. A --saveDataFolder path
# therefore does not isolate SRAM. Portable settings next to this private
# executable select a genuinely separate home before ROM loading.
$settings = [ordered]@{
    Debug=[ordered]@{ScriptWindow=[ordered]@{AllowIoOsAccess=$true;
        ScriptTimeout=60;SaveScriptBeforeRun=$false}}
    Preferences=[ordered]@{SingleInstance=$false;PauseWhenInBackground=$false;
        AutoLoadPatches=$false;OverrideSaveDataFolder=$true;SaveDataFolder=$saves}
    Snes=[ordered]@{Port1=[ordered]@{Type='SnesController'};
        Port2=[ordered]@{Type='None'};
        DisableFrameSkipping=$true;EnableRandomPowerOnState=$false;
        RamPowerOnState='AllZeros';ForceFixedResolution=$false;
        Overscan=[ordered]@{Top=7;Bottom=8;Left=0;Right=0}}
    Video=[ordered]@{VideoFilter='None';AspectRatio='NoStretching';Brightness=0;
        Contrast=0;Hue=0;Saturation=0}
}
$settingsPath = Join-Path $runtime 'settings.json'
$settings | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $settingsPath -Encoding UTF8
$sourceScript = Join-Path $PSScriptRoot 'mesen_setup_menus_capture.lua'
$script = Join-Path $out 'capture.lua'
Copy-Item -LiteralPath $sourceScript -Destination $script
$arguments = @('--testrunner','--timeout=180',
    ('"'+[IO.Path]::GetFullPath($RomPath)+'"'),('"'+$script+'"'))
$manifest = [ordered]@{
    schema=2
    kind='natural-input frontend journey'; menu=$Menu
    configuration=[ordered]@{natural_ui_normalization=[bool]$SimulationThreeMinute;
        hold_menu_without_value_edits=[bool]$HoldMenu;
        repeat_visit=[bool]$RepeatVisit;
        target_row=$TargetRow;target_rights=$TargetRights;
        variant_schedule= $(if ($TargetRow -ge 0) {
            'Natural down pulses at620+10*n; right pulses after the final down. Snapshot753 validates selected row. This is an equivalent UI-state snapshot, not a claim of identical button timing to the C harness.'
        } else { 'none' });
        target= $(if ($SimulationThreeMinute) { 'Exhibition/Simulation/Rookie/3min via controller input' } else { 'fresh SRAM defaults' });
        normalization_schedule= $(if ($SimulationThreeMinute) {
            'Wait 400 native frames; down400,right460,down520,down580,right640,up700,up760,up820; harness evidence labels rebase to370 at normalization frame920. Emulator execution uninterrupted; no CPU/WRAM state injection.'
        } else { 'none' })}
    raw_rgb=[ordered]@{width=256;height=239;visible_first_row=7;visible_rows=224;
        source='emu.getScreenBuffer at endFrame';screenshots='asynchronous evidence only'}
    arguments=$arguments
    isolation=[ordered]@{method='private portable executable and settings.json';
        home=$runtime;save_folder=$saves;initial_save_files=@();
        settings=$settings;installed_executable=$installedMesen}
    sources=[ordered]@{}
    artifacts=[ordered]@{schema=1;files=[ordered]@{}}
}
foreach ($pair in @(@('rom',$RomPath),@('mesen',$mesen),@('capture',$script),
                    @('portable_settings',$settingsPath),@('capture_source',$sourceScript),
                    @('runner',$PSCommandPath))) {
    $manifest.sources[$pair[0]]=[ordered]@{path=[IO.Path]::GetFullPath($pair[1]);
        sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $pair[1]).Hash.ToLowerInvariant()}
}
$oldEnv=@{}
foreach($name in @('NBA95_CAPTURE_DIR','NBA95_CAPTURE_MENU','NBA95_CAPTURE_EVERY_FRAME',
                   'NBA95_CAPTURE_SCROLL','NBA95_CAPTURE_VARIANTS','NBA95_CAPTURE_VALUES',
                   'NBA95_CAPTURE_CALLS','NBA95_CAPTURE_CANONICAL_UI','NBA95_CAPTURE_HOLD_MENU',
                   'NBA95_CAPTURE_TARGET_ROW','NBA95_CAPTURE_TARGET_RIGHTS','NBA95_CAPTURE_REPEAT_VISIT')) {
    $oldEnv[$name]=[Environment]::GetEnvironmentVariable($name,'Process')
    [Environment]::SetEnvironmentVariable($name,$null,'Process')
}
try {
    $env:NBA95_CAPTURE_DIR=$out.Replace('\','/')
    $env:NBA95_CAPTURE_MENU=$Menu
    $env:NBA95_CAPTURE_EVERY_FRAME='1'
    if ($SimulationThreeMinute) { $env:NBA95_CAPTURE_CANONICAL_UI='1' }
    if ($HoldMenu) { $env:NBA95_CAPTURE_HOLD_MENU='1' }
    if ($RepeatVisit) { $env:NBA95_CAPTURE_REPEAT_VISIT='1' }
    if ($TargetRow -ge 0) {
        $env:NBA95_CAPTURE_TARGET_ROW=[string]$TargetRow
        $env:NBA95_CAPTURE_TARGET_RIGHTS=[string]$TargetRights
    }
    $manifest | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $out 'manifest.json')
    $process=Start-Process -FilePath $mesen -ArgumentList $arguments -PassThru -Wait `
        -WindowStyle Hidden -RedirectStandardOutput (Join-Path $out 'stdout.log') `
        -RedirectStandardError (Join-Path $out 'stderr.log')
    if($process.ExitCode -ne 0 -or !(Test-Path (Join-Path $out 'capture_complete.txt'))) {
        throw "Native capture incomplete (exit $($process.ExitCode))."
    }
    # A2BF is a shared builder: it can also execute in the unskipped title
    # sequence. Require the actual Rules/Options handler to have executed,
    # not just a frame counter and completion sentinel.
    $expectedHandler = if ($Menu -eq 'rules') { 0x81D318 } else { 0x828CD1 }
    $handlerObserved = $false
    foreach ($line in Get-Content -LiteralPath (Join-Path $out 'exec_trace.txt')) {
        if ($line -match '^([0-9A-Fa-f]{6})-([0-9A-Fa-f]{6})$') {
            $first=[Convert]::ToInt32($Matches[1],16)
            $last=[Convert]::ToInt32($Matches[2],16)
            if ($first -le $expectedHandler -and $expectedHandler -le $last) {
                $handlerObserved = $true
            }
        }
    }
    if (!$handlerObserved) { throw 'Capture did not execute the requested native menu handler.' }
    $before=[IO.File]::ReadAllBytes((Join-Path $out 'wram_before_open.bin'))
    $after=[IO.File]::ReadAllBytes((Join-Path $out 'wram_open.bin'))
    if ($before.Length -ne 0x20000 -or $after.Length -ne 0x20000) {
        throw 'Native WRAM snapshots must contain the complete 128 KiB.'
    }
    $working=@();$committed=@();$committedAfter=@()
    for ($index=0;$index -lt 4;$index++) {
        $working += [BitConverter]::ToUInt16($before,0x16FB+2*$index)
        $committed += [BitConverter]::ToUInt16($before,0x17AB+2*$index)
        $committedAfter += [BitConverter]::ToUInt16($after,0x17AB+2*$index)
    }
    # Edited main values remain in the working buffer until the menu handoff.
    # Rules/Options commit those values, then reuse the working buffer for
    # their own rows. Requiring a pre-handoff equality rejects real edits.
    if (($working -join ',') -ne ($committedAfter -join ',') -or
        $working[0] -gt 3 -or $working[1] -gt 2 -or
        $working[2] -gt 2 -or $working[3] -gt 3) {
        throw 'Capture did not start from a coherent native Game Setup page.'
    }
    if ($SimulationThreeMinute -and ($working -join ',') -ne '0,1,0,0') {
        throw 'Natural UI normalization did not reach Simulation / three-minute quarters.'
    }
    $manifest['result']=[ordered]@{exit_code=$process.ExitCode;save_files=@(
        Get-ChildItem -LiteralPath $saves -File | ForEach-Object {
            [ordered]@{name=$_.Name;size=$_.Length;
                sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
        });native_handler=('{0:X6}' -f $expectedHandler);
        transition_main_values=$working;committed_before_open=$committed;
        committed_after_open=$committedAfter}
    $artifactNames=@('rgb_state.csv','raster_registers.csv','exec_trace.txt','capture_log.txt',
        'wram_before_open.bin','wram_open.bin','wram_after_back.bin','dispatch_ppu_states.txt','capture_complete.txt')
    if ($RepeatVisit) {
        $artifactNames += @('wram_repeat_before_open.bin','wram_repeat_open.bin',
                           'wram_repeat_before_return.bin','wram_repeat_after_return.bin')
        foreach ($prefix in @('repeat_open','repeat_return')) {
            foreach ($suffix in @('vram.bin','cgram.bin','vram_writes.txt','cgram_writes.txt','ppu_states.txt')) {
                $artifactNames += "${prefix}_transition_${suffix}"
            }
        }
    }
    if ($TargetRow -ge 0) {
        $variantState=[IO.File]::ReadAllBytes((Join-Path $out 'wram_state753.bin'))
        if ($variantState.Length -ne 0x20000) { throw 'Variant snapshot must contain the complete 128 KiB.' }
        $selectedRow=[BitConverter]::ToUInt16($variantState,0x1693)
        if ($selectedRow -ne $TargetRow) { throw 'Natural target-row journey did not reach the requested row.' }
        $manifest.result['variant_snapshot']=[ordered]@{frame=753;selected_row=$selectedRow;
            recent_direction_timer=[BitConverter]::ToUInt16($variantState,0x163B);
            recent_direction_bits=[BitConverter]::ToUInt16($variantState,0x1759)}
        $artifactNames += 'wram_state753.bin'
    }
    foreach ($prefix in @('open','return')) {
        foreach ($suffix in @('vram.bin','cgram.bin','vram_writes.txt',
                             'cgram_writes.txt','ppu_states.txt')) {
            $artifactNames += "${prefix}_transition_${suffix}"
        }
    }
    foreach ($name in $artifactNames) {
        $artifact=Get-Item -LiteralPath (Join-Path $out $name) -ErrorAction Stop
        $manifest.artifacts.files[$name]=[ordered]@{bytes=$artifact.Length;
            sha256=(Get-FileHash -LiteralPath $artifact.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
    }
    $manifest | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $out 'manifest.json')
} finally {
    foreach($name in $oldEnv.Keys) {
        [Environment]::SetEnvironmentVariable($name,$oldEnv[$name],'Process')
    }
}
Write-Host "Exact $Menu capture complete: $out"
