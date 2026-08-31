$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$privateBuild=Join-Path $root 'build\human-pass-initializer'
New-Item -ItemType Directory -Path $privateBuild -Force | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC build tools required'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$objects=Get-Content (Join-Path $root 'nba95_sources.txt') |
 Where-Object {$_ -match '^src/' -and $_ -notmatch '(main|win32_game_main)\.c$'} |
 ForEach-Object {Join-Path $root ('build\obj\'+[IO.Path]::GetFileNameWithoutExtension($_)+'.obj')}
foreach($object in $objects){if(!(Test-Path -LiteralPath $object)){throw 'Frozen checkpoint objects required'}}
$batch=Join-Path $privateBuild 'compile_human_pass_init.bat'
$command=@"
@echo off
call "$vcvars" >nul
cl /nologo /W4 /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_human_pass_init.obj" "$root\src\nba_human_pass_init.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_human_pass_dependency.obj" "$root\src\nba_human_pass.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /O2 /MD /utf-8 /I "$root\include" /Fe"$privateBuild\human_pass_init_probe.exe" /Fo"$privateBuild\human_pass_init_probe.obj" "$root\tools\human_pass_init_probe.c" "$privateBuild\nba_human_pass_init.obj" "$privateBuild\nba_human_pass_dependency.obj" $(($objects|ForEach-Object{'"'+$_+'"'})-join ' ') user32.lib gdi32.lib winmm.lib
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE-ne0){throw 'Human pass probe build failed'}
