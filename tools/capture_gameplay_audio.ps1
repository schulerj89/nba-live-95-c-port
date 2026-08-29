param(
    [Parameter(Mandatory=$true)][string]$OutputDir,
    [int]$Frames=5000,
    [string]$RomPath='F:/Games/SNES/NBA Live 95 (USA).sfc',
    [string]$MesenPath=''
)
$ErrorActionPreference='Stop'
if(-not $MesenPath){$MesenPath=(Get-Command Mesen.exe -ErrorAction Stop).Source}
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TIPOFF_FRAMES="$Frames"
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_SCREENSHOT_FROM='999999'
$env:NBA95_SCREENSHOT_TO='999999'
$env:NBA95_TIPOFF_DRIVER=(Resolve-Path "$PSScriptRoot/mesen_tipoff_capture.lua").Path
$script=(Resolve-Path "$PSScriptRoot/mesen_gameplay_audio_trace.lua").Path
$process=Start-Process -FilePath $MesenPath -ArgumentList @(
    '--testrunner','--timeout=300',('"'+$RomPath+'"'),('"'+$script+'"')
) -PassThru -WindowStyle Hidden
$process.WaitForExit()
if($process.ExitCode -ne 0 -or !(Test-Path "$OutputDir/capture_complete.txt")){
    throw 'Native gameplay audio capture failed'
}
Write-Output "Native gameplay audio capture completed: $OutputDir"
