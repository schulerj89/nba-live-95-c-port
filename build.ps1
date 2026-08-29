param(
    [string]$RomPath = '',
    [string]$AssetPack = '',
    [switch]$ExtractAssets,
    [switch]$Run,
    [switch]$Headless,
    [switch]$Test,
    [int]$Frames = 30,
    [string]$DumpFrame = ''
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $Root "build"
$ObjDir = Join-Path $BuildDir "obj"
$DefaultAssetPack = Join-Path $BuildDir "nba95_assets.pak"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

# Auto-extract asset pack if requested or if it does not exist and ROM is present
if ($ExtractAssets -or (![string]::IsNullOrEmpty($RomPath) -and (Test-Path $RomPath) -and !(Test-Path $DefaultAssetPack) -and [string]::IsNullOrEmpty($AssetPack))) {
    Write-Host "Extracting assets from ROM to: $DefaultAssetPack..." -ForegroundColor Cyan
    $ExtractorScript = Join-Path $Root "tools\extract_assets.py"
    & python $ExtractorScript --rom $RomPath --output $DefaultAssetPack
    if ($LASTEXITCODE -ne 0) {
        throw "Asset extraction failed with exit code $LASTEXITCODE"
    }
}

if ([string]::IsNullOrEmpty($AssetPack) -and (Test-Path $DefaultAssetPack)) {
    $AssetPack = $DefaultAssetPack
}

$VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (!(Test-Path $VsWhere)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools with Desktop C++ workload."
}

$VsPath = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$VsPath) {
    throw "MSVC C++ tools were not found."
}

$VcVars = Join-Path $VsPath "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $VcVars)) {
    throw "vcvars64.bat was not found under $VsPath."
}

$ConsoleExePath = Join-Path $BuildDir "nba95_port.exe"

$SourceManifest = Join-Path $Root "nba95_sources.txt"
$Sources = Get-Content -LiteralPath $SourceManifest | ForEach-Object { $_.Trim() } |
    Where-Object { $_ -and !$_.StartsWith('#') }

$IncludePath = Join-Path $Root "include"

