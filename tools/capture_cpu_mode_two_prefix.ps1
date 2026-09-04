[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$OutputDir,
    [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc'
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDir){throw "Output directory must be new: $OutputDir"}
$rootPattern=[regex]::Escape((Resolve-Path -LiteralPath $PSScriptRoot).Path)
$conflict=Get-CimInstance Win32_Process -Filter "Name='Mesen.exe'" |
    Where-Object {$_.CommandLine -match $rootPattern}
if($conflict){throw 'Close other NBA Live 95 Mesen captures before capture.'}
$rom=(Resolve-Path -LiteralPath $RomPath).Path
$mesen=(Get-Command Mesen.exe).Source
New-Item -ItemType Directory -Path $OutputDir|Out-Null
$capture=(Resolve-Path -LiteralPath $OutputDir).Path
$script=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'mesen_cpu_mode_two_prefix.lua')).Path
$variables=@{
    NBA95_CAPTURE_DIR=$capture; NBA95_TOOL_DIR=$PSScriptRoot;
    NBA95_VEC_ENTRY='86F6CD'; NBA95_VEC_EXITS='86F793';
    NBA95_VEC_READS='0000-4AFF'; NBA95_VEC_WRITES='0000-4AFF';
    NBA95_VEC_LABEL='cpu_mode_two_prefix'; NBA95_VEC_MAX='3';
    NBA95_VEC_DRIVE='1'; NBA95_CPU_VS_CPU='1'; NBA95_VEC_FRAMES='12000';
    NBA95_VEC_DELAY='0'; NBA95_VEC_SHARED_EXITS='0'; NBA95_VEC_PREGAME='1';
    NBA95_VEC_FORCE_PLAY_REQUEST='0'; NBA95_VEC_FORCE_SUB_REQUEST='0'
}
$previous=@{}
try{
    foreach($name in $variables.Keys){
        $previous[$name]=[Environment]::GetEnvironmentVariable($name,'Process')
        [Environment]::SetEnvironmentVariable($name,$variables[$name],'Process')
    }
    $arguments=@('--testrunner','--timeout=180',('"'+$rom.Replace('\','/')+'"'),('"'+$script+'"'))
    $process=Start-Process -FilePath $mesen -ArgumentList $arguments -PassThru -WindowStyle Hidden
    $process.WaitForExit()
}finally{
    foreach($name in $variables.Keys){
        [Environment]::SetEnvironmentVariable($name,$previous[$name],'Process')
    }
}
$sentinel=Join-Path $capture 'capture_complete.txt'
if($process.ExitCode-ne 0-or!(Test-Path -LiteralPath $sentinel)){
    throw "Native CPU mode-two prefix capture failed: exit=$($process.ExitCode)"
}
$complete=(Get-Content -Raw -LiteralPath $sentinel).Trim()
if($complete-ne 'label=cpu_mode_two_prefix vectors=3 orphan_exits=0 shared_exit_callbacks=0'){
    throw "Incomplete CPU mode-two prefix capture: $complete"
}
$complete
