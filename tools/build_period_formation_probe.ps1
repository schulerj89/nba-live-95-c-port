param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Use a fresh private build directory.' }
New-Item -ItemType Directory -Path $output | Out-Null
$deps = Join-Path $root '.analysis/period-formation-dependencies-v1'
$metadata = Get-Content -Raw (Join-Path $deps 'manifest.json') | ConvertFrom-Json
foreach ($entry in $metadata.files.PSObject.Properties.Value) {
 $snapshot = Join-Path $deps ([IO.Path]::GetFileName($entry.snapshot))
 if ((Get-FileHash -LiteralPath $snapshot -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) { throw 'Dependency snapshot identity changed.' }
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$local = @('tools/period_formation_probe.c','src/nba_period_formation.c','src/nba_period_restart_v2.c','src/nba_period_roles.c','src/nba_period_roles_v2.c')
$sourcePaths = @($local | ForEach-Object {Join-Path $root $_}) + @(Get-ChildItem -LiteralPath $deps -Filter '*.c' | ForEach-Object {$_.FullName})
$rsp = @('/nologo','/W4','/WX','/O2','/Gy','/Gw','/MD','/utf-8','/D_CRT_SECURE_NO_WARNINGS',('/I "'+$deps+'"'),('/I "'+$root+'\include"'),('/I "'+$root+'\tools"'),('/Fo"'+$output.Replace('\','/')+'/"'),('/Fe"'+$output+'\period_formation_probe.exe"'))
$rsp += @($sourcePaths | ForEach-Object {'"'+$_+'"'})
$rsp += @('/link /OPT:REF')
$rsp | Set-Content -LiteralPath (Join-Path $output 'compile.rsp') -Encoding ASCII
$batch = Join-Path $output 'compile.bat'
@"
@echo off
call "$vcvars" >nul
if errorlevel 1 exit /b %ERRORLEVEL%
cl @"$output\compile.rsp"
exit /b %ERRORLEVEL%
"@ | Set-Content -LiteralPath $batch -Encoding ASCII
& cmd /c $batch
if ($LASTEXITCODE -ne 0) {throw 'Fresh formation compilation failed.'}
$files = @($sourcePaths) + @('include/nba_period_formation.h','include/nba_period_restart_v2.h','include/nba_period_roles.h','include/nba_period_roles_v2.h','tools/period_formation_fields.inc','tools/build_period_formation_probe.ps1' | ForEach-Object {Join-Path $root $_}) + @(Get-ChildItem -LiteralPath $deps -Filter '*.h' | ForEach-Object {$_.FullName}) + @(Join-Path $deps 'manifest.json')
$manifest = [ordered]@{schema=1;compiler_exit=0;sources=@{};executable=@{}}
foreach($path in $files) {$manifest.sources[$path]=@{sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $path).Length}}
$exe = Join-Path $output 'period_formation_probe.exe'
$manifest.executable=@{path=$exe;sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-manifest.json') -Encoding UTF8
