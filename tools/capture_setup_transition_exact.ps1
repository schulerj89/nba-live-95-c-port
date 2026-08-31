param(
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$MesenPath,
    [Parameter(Mandatory=$true)][string]$OutputRoot,
    [ValidateSet('rules','options')][string]$Menu = 'rules',
    [switch]$SimulationThreeMinute,
    [switch]$HoldMenu,
    [switch]$RepeatVisit,
    [switch]$ResourcePublications,
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
if ($ResourcePublications -and ($Menu -ne 'rules' -or !$SimulationThreeMinute)) {
    throw 'ResourcePublications currently requires the normalized Rules journey.'
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$out = if ([IO.Path]::IsPathRooted($OutputRoot)) {
    [IO.Path]::GetFullPath($OutputRoot)
} else { [IO.Path]::GetFullPath((Join-Path $root $OutputRoot)) }
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
        resource_publications=[bool]$ResourcePublications;
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
# Each native process receives its own environment dictionary. Codex tool
# PowerShell runspaces can share the parent process environment; mutating
# $env:NBA95_* races another capture even when Mesen executables are private.
$startInfo = New-Object Diagnostics.ProcessStartInfo
$startInfo.FileName = $mesen
$startInfo.Arguments = $arguments -join ' '
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
foreach ($name in @($startInfo.EnvironmentVariables.Keys)) {
    if ($name.StartsWith('NBA95_')) { $startInfo.EnvironmentVariables.Remove($name) }
}
$startInfo.EnvironmentVariables['NBA95_CAPTURE_DIR']=$out.Replace('\','/')
$startInfo.EnvironmentVariables['NBA95_CAPTURE_MENU']=$Menu
$startInfo.EnvironmentVariables['NBA95_CAPTURE_EVERY_FRAME']='1'
if ($SimulationThreeMinute) { $startInfo.EnvironmentVariables['NBA95_CAPTURE_CANONICAL_UI']='1' }
if ($HoldMenu) { $startInfo.EnvironmentVariables['NBA95_CAPTURE_HOLD_MENU']='1' }
if ($RepeatVisit) { $startInfo.EnvironmentVariables['NBA95_CAPTURE_REPEAT_VISIT']='1' }
if ($ResourcePublications) { $startInfo.EnvironmentVariables['NBA95_CAPTURE_RESOURCE_PUBLICATIONS']='1' }
if ($TargetRow -ge 0) {
    $startInfo.EnvironmentVariables['NBA95_CAPTURE_TARGET_ROW']=[string]$TargetRow
    $startInfo.EnvironmentVariables['NBA95_CAPTURE_TARGET_RIGHTS']=[string]$TargetRights
}
$manifest.isolation['environment']='private ProcessStartInfo environment; no parent environment mutation'
$manifest | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $out 'manifest.json')
$process = New-Object Diagnostics.Process
$process.StartInfo = $startInfo
try {
    if (!$process.Start()) { throw 'Could not start isolated native capture.' }
    $stdoutTask=$process.StandardOutput.ReadToEndAsync()
    $stderrTask=$process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit(200000)) {
        $process.Kill()
        throw 'Native capture exceeded the bounded 200-second process limit.'
    }
    [IO.File]::WriteAllText((Join-Path $out 'stdout.log'),$stdoutTask.GetAwaiter().GetResult())
    [IO.File]::WriteAllText((Join-Path $out 'stderr.log'),$stderrTask.GetAwaiter().GetResult())
    if($process.ExitCode -ne 0 -or !(Test-Path (Join-Path $out 'capture_complete.txt'))) {
        throw "Native capture incomplete (exit $($process.ExitCode))."
    }
    $expectedEnvironment=@(
        ('directory='+$out.Replace('\','/')),('menu='+$Menu),'every_frame=true',
        ('canonical_ui='+([string][bool]$SimulationThreeMinute).ToLowerInvariant()),
        ('hold_menu='+([string][bool]$HoldMenu).ToLowerInvariant()),
        ('repeat_visit='+([string][bool]$RepeatVisit).ToLowerInvariant()),
        ('target_row='+$TargetRow),('target_rights='+$TargetRights))
    if ($ResourcePublications) { $expectedEnvironment += 'resource_publications=true' }
    $observedEnvironment=@(Get-Content -LiteralPath (Join-Path $out 'capture_environment.txt'))
    if (($observedEnvironment -join "`n") -cne ($expectedEnvironment -join "`n")) {
        throw 'Native Lua environment does not match this declared capture folder/profile.'
    }
    $manifest.isolation['observed_environment']=$observedEnvironment
    # A2BF is an audio-loop tail also executed in the unskipped title
    # sequence; it is only the historical capture label anchor. Require the actual Rules/Options handler to have executed,
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
        'wram_before_open.bin','wram_open.bin','wram_after_back.bin','dispatch_ppu_states.txt','capture_environment.txt','capture_complete.txt')
    if ($ResourcePublications) { $artifactNames += @('resource_jobs.csv','resource_writes.csv') }
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
    $process.Dispose()
}
Write-Host "Exact $Menu capture complete: $out"
