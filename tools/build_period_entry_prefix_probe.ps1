$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$privateBuild=Join-Path $root 'build\period-entry-prefix'
New-Item -ItemType Directory -Path $privateBuild -Force | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC build tools required'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$batch=Join-Path $privateBuild 'compile_period_entry_prefix.bat'
$command=@"
@echo off
call "$vcvars" >nul
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /c /Fo"$privateBuild\nba_period_entry_prefix.obj" "$root\src\nba_period_entry_prefix.c"
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$privateBuild\period_entry_prefix_probe.exe" /Fo"$privateBuild\period_entry_prefix_probe.obj" "$root\tools\period_entry_prefix_probe.c" "$privateBuild\nba_period_entry_prefix.obj"
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE-ne0){throw 'Period entry prefix probe build failed'}
$sourceNames=@('include/nba_types.h','include/nba_period_entry_prefix.h','src/nba_period_entry_prefix.c','tools/period_entry_prefix_probe.c','tools/period_entry_prefix_fields.inc','tools/build_period_entry_prefix_probe.ps1')
$sourceHashes=@{}
foreach($name in $sourceNames){$sourceHashes[$name]=(Get-FileHash -LiteralPath (Join-Path $root $name) -Algorithm SHA256).Hash.ToLower()}
$exe=Join-Path $privateBuild 'period_entry_prefix_probe.exe'
@{schema=1;compiler_exit=0;sources=$sourceHashes;executable=@{path=$exe;sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLower()}} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $privateBuild 'build-manifest.json') -Encoding UTF8
