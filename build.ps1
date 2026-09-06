param(
    [string]$RomPath = '',
    [string]$AssetPack = '',
    [string]$CaptureRoot = '',
    [string]$RecompRoot = '',
    [string]$OutputExe = '',
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
$NativeCaptureRoot = if ([string]::IsNullOrWhiteSpace($CaptureRoot)) {
    Join-Path $Root '.analysis'
} else {
    [IO.Path]::GetFullPath($CaptureRoot)
}
$NativeRecompRoot = if ([string]::IsNullOrWhiteSpace($RecompRoot)) {
    Join-Path (Split-Path $Root -Parent) 'NBA-Live-95-Recomp'
} else {
    [IO.Path]::GetFullPath($RecompRoot)
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

# Auto-extract asset pack if requested or if it does not exist and ROM is present
if ($ExtractAssets -or (![string]::IsNullOrEmpty($RomPath) -and (Test-Path $RomPath) -and !(Test-Path $DefaultAssetPack) -and [string]::IsNullOrEmpty($AssetPack))) {
    Write-Host "Extracting assets from ROM to: $DefaultAssetPack..." -ForegroundColor Cyan
    $ExtractorScript = Join-Path $Root "tools\extract_assets.py"
    & python $ExtractorScript --rom $RomPath --output $DefaultAssetPack --capture-root $NativeCaptureRoot
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

$ConsoleExePath = if ([string]::IsNullOrWhiteSpace($OutputExe)) {
    Join-Path $BuildDir "nba95_port.exe"
} else {
    [IO.Path]::GetFullPath($OutputExe)
}

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
        throw '-Test requires -RomPath.'
    }
    if ([string]::IsNullOrEmpty($AssetPack) -or !(Test-Path -LiteralPath $AssetPack)) {
        throw '-Test requires a generated asset pack.'
    }

    $TestRunDir = Join-Path $BuildDir ('test-runs/' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Force -Path $TestRunDir | Out-Null
    $RegressionTimings = [Collections.Generic.List[object]]::new()
    Write-Host "Test logs and timings: $TestRunDir"

    function Invoke-PythonRegression {
        param(
            [Parameter(Mandatory = $true)]
            [string]$Script,
            [string[]]$Arguments = @()
        )
        $ScriptPath = Join-Path $Root "tools\$Script"
        $Timer = [Diagnostics.Stopwatch]::StartNew()
        $ExitCode = -1
        try {
            & python $ScriptPath @Arguments 2>&1 |
                Tee-Object -FilePath (Join-Path $TestRunDir "$Script.log")
            $ExitCode = $LASTEXITCODE
            if ($ExitCode -ne 0) { throw "$Script failed with exit code $ExitCode" }
        } finally {
            $Timer.Stop()
            $RegressionTimings.Add([pscustomobject]@{
                script = $Script
                seconds = [Math]::Round($Timer.Elapsed.TotalSeconds, 3)
                exit_code = $ExitCode
            })
            ConvertTo-Json -InputObject @($RegressionTimings.ToArray()) |
                Set-Content -LiteralPath (Join-Path $TestRunDir 'timings.json') -Encoding UTF8
            Write-Host ("[TIMING] {0}: {1:N2}s (exit {2})" -f $Script, $Timer.Elapsed.TotalSeconds, $ExitCode)
        }
    }

    # Keep the default gate focused on maintained harnesses and complete
    # product routes. Historical one-routine probes remain available in Git.
    Invoke-PythonRegression -Script 'test_headless_input_integrity.py'
    Invoke-PythonRegression -Script 'test_differential.py'
    Invoke-PythonRegression -Script 'test_mesen_portable.py'
    Invoke-PythonRegression -Script 'test_setup_transition_integrity.py'
    Invoke-PythonRegression -Script 'test_jsonl_rows.py'
    Invoke-PythonRegression -Script 'test_cpu_trace_analyzer.py'

    $InputReportDir = Join-Path $BuildDir ('headless-input-' + [guid]::NewGuid().ToString('N'))
    Invoke-PythonRegression -Script 'test_headless_input.py' -Arguments @(
        '--exe', $ConsoleExePath, '--rom', $RomPath, '--pack', $AssetPack,
        '--output', $InputReportDir
    )

    # Initialize MSVC once for all probes; each still links the current objects.
    & (Join-Path $Root 'tools\build_vector_probe.ps1') -Name @(
        'game_wram_lifetime_probe', 'graphics_allocator_vector_probe',
        'cpu_defense_context_vector_probe', 'cpu_mode_seven_vector_probe',
        'cpu_mode_eight_vector_probe', 'cpu_mode_nine_vector_probe',
        'cpu_mode_ten_vector_probe', 'cpu_mode_twelve_vector_probe',
        'cpu_mode_thirteen_vector_probe', 'cpu_mode_fourteen_vector_probe',
        'cpu_mode_two_parent_vector_probe', 'normal_actor_parent_vector_probe'
    )

    Invoke-PythonRegression -Script 'test_game_wram_lifetime.py' -Arguments @(
        '--probe', (Join-Path $BuildDir 'game_wram_lifetime_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_graphics_allocator_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\graphics-allocator-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'graphics_allocator_vector_probe.exe')
    )

    Invoke-PythonRegression -Script 'verify_cpu_defense_context_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-defense-context-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_defense_context_vector_probe.exe')
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_seven_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-seven-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_seven_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_eight_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-eight-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_eight_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_nine_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-nine-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_nine_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_ten_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-ten-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_ten_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_twelve_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-twelve-witnesses.json'),
        '--caller-vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-twelve-ft-caller-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_twelve_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_thirteen_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-thirteen-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_thirteen_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_fourteen_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-fourteen-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_fourteen_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'test_cpu_mode_fourteen_fixture.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-fourteen-witnesses.json')
    )

    Invoke-PythonRegression -Script 'verify_cpu_mode_two_parent_vectors.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-two-parent-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'cpu_mode_two_parent_vector_probe.exe'),
        '--pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'test_cpu_mode_two_parent_fixture.py' -Arguments @(
        '--vectors', (Join-Path $Root 'tests\fixtures\cpu-mode-two-parent-witnesses.json')
    )

    Invoke-PythonRegression -Script 'test_cpu_mode_thirteen_pack.py' -Arguments @(
        '--probe', (Join-Path $BuildDir 'cpu_mode_fourteen_vector_probe.exe'),
        '--new-pack', $AssetPack
    )

    Invoke-PythonRegression -Script 'verify_normal_actor_parent_vectors.py' -Arguments @(
        '--vectors',
        (Join-Path $Root 'tests\fixtures\normal-actor-parent-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-five-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-three-role-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-one-role-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-two-prefix-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-two-half-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-two-role-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-four-role-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-four-pursuit-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-four-assignment-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-four-special-policy-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-four-far-policy-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-four-tied-policy-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-six-role-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-six-pursuit-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-six-anticipation-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-six-assignment-witnesses.json'),
        (Join-Path $Root 'tests\fixtures\cpu-mode-six-negative-role-witnesses.json'),
        '--probe', (Join-Path $BuildDir 'normal_actor_parent_vector_probe.exe'),
        '--pack', $AssetPack
    )

    $CommonRegressionArgs = @(
        '--pack', $AssetPack, '--exe', $ConsoleExePath, '--rom', $RomPath
    )
    foreach ($RegressionScript in @(
        'test_frontend_route.py',
        'test_title_pipeline.py',
        'test_setup_transition.py',
        'test_team_select.py',
        'test_player_setup.py',
        'test_player_lab.py',
        'test_player_intro.py',
        'test_player_intro_text.py',
        'test_tipoff.py',
        'test_gameplay_audio.py',
        'test_gameplay_debugger.py'
    )) {
        Invoke-PythonRegression -Script $RegressionScript -Arguments $CommonRegressionArgs
    }

    Invoke-PythonRegression -Script 'test_core_safety.py' -Arguments @(
        '--pack', $AssetPack, '--exe', $ConsoleExePath, '--rom', $RomPath,
        '--capture-root', $NativeCaptureRoot
    )

    $Gameplay55AssetArgs = @('--pack', $AssetPack)
    $Gameplay55NativeDir = Join-Path $NativeCaptureRoot 'ppu-runtime-final-20260829'
    if (Test-Path -LiteralPath (Join-Path $Gameplay55NativeDir 'scanout_0989_vram.bin')) {
        $Gameplay55AssetArgs += @('--native-dir', $Gameplay55NativeDir)
    }
    Invoke-PythonRegression -Script 'test_gameplay55_assets.py' -Arguments $Gameplay55AssetArgs

    Invoke-PythonRegression -Script 'test_intro_sequence.py' -Arguments @(
        '--pack', $AssetPack, '--exe', $ConsoleExePath, '--rom', $RomPath,
        '--native', (Join-Path $NativeCaptureRoot 'intro-exact-20260830\capture-v4')
    )
    Invoke-PythonRegression -Script 'test_project_census.py' -Arguments @(
        '--capture-root', $NativeCaptureRoot, '--recomp', $NativeRecompRoot
    )

    # Fail on inexpensive route/compositor checks before the long CPU trace.
    # All gates still run; none is sampled or replaced by the shorter tests.
    Invoke-PythonRegression -Script 'test_snes_mode1.py' -Arguments $CommonRegressionArgs
    Invoke-PythonRegression -Script 'test_cpu_gameplay.py' -Arguments $CommonRegressionArgs
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
