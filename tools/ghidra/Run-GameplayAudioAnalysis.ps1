[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RomPath,
    [Parameter(Mandatory=$true)][string]$GhidraHome,
    [Parameter(Mandatory=$true)][string]$JdkHome
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$project=Join-Path $root 'ghidra-projects'
$output=Join-Path $root '.analysis\gameplay-audio-ghidra'
$headless=Join-Path $GhidraHome 'support\analyzeHeadless.bat'
if(!(Test-Path -LiteralPath $headless)){throw "Ghidra headless not found: $headless"}
if(!(Test-Path -LiteralPath (Join-Path $JdkHome 'bin\java.exe'))){throw "JDK not found: $JdkHome"}
New-Item -ItemType Directory -Force $project,$output | Out-Null
$rom=[IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))
$bank=New-Object byte[] 0x8000
[Array]::Copy($rom,0x10000,$bank,0,$bank.Length)
$bankPath=Join-Path $output 'bank_82.bin'
[IO.File]::WriteAllBytes($bankPath,$bank)
$env:JAVA_HOME=$JdkHome
& $headless $project NbaLive95GameplayAudio `
    -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
    -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
    -postScript 'DumpGameplayAudio.java' $output
if($LASTEXITCODE -ne 0){throw "Gameplay audio Ghidra analysis failed: $LASTEXITCODE"}
Write-Host "Gameplay audio Ghidra proof: $output\gameplay_audio_ghidra.txt"
