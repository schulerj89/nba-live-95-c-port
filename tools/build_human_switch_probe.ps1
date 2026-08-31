param([string]$OutputDirectory = '')
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$privateBuild=if($OutputDirectory){[IO.Path]::GetFullPath($OutputDirectory)}else{Join-Path $root 'build\human-control-action'}
New-Item -ItemType Directory -Path $privateBuild -Force | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC build tools required'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$objects=Get-Content (Join-Path $root 'nba95_sources.txt') |
 Where-Object {$_ -match '^src/' -and $_ -notmatch '(main|win32_game_main|nba_gameplay_ai)\.c$'} |
 ForEach-Object {Join-Path $root ('build\obj\'+[IO.Path]::GetFileNameWithoutExtension($_)+'.obj')}
foreach($object in $objects){if(!(Test-Path -LiteralPath $object)){throw 'Frozen checkpoint objects required'}}
$batch=Join-Path $privateBuild 'compile_human_switch.bat'
$command=@"
@echo off
call "$vcvars" >nul
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_human_switch.obj" "$root\src\nba_human_switch.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_gameplay_ai.obj" "$root\src\nba_gameplay_ai.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$privateBuild\human_switch_probe.exe" /Fo"$privateBuild\human_switch_probe.obj" "$root\tools\human_switch_probe.c" "$privateBuild\nba_human_switch.obj" "$privateBuild\nba_gameplay_ai.obj" $(($objects|ForEach-Object{'"'+$_+'"'})-join ' ') user32.lib gdi32.lib winmm.lib
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$privateBuild\human_direction_audit_probe.exe" /Fo"$privateBuild\human_direction_audit_probe.obj" "$root\tools\human_direction_audit_probe.c" "$privateBuild\nba_gameplay_ai.obj" $(($objects|ForEach-Object{'"'+$_+'"'})-join ' ') user32.lib gdi32.lib winmm.lib
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE-ne0){throw 'Human switch probe build failed'}
