param(
    [string]$RomPath = '',
    [string]$AssetPack = '',
    [switch]$ExtractAssets,
    [switch]$Run,
    [switch]$Headless,
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
        Write-Host "Warning: Asset extraction script returned non-zero exit code." -ForegroundColor Yellow
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

$CommonSources = @(
    "src\nba_rom.c",
    "src\nba_assets.c",
    "src\nba_audio.c",
    "src\nba_audio_debugger.c",
    "src\nba_font.c",
    "src\nba_renderer.c",
    "src\nba_ea_intro.c",
    "src\nba_title_sequence.c",
    "src\nba_setup_screen.c",
    "src\nba_game.c"
)

$IncludePath = Join-Path $Root "include"

$CompileScript = Join-Path $BuildDir "compile.bat"
$CompileBatchContent = @"
@echo off
call "$VcVars" > nul
echo Compiling nba95_port.exe (CLI/Test runner)...
cl.exe /nologo /W3 /O2 /MD /utf-8 /I "$IncludePath" /Fe"$ConsoleExePath" /Fo"$ObjDir\\" "$Root\src\main.c" "$Root\src\win32_game_main.c" $(($CommonSources | ForEach-Object { "`"$Root\$_`"" }) -join ' ') user32.lib gdi32.lib winmm.lib
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
