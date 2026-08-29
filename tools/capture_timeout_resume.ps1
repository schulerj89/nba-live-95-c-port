param(
 [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc',
 [string]$OutputRoot='.analysis\timeout-resume-native-20260829'
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$out=[IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
$mesen=(Get-Command Mesen.exe -ErrorAction Stop).Source
foreach($case in @('left','right','zero_left','zero_right')) {
 $caseOut=Join-Path $out $case;New-Item -ItemType Directory -Force -Path $caseOut|Out-Null
 $env:NBA95_CAPTURE_DIR=$caseOut.Replace('\','/');$env:NBA95_TIMEOUT_CASE=$case
 $p=Start-Process -FilePath $mesen -ArgumentList @('--testrunner','--timeout=600',('"'+$RomPath+'"'),('"'+(Join-Path $PSScriptRoot 'mesen_timeout_resume_capture.lua')+'"')) -PassThru -Wait -WindowStyle Hidden
 if($p.ExitCode -ne 0 -or !(Test-Path (Join-Path $caseOut 'capture_complete.txt'))){throw "Timeout/resume capture failed: $case (exit $($p.ExitCode))"}
}
Remove-Item Env:NBA95_CAPTURE_DIR,Env:NBA95_TIMEOUT_CASE -ErrorAction SilentlyContinue
Write-Host "Timeout/resume captures complete: $out"
