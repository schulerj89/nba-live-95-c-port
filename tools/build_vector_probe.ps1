param([Parameter(Mandatory=$true)][string]$Name)
$ErrorActionPreference = 'Stop'
if ($Name -notmatch '^[a-z0-9_]+$') { throw 'Expected a probe basename.' }
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = Join-Path $PSScriptRoot "$Name.c"
if (!(Test-Path -LiteralPath $source)) { throw "Probe source not found: $source" }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'MSVC build tools are required.' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$build = Join-Path $root 'build'
$objects = Get-Content (Join-Path $root 'nba95_sources.txt') |
    Where-Object { $_ -match '^src/' -and $_ -notmatch '(main|win32_game_main)\.c$' } |
    ForEach-Object { Join-Path $build ('obj\' + [IO.Path]::GetFileNameWithoutExtension($_) + '.obj') }
foreach ($object in $objects) {
    if (!(Test-Path -LiteralPath $object)) { throw 'Run build.ps1 before compiling vector probes.' }
}
$command = @"
@echo off
call "$vcvars" >nul
cl /nologo /W4 /O2 /MD /utf-8 /I "$root\include" /Fe"$build\$Name.exe" /Fo"$build\$Name.obj" "$source" $(($objects | ForEach-Object { '"' + $_ + '"' }) -join ' ') user32.lib gdi32.lib winmm.lib
exit /b %ERRORLEVEL%
"@
$batch = Join-Path $build "compile_$Name.bat"
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if ($LASTEXITCODE -ne 0) { throw "Probe build failed: $Name" }
