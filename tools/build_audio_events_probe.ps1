param()
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC is required.'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$build=Join-Path $root 'build/audio-events'
New-Item -ItemType Directory -Path $build -Force|Out-Null
$batch=Join-Path $build 'compile.bat'
$command=@"
@echo off
call "$vcvars" >nul
cl /nologo /std:c11 /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$build\audio_events_probe.exe" /Fo"$build\\" "$root\tools\audio_events_probe.c" "$root\src\nba_audio_events.c"
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ascii
& cmd /c $batch
if($LASTEXITCODE-ne 0){throw 'Audio event probe compilation failed.'}
