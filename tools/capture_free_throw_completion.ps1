[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$OutputDir,
    [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc',
    [int]$Calls=180,
    [ValidateSet(1,2)][int]$Attempts=2
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
New-Item -ItemType Directory -Force $OutputDir|Out-Null
$env:NBA95_CAPTURE_DIR=(Resolve-Path $OutputDir).Path
$env:NBA95_TOOL_DIR=(Resolve-Path $PSScriptRoot).Path
$env:NBA95_VEC_ENTRY='879CBF,87A15C,859530'
$env:NBA95_VEC_EXITS='87A017,87A2FB,87A2FD,859535,859597'
$env:NBA95_VEC_READS='0000-00FF,07F6-0A0F,17BF-17C1,180B-180D,3435-3F4A,46EB-48FF,492F-493F'
$env:NBA95_VEC_WRITES=$env:NBA95_VEC_READS
$env:NBA95_VEC_LABEL='free-throw-completion'
$env:NBA95_VEC_MAX="$Calls"
$env:NBA95_VEC_DRIVE='1'
$env:NBA95_CPU_VS_CPU='1'
$env:NBA95_VEC_FRAMES='12000'
$env:NBA95_VEC_DELAY='0'
$env:NBA95_VEC_SHARED_EXITS='1'
$env:NBA95_FT_ATTEMPTS="$Attempts"
$script=(Resolve-Path "$PSScriptRoot\mesen_free_throw_completion_control.lua").Path
$arguments=@('--testrunner','--timeout=300',('"'+$RomPath.Replace('\','/')+'"'),('"'+$script+'"'))
$process=Start-Process -FilePath (Get-Command Mesen.exe).Source -ArgumentList $arguments -PassThru -WindowStyle Hidden
$process.WaitForExit()
$sentinel=Join-Path $OutputDir 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){throw "Free-throw capture failed: $OutputDir"}
Get-Content $sentinel
