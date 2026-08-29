param(
    [string]$RomPath = 'F:\Games\SNES\NBA Live 95 (USA).sfc',
    [string]$OutputRoot = '.analysis\match-lifecycle-native-20260829'
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$out=[IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
$script=Join-Path $PSScriptRoot 'mesen_match_lifecycle_capture.lua'
$mesen=(Get-Command Mesen.exe -ErrorAction Stop).Source
foreach($case in @('q1','halftime','regulation_tie','regulation_final')) {
    $caseOut=Join-Path $out $case
    New-Item -ItemType Directory -Force -Path $caseOut | Out-Null
    $env:NBA95_CAPTURE_DIR=$caseOut.Replace('\','/')
    $env:NBA95_LIFECYCLE_CASE=$case
    $p=Start-Process -FilePath $mesen -ArgumentList @(
        '--testrunner','--timeout=600',('"'+$RomPath+'"'),('"'+$script+'"')) `
        -PassThru -Wait -WindowStyle Hidden
    if($p.ExitCode -ne 0 -or !(Test-Path (Join-Path $caseOut 'capture_complete.txt'))) {
        throw "Lifecycle capture failed: $case (exit $($p.ExitCode))"
    }
}
Remove-Item Env:NBA95_CAPTURE_DIR,Env:NBA95_LIFECYCLE_CASE -ErrorAction SilentlyContinue
Write-Host "Lifecycle captures complete: $out"
