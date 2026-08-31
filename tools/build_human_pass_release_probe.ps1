$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$privateBuild=Join-Path $root 'build\human-pass-release'
New-Item -ItemType Directory -Path $privateBuild -Force | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC build tools required'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$batch=Join-Path $privateBuild 'compile_human_pass_release.bat'
$command=@"
@echo off
call "$vcvars" >nul
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_human_pass_release.obj" "$root\src\nba_human_pass_release.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_human_pass_pose.obj" "$root\src\nba_human_pass_pose.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_assets.obj" "$root\src\nba_assets.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_ea_intro.obj" "$root\src\nba_ea_intro.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_intro_text.obj" "$root\src\nba_intro_text.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_renderer.obj" "$root\src\nba_renderer.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_snes_ppu.obj" "$root\src\nba_snes_ppu.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_rom_font.obj" "$root\src\nba_rom_font.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$privateBuild\human_pass_release_probe.exe" /Fo"$privateBuild\human_pass_release_probe.obj" "$root\tools\human_pass_release_probe.c" "$privateBuild\nba_human_pass_release.obj" "$privateBuild\nba_human_pass_pose.obj" "$privateBuild\nba_assets.obj" "$privateBuild\nba_ea_intro.obj" "$privateBuild\nba_intro_text.obj" "$privateBuild\nba_renderer.obj" "$privateBuild\nba_snes_ppu.obj" "$privateBuild\nba_rom_font.obj"
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE-ne0){throw 'Human pass release probe build failed'}
