[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
      [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw 'Capture directory must be new.'}
New-Item -ItemType Directory -Path $OutputDir|Out-Null
$capture=(Resolve-Path -LiteralPath $OutputDir).Path
$saves=Join-Path $capture 'isolated-saves'
New-Item -ItemType Directory -Path $saves|Out-Null
$installedMesen=(Get-Command Mesen.exe).Source
$runtime=Join-Path $capture 'portable-mesen'
New-Item -ItemType Directory -Path $runtime|Out-Null
$mesen=Join-Path $runtime 'Mesen.exe'
Copy-Item -LiteralPath $installedMesen -Destination $mesen
$settings=[ordered]@{
    Debug=[ordered]@{ScriptWindow=[ordered]@{AllowIoOsAccess=$true;ScriptTimeout=60;SaveScriptBeforeRun=$false}}
    Preferences=[ordered]@{SingleInstance=$false;PauseWhenInBackground=$false;
        AutoLoadPatches=$false;OverrideSaveDataFolder=$true;SaveDataFolder=$saves}
    Snes=[ordered]@{Port1=[ordered]@{Type='SnesController'};Port2=[ordered]@{Type='None'};
        DisableFrameSkipping=$true;EnableRandomPowerOnState=$false;RamPowerOnState='AllZeros';
        ForceFixedResolution=$false;Overscan=[ordered]@{Top=7;Bottom=8;Left=0;Right=0}}
    Video=[ordered]@{VideoFilter='None';AspectRatio='NoStretching';Brightness=0;Contrast=0;
        Hue=0;Saturation=0;ScanlineIntensity=0;UseBilinearInterpolation=$false;ScreenRotation='None'}
}
$settingsPath=Join-Path $runtime 'settings.json'
$settings|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $settingsPath -Encoding utf8
$settingsInitialHash=(Get-FileHash -Algorithm SHA256 -LiteralPath $settingsPath).Hash.ToLowerInvariant()
$rom=(Resolve-Path -LiteralPath $RomPath).Path
$script=Join-Path $PSScriptRoot 'mesen_ppu_brightness.lua'
$arguments=@('--testrunner','--timeout=180',('"'+$rom+'"'),('"'+$script+'"'))
$old=[Environment]::GetEnvironmentVariable('NBA95_CAPTURE_DIR','Process')
try {
    $env:NBA95_CAPTURE_DIR=$capture.Replace('\','/')
    $process=Start-Process -FilePath $mesen -ArgumentList $arguments -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $capture 'stdout.log') `
        -RedirectStandardError (Join-Path $capture 'stderr.log')
    $process.WaitForExit()
    if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath (Join-Path $capture 'capture_complete.txt'))){
        throw "Incomplete brightness raster capture: $capture"
    }
    $observedFolder=(Get-Content -Raw -LiteralPath (Join-Path $capture 'observed-script-data-folder.txt')).Trim()
    $expectedDataRoot=[IO.Path]::GetFullPath((Join-Path $runtime 'LuaScriptData'))+[IO.Path]::DirectorySeparatorChar
    if (![IO.Path]::GetFullPath($observedFolder).StartsWith($expectedDataRoot,[StringComparison]::OrdinalIgnoreCase)) {
        throw "Mesen did not use the private portable home: $observedFolder"
    }
    $loaded=Get-Content -Raw -LiteralPath $settingsPath|ConvertFrom-Json
    foreach($group in @('Video','Snes','Preferences')) {
        foreach($key in $settings[$group].Keys) {
            if ($settings[$group][$key] -is [System.Collections.IDictionary]) {continue}
            if ($loaded.$group.$key -ne $settings[$group][$key]) {
                throw "Portable settings changed or were not honored: $group.$key"
            }
        }
    }
    $manifest=[ordered]@{
        oracle='original ROM executing in Mesen, controlled PPU raster';calls=1536;
        captured_utc=[DateTime]::UtcNow.ToString('o');arguments=$arguments;
        injections=@('PPU $2100,$212C,$212D,$2130,$2131,$2133','CGRAM[0]');
        cpu_state_injection=$false;rom_patch=$false;wram_injection=$false;
        color_math='disabled';sample_points=@(@(0,7),@(255,7),@(128,119),@(0,230),@(255,230));
        retry_policy='repeat identical input only if actual PPU/CGRAM differs, a conflicting native hardware write occurred, or sampled raster is nonuniform; never compare expected RGB';
        rejection_reason_bits=@{forced_blank=1;brightness=2;main_layers=4;sub_layers=8;cgram=16;nonuniform_samples=32;conflicting_native_write=64};
        isolation=[ordered]@{method='private portable executable/settings';home=$runtime;
            observed_script_data_folder=$observedFolder;save_folder=$saves;settings=$settings;
            initial_settings_sha256=$settingsInitialHash;post_settings_verified=$true};
        rejected_attempts=@(Get-Content -LiteralPath (Join-Path $capture 'rejected-attempts.jsonl')).Count;
        sources=[ordered]@{}
    }
    foreach($pair in @(@('rom',$rom),@('mesen',$mesen),@('script',$script),@('settings',$settingsPath),@('trace',(Join-Path $capture 'brightness.jsonl')),@('rejected',(Join-Path $capture 'rejected-attempts.jsonl')),@('native_writes',(Join-Path $capture 'native-hardware-writes.jsonl')))){
        $manifest.sources[$pair[0]]=[ordered]@{path=$pair[1];sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $pair[1]).Hash.ToLowerInvariant()}
    }
    $manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $capture 'manifest.json') -Encoding utf8
    Get-Content -LiteralPath (Join-Path $capture 'capture_complete.txt')
}finally{
    [Environment]::SetEnvironmentVariable('NBA95_CAPTURE_DIR',$old,'Process')
}
