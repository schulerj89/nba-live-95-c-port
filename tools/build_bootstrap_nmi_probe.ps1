param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $output){throw 'Fresh output directory required.'}
New-Item -ItemType Directory -Path $output | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if($LASTEXITCODE -ne 0 -or !$vs){throw 'MSVC required'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$sources=@('tools/bootstrap_nmi_probe.c','src/nba_bootstrap.c','src/nba_bootstrap_nmi.c','src/nba_bootstrap_nmi_cpu.c','src/nba_bootstrap_cpu.c','src/nba_bootstrap_ipl.c','src/nba_bootstrap_rom.c','src/nba_setup_spc_init.c','src/nba_setup_spc_control.c','src/nba_rom.c')
$headers=@('include/nba_bootstrap_fill.h','include/nba_bootstrap_nmi.h','src/nba_bootstrap_nmi_cpu_program.inc','tools/generate_bootstrap_nmi_cpu.py','include/nba_bootstrap.h','include/nba_bootstrap_internal.h','include/nba_setup_codec_work.h','include/nba_setup_spc_init.h','include/nba_setup_spc_control.h','include/nba_setup_spc_resident.h','include/nba_rom.h','include/nba_types.h','src/nba_bootstrap_cpu_program.inc','tools/generate_bootstrap_cpu.py','tools/build_bootstrap_nmi_probe.ps1')
$arguments=($sources|ForEach-Object {'"'+(Join-Path $root $_)+'"'}) -join ' '
$batch=Join-Path $output 'compile.bat'
@"
@echo off
call "$vcvars" >nul
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /I "$root\include" /Fe"$output\bootstrap_nmi_probe.exe" /Fo"$output\\" $arguments
exit /b %ERRORLEVEL%
"@ | Set-Content -LiteralPath $batch -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE -ne 0){throw 'Bootstrap compile failed'}
$manifest=[ordered]@{schema=1;compiler_exit=0;sources=@{};executable=@{}}
foreach($name in ($sources+$headers)){
 $p=Join-Path $root $name
 $manifest.sources[$name]=@{path=$p;sha256=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$exe=Join-Path $output 'bootstrap_nmi_probe.exe'
$manifest.executable=@{path=$exe;sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()}
$manifest|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $output 'build-manifest.json') -Encoding UTF8
