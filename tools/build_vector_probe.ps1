param([Parameter(Mandatory=$true)][string[]]$Name)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
foreach ($probeName in $Name) {
    if ($probeName -notmatch '^[a-z0-9_]+$') { throw 'Expected a probe basename.' }
    $source = Join-Path $PSScriptRoot "$probeName.c"
    if (!(Test-Path -LiteralPath $source)) { throw "Probe source not found: $source" }
}
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
$commands = foreach ($probeName in $Name) {
    $source = Join-Path $PSScriptRoot "$probeName.c"
    @"
cl /nologo /W4 /O2 /MD /utf-8 /I "$root\include" /Fe"$build\$probeName.exe" /Fo"$build\$probeName.obj" "$source" $(($objects | ForEach-Object { '"' + $_ + '"' }) -join ' ') user32.lib gdi32.lib winmm.lib
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
"@
}
$command = @"
@echo off
call "$vcvars" >nul
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
$($commands -join "`r`n")
exit /b 0
"@
$batchName = if ($Name.Count -eq 1) { "compile_$($Name[0]).bat" } else { 'compile_vector_probes.bat' }
$batch = Join-Path $build $batchName
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if ($LASTEXITCODE -ne 0) { throw "Probe build failed: $Name" }
