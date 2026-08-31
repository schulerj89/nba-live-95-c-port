param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Choose a fresh private build directory.' }
New-Item -ItemType Directory -Path $output | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or !$vs) { throw 'MSVC build tools required.' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$batch = Join-Path $output 'compile.bat'
$command = @"
@echo off
call "$vcvars" >nul
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$output\period_roles_probe_v2.exe" /Fo"$output\\" "$root\tools\period_roles_probe_v2.c" "$root\src\nba_period_roles_v2.c" "$root\src\nba_period_roles.c"
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if ($LASTEXITCODE -ne 0) { throw 'Fresh period roles compilation failed.' }
$files = @('include/nba_period_roles.h','src/nba_period_roles.c','include/nba_period_roles_v2.h','src/nba_period_roles_v2.c','tools/period_roles_probe_v2.c','tools/period_roles_probe_fields_v2.inc','tools/build_period_roles_probe_v2.ps1')
$manifest = [ordered]@{schema=1;compiler_exit=0;sources=@{};executable=@{}}
foreach ($name in $files) {
    $path = Join-Path $root $name
    $manifest.sources[$name] = @{path=$path;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$exe = Join-Path $output 'period_roles_probe_v2.exe'
$manifest.executable = @{path=$exe;sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-manifest.json') -Encoding UTF8
