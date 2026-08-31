$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$privateBuild=Join-Path $root 'build\human-pass-launch'
New-Item -ItemType Directory -Path $privateBuild -Force | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC build tools required'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$batch=Join-Path $privateBuild 'compile_human_pass_launch.bat'
$command=@"
@echo off
call "$vcvars" >nul
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_human_pass_launch.obj" "$root\src\nba_human_pass_launch.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$privateBuild\human_pass_launch_probe.exe" /Fo"$privateBuild\human_pass_launch_probe.obj" "$root\tools\human_pass_launch_probe.c" "$privateBuild\nba_human_pass_launch.obj"
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE-ne0){throw 'Human pass launch probe build failed'}
