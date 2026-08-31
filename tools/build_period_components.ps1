param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $output){throw 'Choose a fresh private build directory.'}
New-Item -ItemType Directory -Path $output | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if($LASTEXITCODE-ne0 -or !$vs){throw 'MSVC build tools required.'}
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$sources=@('nba_assets','nba_player_lab','nba_controller','nba_ea_intro','nba_intro_text','nba_renderer','nba_snes_ppu','nba_rom_font','nba_font','nba_gameplay_ai','nba_team_select','period_appearance','period_support')
$lines=[Collections.Generic.List[string]]::new()
$lines.Add('@echo off');$lines.Add("call `"$vcvars`" >nul");$lines.Add('if errorlevel 1 exit /b %ERRORLEVEL%')
foreach($name in $sources){
    $lines.Add("cl /nologo /W4 /WX /O2 /MD /utf-8 /D_CRT_SECURE_NO_WARNINGS /I `"$root\include`" /c /Fo`"$output\$name.obj`" `"$root\src\$name.c`"")
    $lines.Add('if errorlevel 1 exit /b %ERRORLEVEL%')
}
$objects=($sources | ForEach-Object {"`"$output\$_.obj`""}) -join ' '
foreach($name in @('period_appearance','period_support')){
    $lines.Add("cl /nologo /W4 /WX /O2 /MD /utf-8 /D_CRT_SECURE_NO_WARNINGS /I `"$root\include`" /Fe`"$output\${name}_probe.exe`" /Fo`"$output\${name}_probe.obj`" `"$root\tools\${name}_probe.c`" $objects user32.lib gdi32.lib winmm.lib")
    $lines.Add('if errorlevel 1 exit /b %ERRORLEVEL%')
}
$lines.Add('exit /b 0')
$batch=Join-Path $output 'compile.bat'
$lines | Set-Content -LiteralPath $batch -Encoding ASCII
& cmd /c $batch
if($LASTEXITCODE-ne0){throw 'Fresh period component build failed.'}
$files=@('tools/build_period_components.ps1','tools/period_appearance_probe.c','tools/period_support_probe.c')
$files+=($sources | ForEach-Object {"src/$_.c"})
$files+=(Get-ChildItem -LiteralPath (Join-Path $root 'include') -Filter '*.h' | ForEach-Object {"include/$($_.Name)"})
$manifest=[ordered]@{schema=1;compiler_exit=0;sources=@{};executables=@{}}
foreach($name in $files){$p=Join-Path $root $name;$manifest.sources[$name]=@{path=$p;sha256=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}}
foreach($name in @('period_appearance','period_support')){
    $p=Join-Path $output "${name}_probe.exe"
    $manifest.executables[$name]=@{path=$p;sha256=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'build-manifest.json') -Encoding UTF8
