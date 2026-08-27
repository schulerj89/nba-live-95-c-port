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
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name complete_shot_vector_probe
    & python (Join-Path $Root 'tools\verify_complete_shot_vectors.py') `
        --normalized --vectors (Join-Path $Root 'tests\fixtures\complete-shot-witnesses.json') `
        --probe (Join-Path $BuildDir 'complete_shot_vector_probe.exe') --pack $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Complete launch ROM witness regression failed.' }
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
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name owner_pose_runtime_probe
    & (Join-Path $BuildDir 'owner_pose_runtime_probe.exe') $AssetPack
    if ($LASTEXITCODE -ne 0) { throw 'Owner pose/natural-special runtime regression failed.' }
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
    & python (Join-Path $Root "tools\test_tipoff.py") `
        --pack $AssetPack --exe $ConsoleExePath --rom $RomPath
    if ($LASTEXITCODE -ne 0) {
        throw "Tip-off regression tests failed with exit code $LASTEXITCODE"
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
