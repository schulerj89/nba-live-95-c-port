param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output=[IO.Path]::GetFullPath($OutputDirectory)
& (Join-Path $PSScriptRoot 'build_period_components.ps1') -OutputDirectory $output
$baseManifest=Join-Path $output 'build-manifest.json'
$base=Get-Content -LiteralPath $baseManifest -Raw | ConvertFrom-Json
$objects=($base.sources.PSObject.Properties.Name | Where-Object {$_ -like 'src/*'} | ForEach-Object {
    '"'+(Join-Path $output ([IO.Path]::GetFileNameWithoutExtension($_)+'.obj'))+'"'
}) -join ' '
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if($LASTEXITCODE-ne0 -or !$vs){throw 'MSVC build tools required.'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$batch=Join-Path $output 'compile-render.bat'
$command=@"
@echo off
call "$vcvars" >nul
if errorlevel 1 exit /b %ERRORLEVEL%
cl /nologo /W4 /WX /O2 /MD /utf-8 /D_CRT_SECURE_NO_WARNINGS /I "$root\include" /Fe"$output\period_render_tail_probe.exe" /Fo"$output\\" "$root\tools\period_render_tail_probe.c" "$root\src\period_render_tail.c" $objects user32.lib gdi32.lib winmm.lib
exit /b %ERRORLEVEL%
"@
Set-Content -LiteralPath $batch -Value $command -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE-ne0){throw 'Fresh render-tail build failed.'}
$manifest=[ordered]@{schema=1;compiler_exit=0;base_manifest_sha256=(Get-FileHash -LiteralPath $baseManifest -Algorithm SHA256).Hash.ToLowerInvariant();sources=@{};executable=@{}}
foreach($name in @('include/period_render_tail.h','src/period_render_tail.c','tools/period_render_tail_probe.c','tools/build_period_render_probe.ps1')){
    $p=Join-Path $root $name;$manifest.sources[$name]=@{path=$p;sha256=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$exe=Join-Path $output 'period_render_tail_probe.exe'
$manifest.executable=@{path=$exe;sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'render-build-manifest.json') -Encoding UTF8