$CompileScript = Join-Path $BuildDir "compile.bat"
$CompileBatchContent = @"
@echo off
call "$VcVars" > nul
echo Compiling nba95_port.exe (CLI/Test runner)...
cl.exe /nologo /W4 /O2 /MD /utf-8 /I "$IncludePath" /Fe"$ConsoleExePath" /Fo"$ObjDir\\" $(($Sources | ForEach-Object { "`"$Root\$($_ -replace '/', '\')`"" }) -join ' ') user32.lib gdi32.lib winmm.lib
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
"@

Set-Content -Path $CompileScript -Value $CompileBatchContent -Encoding ASCII
Write-Host "Building executables..." -ForegroundColor Cyan
& cmd.exe /c $CompileScript

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

Write-Host "Build complete:" -ForegroundColor Green
Write-Host "  Executable: $ConsoleExePath"

if ($Test) {
    if ([string]::IsNullOrEmpty($RomPath) -or !(Test-Path -LiteralPath $RomPath)) {
        throw "-Test requires -RomPath."
    }
    if ([string]::IsNullOrEmpty($AssetPack) -or !(Test-Path -LiteralPath $AssetPack)) {
        throw "-Test requires a generated asset pack."
    }
    & python (Join-Path $Root 'tools\test_differential.py')
    if ($LASTEXITCODE -ne 0) { throw 'Strict differential harness unit tests failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name differential_observer_probe
    & (Join-Path $BuildDir 'differential_observer_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Differential observer changed gameplay or missed real sweep boundaries.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name ball_init_differential_probe
    foreach ($InitFixture in @('ball-initialization', 'ball-initialization-poisoned')) {
        & python (Join-Path $Root 'tools\verify_ball_init_differential.py') `
            --fixture (Join-Path $Root "tests\fixtures\$InitFixture.json") `
            --probe (Join-Path $BuildDir 'ball_init_differential_probe.exe') `
            --report (Join-Path $BuildDir "$InitFixture-report.json")
        if ($LASTEXITCODE -ne 0) { throw "Native ball initialization differential test failed: $InitFixture" }
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name ball_init_runtime_probe
    & (Join-Path $BuildDir 'ball_init_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Native ball initialization runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name jump_reach_probe
    & python (Join-Path $Root 'tools\test_jump_reach.py') --pack $AssetPack --rom $RomPath `
        --probe (Join-Path $BuildDir 'jump_reach_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Jump/reach native decision/channel regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name reach_launch_probe
    & python (Join-Path $Root 'tools\verify_reach_launch.py') `
        --native (Join-Path $Root 'tests\fixtures\reach-launch-witnesses.jsonl') `
        --probe (Join-Path $BuildDir 'reach_launch_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Jump/reach EAA8 near-child replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name graphics_scratch_probe
    & python (Join-Path $Root 'tools\verify_graphics_scratch.py') `
        --native (Join-Path $Root 'tests\fixtures\graphics-scratch-witnesses.jsonl') `
        --probe (Join-Path $BuildDir 'graphics_scratch_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Jump/reach graphics-scratch scheduler replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name jump_runtime_probe
    & (Join-Path $BuildDir 'jump_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Jump/reach production runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name defensive_pose_vector_probe
    & python (Join-Path $Root 'tools\verify_defensive_pose_vectors.py') --normalized `
        --vectors (Join-Path $Root 'tests\fixtures\defensive-pose-witnesses.json') `
        --probe (Join-Path $BuildDir 'defensive_pose_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Defensive idle/pose native replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name defensive_pose_runtime_probe
    & (Join-Path $BuildDir 'defensive_pose_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Defensive idle/pose runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name active_appearance_vector_probe
    & python (Join-Path $Root 'tools\verify_active_appearance_vectors.py') --normalized `
        --vectors (Join-Path $Root 'tests\fixtures\active-appearance-witnesses.json') `
        --probe (Join-Path $BuildDir 'active_appearance_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Active appearance-record native replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name active_appearance_runtime_probe
    & (Join-Path $BuildDir 'active_appearance_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Active appearance-record runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name sprite_compositor_probe
    & (Join-Path $BuildDir 'sprite_compositor_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Sprite compositor native replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name animation_render_closure_probe
    & (Join-Path $BuildDir 'animation_render_closure_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Animation render closure failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name jersey_number_vector_probe
    & python (Join-Path $Root 'tools\verify_jersey_number_vectors.py') --normalized `
        --vectors (Join-Path $Root 'tests\fixtures\jersey-number-witnesses.json') `
        --probe (Join-Path $BuildDir 'jersey_number_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Ten-player jersey-number ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name camera_handoff_probe
    & python (Join-Path $Root 'tools\verify_camera_handoff.py') --require-census `
        --vectors (Join-Path $Root 'tests\fixtures\camera-handoff-witnesses.json') `
        --probe (Join-Path $BuildDir 'camera_handoff_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Complete camera ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name camera_handoff_runtime_probe
    & (Join-Path $BuildDir 'camera_handoff_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Camera handoff runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name court_presentation_probe
    & python (Join-Path $Root 'tools\verify_court_presentation.py') --require-census `
        --vectors (Join-Path $Root 'tests\fixtures\camera-presentation-witnesses.json') `
        --probe (Join-Path $BuildDir 'court_presentation_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Camera/presentation ROM replay failed.' }
    & python (Join-Path $Root 'tools\test_court_presentation.py') --pack $AssetPack --rom $RomPath
    if ($LASTEXITCODE -ne 0) { throw 'Court panorama ROM asset checks failed.' }
    & python (Join-Path $Root 'tools\test_snes_mode1.py') `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) { throw 'SNES Mode-1 compositor regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name bank80_services_probe
    & (Join-Path $BuildDir 'bank80_services_probe.exe')
    if ($LASTEXITCODE -ne 0) {
        throw 'Bank $80 NMI/PPU/APU/controller host-service gate failed.'
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name gameplay55_services_probe
    & (Join-Path $BuildDir 'gameplay55_services_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) {
        throw 'Bank $80 gameplay presentation/resource/timing service gate failed.'
    }
    $Gameplay55AssetArgs = @('--pack', $AssetPack)
    $Gameplay55NativeDir = Join-Path $Root '.analysis\ppu-runtime-final-20260829'
    if (Test-Path -LiteralPath (Join-Path $Gameplay55NativeDir 'scanout_0989_vram.bin')) {
        $Gameplay55AssetArgs += @('--native-dir', $Gameplay55NativeDir)
    }
    & python (Join-Path $Root 'tools\test_gameplay55_assets.py') $Gameplay55AssetArgs
    if ($LASTEXITCODE -ne 0) {
        throw 'Bank $80 gameplay asset publication integrity gate failed.'
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name gameplay65_flow_probe
    & (Join-Path $BuildDir 'gameplay65_flow_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) {
        throw 'Bank $82/$83 Team Select through lineup production-flow gate failed.'
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name court_runtime_probe
    & (Join-Path $BuildDir 'court_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Court runtime integration failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name special_shot_vector_probe
    & python (Join-Path $Root 'tools\verify_special_shot_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\special-shot-witnesses.json') `
        --probe (Join-Path $BuildDir 'special_shot_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Special-shot ROM witness regression failed.' }
    & python (Join-Path $Root 'tools\verify_special_shot_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\natural-shot-selection-witnesses.json') `
        --probe (Join-Path $BuildDir 'special_shot_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Natural selector ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name shot_state_vector_probe
    & python (Join-Path $Root 'tools\verify_shot_state_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\shot-state-witnesses.json') `
        --probe (Join-Path $BuildDir 'shot_state_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Shot-state ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name shot_state_runtime_probe
    & (Join-Path $BuildDir 'shot_state_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Shot-state runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name match_lifecycle_probe
    & (Join-Path $BuildDir 'match_lifecycle_probe.exe') $AssetPack $RomPath
    if ($LASTEXITCODE -ne 0) { throw 'Match lifecycle initialization binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name match_lifecycle_expiry_probe
    & (Join-Path $BuildDir 'match_lifecycle_expiry_probe.exe') `
        (Join-Path $Root 'tests\fixtures\match-lifecycle-expiry-witnesses.json')
    if ($LASTEXITCODE -ne 0) { throw 'Match lifecycle expiry native replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name timeout_resume_runtime_probe
    & (Join-Path $BuildDir 'timeout_resume_runtime_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Timeout/resume runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name complete_shot_vector_probe
    & python (Join-Path $Root 'tools\verify_complete_shot_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\complete-shot-witnesses.json') `
        --probe (Join-Path $BuildDir 'complete_shot_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Complete launch ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name close_finish_vector_probe
    & python (Join-Path $Root 'tools\verify_close_finish_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\close-finish-witnesses.json') `
        --probe (Join-Path $BuildDir 'close_finish_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Close-finish ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name close_finish_runtime_probe
    & (Join-Path $BuildDir 'close_finish_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Close-finish runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name matchup_helper_vector_probe
    & python (Join-Path $Root 'tools\verify_matchup_helper_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\matchup-helper-witnesses.json') `
        --probe (Join-Path $BuildDir 'matchup_helper_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Defensive matchup-helper ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name matchup_helper_runtime_probe
    & (Join-Path $BuildDir 'matchup_helper_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Defensive matchup-helper runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name pass_init_vector_probe
    & python (Join-Path $Root 'tools\verify_pass_init_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\pass-init-witnesses.json') `
        --probe (Join-Path $BuildDir 'pass_init_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'CPU pass initialization ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name pass_release_vector_probe
    & python (Join-Path $Root 'tools\verify_pass_release_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\pass-release-witnesses.json') `
        --probe (Join-Path $BuildDir 'pass_release_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'CPU grounded pass-release ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name player_contact_sweep_vector_probe
    & python (Join-Path $Root 'tools\verify_player_contact_sweep_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\player-contact-sweep-witnesses.json') `
        --probe (Join-Path $BuildDir 'player_contact_sweep_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Player contact-sweep ROM replay failed.' }
    & python (Join-Path $Root 'tools\verify_ball_contact_sweep_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\ball-contact-sweep-witnesses.json') `
        --probe (Join-Path $BuildDir 'player_contact_sweep_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Ball contact-sweep ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name defense_target_family_probe
    & python (Join-Path $Root 'tools\verify_defense_target_families.py') `
        --vectors (Join-Path $Root 'tests\fixtures\defense-target-family-witnesses.json') `
        --probe (Join-Path $BuildDir 'defense_target_family_probe.exe') --rom $RomPath
    if ($LASTEXITCODE -ne 0) { throw 'Defensive target-family ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name play_request_vector_probe
    & python (Join-Path $Root 'tools\verify_play_request_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\play-request-witnesses.json') `
        --probe (Join-Path $BuildDir 'play_request_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Play-request dispatcher ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name formation_route_vector_probe
    & python (Join-Path $Root 'tools\verify_formation_route_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\formation-route-witnesses.json') `
        --probe (Join-Path $BuildDir 'formation_route_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Formation-route ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name mode11_parent_vector_probe
    & python (Join-Path $Root 'tools\verify_mode11_parent_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\mode11-parent-witnesses.json') `
        --probe (Join-Path $BuildDir 'mode11_parent_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Mode-11 parent dispatcher ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name normal_actor_parent_vector_probe
    & python (Join-Path $Root 'tools\verify_normal_actor_parent_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\normal-actor-parent-witnesses.json') `
        --probe (Join-Path $BuildDir 'normal_actor_parent_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Normal CPU actor-parent ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name requested_direction_vector_probe
    & python (Join-Path $Root 'tools\verify_requested_direction_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\requested-direction-witnesses.json') `
        --probe (Join-Path $BuildDir 'requested_direction_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'CPU requested-direction ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name violation_parent_vector_probe
    & python (Join-Path $Root 'tools\verify_violation_parent_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\violation-parent-witnesses.json') `
        --probe (Join-Path $BuildDir 'violation_parent_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Violation parent dispatcher ROM replay failed.' }
    & python (Join-Path $Root 'tools\test_shot_assets.py') --pack $AssetPack --rom $RomPath --exe $ConsoleExePath
    if ($LASTEXITCODE -ne 0) { throw 'Shot asset ROM comparison failed.' }
    & python (Join-Path $Root 'tools\test_special_shot_integration.py') --pack $AssetPack --rom $RomPath --exe $ConsoleExePath
    if ($LASTEXITCODE -ne 0) { throw 'Special-shot gameplay integration failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name action_animation_vector_probe
    & python (Join-Path $Root 'tools\verify_action_animation_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\action-animation-witnesses.json') `
        --probe (Join-Path $BuildDir 'action_animation_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Action/animation ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name owner_pose_animation_vector_probe
    & python (Join-Path $Root 'tools\verify_owner_pose_animation_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\owner-pose-animation-witnesses.json') `
        --probe (Join-Path $BuildDir 'owner_pose_animation_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Owner pose/animation ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name owner_flow_vector_probe
    & python (Join-Path $Root 'tools\verify_owner_flow_vectors.py') `
        --normalized --require-census --vectors (Join-Path $Root 'tests\fixtures\owner-flow-witnesses.json') `
        --probe (Join-Path $BuildDir 'owner_flow_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Owner flow/reversal/idle ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name inbound_motion_vector_probe
    & python (Join-Path $Root 'tools\verify_inbound_motion_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\inbound-motion-witnesses.json') `
        --probe (Join-Path $BuildDir 'inbound_motion_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Inbound continuation ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name inbound_arrival_vector_probe
    & python (Join-Path $Root 'tools\verify_inbound_arrival_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\inbound-arrival-witnesses.json') `
        --probe (Join-Path $BuildDir 'inbound_arrival_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Inbound arrival ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name inbound_selector_vector_probe
    & python (Join-Path $Root 'tools\verify_inbound_selector_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\inbound-selector-witnesses.json') `
        --probe (Join-Path $BuildDir 'inbound_selector_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Inbound selector ROM witness regression failed.' }
    & python (Join-Path $Root 'tools\verify_inbound_alternate.py') `
        --vectors (Join-Path $Root 'tests\fixtures\inbound-alternate-witnesses.json') `
        --probe (Join-Path $BuildDir 'inbound_selector_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Alternate inbound-selector ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name human_inbound_direction_probe
    & python (Join-Path $Root 'tools\verify_human_inbound_direction.py') `
        --vectors (Join-Path $Root 'tests\fixtures\human-inbound-direction-witnesses.json') `
        --probe (Join-Path $BuildDir 'human_inbound_direction_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Human inbound-direction ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name dead_ball_reset_probe
    & python (Join-Path $Root 'tools\verify_dead_ball_reset.py') `
        --vectors (Join-Path $Root 'tests\fixtures\dead-ball-reset-witnesses.json') `
        --probe (Join-Path $BuildDir 'dead_ball_reset_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Common dead-ball reset ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name foul_classifier_vector_probe
    & python (Join-Path $Root 'tools\verify_foul_classifier_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\foul-classifier-witnesses.json') `
        --probe (Join-Path $BuildDir 'foul_classifier_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Contact-foul classifier ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name foul_bookkeeping_vector_probe
    & python (Join-Path $Root 'tools\verify_foul_bookkeeping_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\foul-bookkeeping-witnesses.json') `
        --probe (Join-Path $BuildDir 'foul_bookkeeping_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Foul/stat bookkeeping ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name foul_consume_vector_probe
    & python (Join-Path $Root 'tools\verify_foul_consume_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\foul-consume-witnesses.json') `
        --probe (Join-Path $BuildDir 'foul_consume_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Pending-foul consumer ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name free_throw_completion_vector_probe
    & python (Join-Path $Root 'tools\verify_free_throw_completion_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\free-throw-completion-witnesses.json') `
        --probe (Join-Path $BuildDir 'free_throw_completion_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'CPU free-throw completion ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name draw_direction_vector_probe
    & python (Join-Path $Root 'tools\verify_draw_direction_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\draw-direction-witnesses.json') `
        --probe (Join-Path $BuildDir 'draw_direction_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Player draw-direction ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name draw_preparation_vector_probe
    & python (Join-Path $Root 'tools\verify_draw_preparation_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\draw-preparation-witnesses.json') `
        --probe (Join-Path $BuildDir 'draw_preparation_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Player draw-preparation ROM witness regression failed.' }
    & python (Join-Path $Root 'tools\verify_player_draw_pipeline.py') `
        --whole (Join-Path $Root 'tests\fixtures\player-draw-whole-witnesses.json') `
        --preparation (Join-Path $Root 'tests\fixtures\draw-preparation-witnesses.json') `
        --direction (Join-Path $Root 'tests\fixtures\draw-direction-witnesses.json')
    if ($LASTEXITCODE -ne 0) { throw 'Whole player-draw pipeline census regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name draw_indicator_vector_probe
    & python (Join-Path $Root 'tools\verify_draw_indicator_vectors.py') `
        --vectors (Join-Path $Root 'tests\fixtures\draw-indicator-witnesses.json') `
        --probe (Join-Path $BuildDir 'draw_indicator_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Human edge-indicator ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name raw_sprite_compositor_probe
    & python (Join-Path $Root 'tools\verify_raw_sprite_compositor.py') `
        --vectors (Join-Path $Root 'tests\fixtures\raw-sprite-compositor-witnesses.json') `
        --probe (Join-Path $BuildDir 'raw_sprite_compositor_probe.exe') `
        --assets $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Raw ROM sprite compositor regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name appearance_upload_runtime_probe
    & python (Join-Path $Root 'tools\verify_appearance_upload.py') `
        --vectors (Join-Path $Root 'tests\fixtures\appearance-upload-witness.json') `
        --probe (Join-Path $BuildDir 'appearance_upload_runtime_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Gameplay appearance-load ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name owner_pose_runtime_probe
    & (Join-Path $BuildDir 'owner_pose_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Owner pose/natural-special runtime regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name dynamic_dribble_attachment_probe
    & (Join-Path $BuildDir 'dynamic_dribble_attachment_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Dynamic dribble ball-attachment ordering regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name action_pose_vector_probe
    & python (Join-Path $Root 'tools\verify_action_pose_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\action-pose-witnesses.json') `
        --probe (Join-Path $BuildDir 'action_pose_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Action pose/appearance ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name shot_action_vector_probe
    & python (Join-Path $Root 'tools\verify_shot_action_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\shot-action-witnesses.json') `
        --probe (Join-Path $BuildDir 'shot_action_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Shot action ROM witness regression failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name shot_branch_vector_probe
    & python (Join-Path $Root 'tools\verify_shot_branch_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\shot-branch-witnesses.json') `
        --probe (Join-Path $BuildDir 'shot_branch_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Shot branch ROM witness regression failed.' }
    & python (Join-Path $Root "tools\test_title_pipeline.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Title regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_setup_transition.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Game Setup transition regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_core_safety.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Core safety regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_team_select.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Team Select regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_player_setup.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Player Setup regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_player_lab.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Player Lab regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_formation_assets.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Gameplay formation asset tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_player_intro.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Player Introduction regression tests failed with exit code $LASTEXITCODE"
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_contact_probe
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_completion_probe
    & python (Join-Path $Root 'tools\verify_tip_completion.py') --normalized `
        --vectors (Join-Path $Root 'tests\fixtures\tip-completion.json') --probe (Join-Path $BuildDir 'tip_completion_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Tip completion native replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name ball_acquisition_vector_probe
    & python (Join-Path $Root 'tools\verify_tip_acquisition.py') --normalized --pack $AssetPack `
        --vectors (Join-Path $Root 'tests\fixtures\tip-acquisition.json') --probe (Join-Path $BuildDir 'ball_acquisition_vector_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Tip acquisition/caller native replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_possession_runtime_probe
    & (Join-Path $BuildDir 'tip_possession_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Tip possession runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_launch_probe
    & python (Join-Path $Root 'tools\verify_tip_launch.py') --normalized --pack $AssetPack `
        --vectors (Join-Path $Root 'tests\fixtures\tip-launch.json') --probe (Join-Path $BuildDir 'tip_launch_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Tip launch ROM replay failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_launch_runtime_probe
    & (Join-Path $BuildDir 'tip_launch_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Tip launch runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_receiver_probe
    & python (Join-Path $Root 'tools\verify_tip_receiver.py') --normalized `
        --vectors (Join-Path $Root 'tests\fixtures\tip-receiver.json') --probe (Join-Path $BuildDir 'tip_receiver_probe.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Tip receiver ROM replay failed.' }
    & (Join-Path $BuildDir 'tip_receiver_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Tip receiver runtime binding failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_flow_endurance_probe
    & (Join-Path $BuildDir 'tip_flow_endurance_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Tip flow endurance failed.' }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name gameplay85_endurance_probe
    & (Join-Path $BuildDir 'gameplay85_endurance_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) {
        throw 'Multi-team gameplay-85 state/render endurance failed.'
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name gameplay100_closure_probe
    & (Join-Path $BuildDir 'gameplay100_closure_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) {
        throw 'Captured-address closure production journey failed.'
    }
    foreach ($witness in @('tip-contact-natural.json', 'tip-contact-controlled.json')) {
        & python (Join-Path $Root 'tools\verify_tip_contact.py') --normalized `
            --vectors (Join-Path $Root "tests\fixtures\$witness") --probe (Join-Path $BuildDir 'tip_contact_probe.exe')
        if ($LASTEXITCODE -ne 0) { throw 'Tip contact ROM replay failed.' }
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name tip_contact_runtime_probe
    & (Join-Path $BuildDir 'tip_contact_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Tip contact runtime binding failed.' }
    & python (Join-Path $Root "tools\test_tipoff.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Tip-off regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_gameplay_audio.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Gameplay audio regression tests failed with exit code $LASTEXITCODE"
    }
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name gameplay_audio_variant_probe
    & (Join-Path $BuildDir 'gameplay_audio_variant_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) {
        throw 'Gameplay audio command/RNG/DSP vectors failed.'
    }
    & python (Join-Path $Root "tools\test_gameplay_debugger.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Gameplay Lab regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_cpu_gameplay.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "CPU gameplay regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_intro_sequence.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Intro sequence regression tests failed with exit code $LASTEXITCODE"
    }
    & python (Join-Path $Root "tools\test_project_census.py")
    if ($LASTEXITCODE -ne 0) {
        throw "Project census regression tests failed with exit code $LASTEXITCODE"
    }
}

if ($Headless -or $DumpFrame) {
    Write-Host "Running headless verification..." -ForegroundColor Yellow
    $argsList = @('--headless', '--rom', $RomPath, '--frames', $Frames)
    if (![string]::IsNullOrEmpty($AssetPack)) {
        $argsList += @('--assets', $AssetPack)
    }
    if ($DumpFrame) {
        $argsList += @('--dump-frame', $DumpFrame)
    }
    & $ConsoleExePath $argsList
    if ($LASTEXITCODE -ne 0) {
        throw "Headless verification failed with exit code $LASTEXITCODE"
    }
} elseif ($Run) {
    Write-Host "Launching NBA Live '95 C Port..." -ForegroundColor Cyan
    $runArgs = @('--rom', $RomPath)
    if (![string]::IsNullOrEmpty($AssetPack)) {
        $runArgs += @('--assets', $AssetPack)
    }
    & $ConsoleExePath $runArgs
}
