param(
    [Parameter(Mandatory=$true)][string]$OutputRoot,
    [ValidateSet(0,1,2)][int]$Selection=2,
    [switch]$AlternateTeams,
    [ValidateRange(-1,1800)][int]$PauseAfterFrames=-1,
    [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$MesenPath
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$out=[IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
if(Test-Path -LiteralPath $out){throw 'OutputRoot must be a new directory.'}
$rom=(Resolve-Path -LiteralPath $RomPath).Path
if((Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant() -ne
    '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870') {
    throw 'Unexpected original ROM identity.'
}
New-Item -ItemType Directory -Path $out|Out-Null
$runtime=Join-Path $out 'portable-mesen';$saves=Join-Path $out 'isolated-saves'
New-Item -ItemType Directory -Path $runtime,$saves|Out-Null
$installed=if($MesenPath){(Resolve-Path -LiteralPath $MesenPath).Path}else{(Get-Command Mesen.exe).Source}
$mesen=Join-Path $runtime 'Mesen.exe';Copy-Item -LiteralPath $installed -Destination $mesen
$settings=[ordered]@{
    Debug=[ordered]@{ScriptWindow=[ordered]@{AllowIoOsAccess=$true;ScriptTimeout=60}}
    Preferences=[ordered]@{SingleInstance=$false;PauseWhenInBackground=$false;
        AutoLoadPatches=$false;OverrideSaveDataFolder=$true;SaveDataFolder=$saves}
    Snes=[ordered]@{Port1=[ordered]@{Type='SnesController'};Port2=[ordered]@{Type='None'};
        DisableFrameSkipping=$true;EnableRandomPowerOnState=$false;RamPowerOnState='AllZeros';
        ForceFixedResolution=$false;Overscan=[ordered]@{Top=7;Bottom=8;Left=0;Right=0}}
    Video=[ordered]@{VideoFilter='None';AspectRatio='NoStretching';Brightness=0;Contrast=0;Hue=0;Saturation=0}
}
$settingsFile=Join-Path $runtime 'settings.json'
$settings|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $settingsFile -Encoding utf8
$script=Join-Path $out 'capture.lua';Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'mesen_controller_ownership.lua') -Destination $script
$arguments=@('--testrunner','--timeout=180',('"'+$rom+'"'),('"'+$script+'"'))
$manifest=[ordered]@{schema=1;kind='natural controller-only Player Setup to gameplay journey';
    selection=$Selection;alternate_teams=[bool]$AlternateTeams;
    pause_after_frames=$PauseAfterFrames;state_injection=$false;rom_patch=$false;
    isolation=[ordered]@{home=$runtime;save_folder=$saves;initial_save_files=@();settings=$settings};
    input_schedule='Normal title/setup/team Start presses; on PlayerSetup wait350, left400 iftarget<2, left460 iftarget<1, Start700. Court right60..89,up110..139,B170..184,A230..232,Y300..319. All times are phase-local native frames.';
    variant_schedule='When AlternateTeams: Setup-local650right,700L,750right,850Start instead of650Start. When PauseAfterFrames>=0: court-localStart at thatframe; capture ends after native818D->81D2 publishes requesting-controller team08D2. It does not choose a menu item or claim timeout confirmation. No state injection.';
    arguments=$arguments;sources=[ordered]@{};artifacts=[ordered]@{}}
foreach($pair in @(@('rom',$rom),@('mesen',$mesen),@('capture',$script),@('settings',$settingsFile),@('runner',$PSCommandPath))){
    $manifest.sources[$pair[0]]=[ordered]@{path=$pair[1];sha256=(Get-FileHash -LiteralPath $pair[1] -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$old=@{}
foreach($name in @('NBA95_CAPTURE_DIR','NBA95_CONTROL_SELECTION','NBA95_CONTROL_TEAM_VARIANT','NBA95_CONTROL_PAUSE_AT')){$old[$name]=[Environment]::GetEnvironmentVariable($name,'Process')}
try{
    $env:NBA95_CAPTURE_DIR=$out.Replace('\','/');$env:NBA95_CONTROL_SELECTION=[string]$Selection
    $env:NBA95_CONTROL_TEAM_VARIANT=$(if($AlternateTeams){'1'}else{'0'})
    $env:NBA95_CONTROL_PAUSE_AT=[string]$PauseAfterFrames
    $manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $out 'manifest.json') -Encoding utf8
    $process=Start-Process -FilePath $mesen -ArgumentList $arguments -PassThru -Wait -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $out 'stdout.log') -RedirectStandardError (Join-Path $out 'stderr.log')
    if($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath (Join-Path $out 'capture_complete.txt'))){
        throw "Incomplete native ownership journey (exit $($process.ExitCode)); retained $out"
    }
    foreach($file in Get-ChildItem -LiteralPath $out -File){
        if($file.Name -eq 'manifest.json'){continue}
        $manifest.artifacts[$file.Name]=[ordered]@{bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
    }
    # ReadAllText returns a plain string. Windows PowerShell's Get-Content
    # carries provider metadata into ConvertTo-Json and can recursively walk
    # that metadata instead of serializing this small completion summary.
    $manifest['result']=[ordered]@{exit_code=[int]$process.ExitCode;
        summary=[IO.File]::ReadAllText((Join-Path $out 'capture_complete.txt'))}
    $manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $out 'manifest.json') -Encoding utf8
}finally{foreach($name in $old.Keys){[Environment]::SetEnvironmentVariable($name,$old[$name],'Process')}}
Write-Host "Natural controller ownership capture complete: $out"
