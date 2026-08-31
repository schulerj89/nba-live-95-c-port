[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
      [ValidateSet('presets','rules','options','load','held','main')][string]$Journey='presets',
      [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc',
      [string]$SaveSource)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw 'Capture directory must be new.'}
New-Item -ItemType Directory -Path $OutputDir|Out-Null
$capture=(Resolve-Path -LiteralPath $OutputDir).Path
$saves=Join-Path $capture 'isolated-saves'
$runtime=Join-Path $capture 'portable-mesen'
New-Item -ItemType Directory -Path $saves,$runtime|Out-Null
$installed=(Get-Command Mesen.exe).Source
$mesen=Join-Path $runtime 'Mesen.exe'
Copy-Item -LiteralPath $installed -Destination $mesen
$initialSaves=@()
if($SaveSource){
    $seed=(Resolve-Path -LiteralPath $SaveSource).Path
    if((Get-Item -LiteralPath $seed).Length -ne 8192){throw 'Native SRAM seed must be8192 bytes.'}
    $destination=Join-Path $saves ([IO.Path]::GetFileNameWithoutExtension($RomPath)+'.srm')
    Copy-Item -LiteralPath $seed -Destination $destination
    $initialSaves=@([ordered]@{path=$seed;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $seed).Hash.ToLowerInvariant()})
}
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
$rom=(Resolve-Path -LiteralPath $RomPath).Path
$script=Join-Path $capture 'capture.lua'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'mesen_setup_config.lua') -Destination $script
$runner=Join-Path $capture 'capture-runner.ps1'
Copy-Item -LiteralPath $PSCommandPath -Destination $runner
$arguments=@('--testrunner','--timeout=240',('"'+$rom+'"'),('"'+$script+'"'))
$manifest=[ordered]@{classification='natural controller-only configuration journey';
    journey=$Journey;cpu_state_injection=$false;rom_patch=$false;wram_injection=$false;
    sram_injection=$false;default_input_pulse_frames=3;ordinary_action_period=60;
    transition_action_period=260;arguments=$arguments;
    initial_save_files=$initialSaves;
    isolation=[ordered]@{method='private portable executable/settings';home=$runtime;
        save_folder=$saves;settings=$settings};sources=[ordered]@{}}
foreach($pair in @(@('rom',$rom),@('mesen',$mesen),@('script',$script),@('settings',$settingsPath),@('runner',$runner))){
    $manifest.sources[$pair[0]]=[ordered]@{path=$pair[1];sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $pair[1]).Hash.ToLowerInvariant()}
}
$old=@{}
foreach($name in @('NBA95_CAPTURE_DIR','NBA95_CONFIG_JOURNEY')){
    $old[$name]=[Environment]::GetEnvironmentVariable($name,'Process')
}
try{
    $env:NBA95_CAPTURE_DIR=$capture.Replace('\','/')
    $env:NBA95_CONFIG_JOURNEY=$Journey
    $manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $capture 'manifest.json') -Encoding utf8
    $process=Start-Process -FilePath $mesen -ArgumentList $arguments -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $capture 'stdout.log') `
        -RedirectStandardError (Join-Path $capture 'stderr.log')
    $processHandle=$process.Handle
    $process.WaitForExit()
    $manifest['process_exit_code']=$process.ExitCode
    $manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $capture 'manifest.json') -Encoding utf8
    if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath (Join-Path $capture 'capture_complete.txt'))){
        throw "Incomplete configuration capture (exit $($process.ExitCode)): $capture"
    }
    $observed=(Get-Content -Raw -LiteralPath (Join-Path $capture 'observed-script-data-folder.txt')).Trim()
    $expected=[IO.Path]::GetFullPath((Join-Path $runtime 'LuaScriptData'))+[IO.Path]::DirectorySeparatorChar
    if(![IO.Path]::GetFullPath($observed).StartsWith($expected,[StringComparison]::OrdinalIgnoreCase)){
        throw "Unexpected Mesen home: $observed"
    }
    if((Get-FileHash -Algorithm SHA256 -LiteralPath $settingsPath).Hash.ToLowerInvariant() -ne $manifest.sources.settings.sha256){
        throw 'Portable settings changed during capture.'
    }
    $manifest.isolation['observed_script_data_folder']=$observed
    $manifest.isolation['post_settings_verified']=$true
    $manifest['captured_utc']=[DateTime]::UtcNow.ToString('o')
    foreach($name in @('actions.json','action-states.jsonl','events.jsonl')){
        $path=Join-Path $capture $name
        $manifest.sources[$name]=[ordered]@{path=$path;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()}
    }
    $manifest['final_save_files']=@(Get-ChildItem -LiteralPath $saves -File|ForEach-Object{
        [ordered]@{path=$_.FullName;size=$_.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
    })
    $manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $capture 'manifest.json') -Encoding utf8
    Get-Content -LiteralPath (Join-Path $capture 'capture_complete.txt')
}finally{
    foreach($name in $old.Keys){[Environment]::SetEnvironmentVariable($name,$old[$name],'Process')}
}
