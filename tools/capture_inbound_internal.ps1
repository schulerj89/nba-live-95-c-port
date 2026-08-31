[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDir,
      [string]$RomPath='F:\Games\SNES\NBA Live 95 (USA).sfc',
      [int]$Calls=500)
$ErrorActionPreference='Stop'
if ($Calls -lt 1) { throw 'Calls must be positive.' }
if (Test-Path -LiteralPath $OutputDir) { throw "Capture directory must be new: $OutputDir" }
$resolvedRom=(Resolve-Path -LiteralPath $RomPath).Path
$mesen=(Get-Command Mesen.exe).Source
New-Item -ItemType Directory -Path $OutputDir | Out-Null
$capture=(Resolve-Path -LiteralPath $OutputDir).Path
$script=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'mesen_inbound_internal.lua')).Path
$variables=@{
    NBA95_CAPTURE_DIR=$capture; NBA95_TOOL_DIR=$PSScriptRoot;
    NBA95_INBOUND_INTERNAL_MAX="$Calls";
    NBA95_VEC_ENTRY='86F43A'; NBA95_VEC_EXITS='86F439,86F653';
    NBA95_VEC_READS='0096-0097'; NBA95_VEC_WRITES='0096-0097';
    NBA95_VEC_LABEL='inbound-internal-driver'; NBA95_VEC_MAX='1000000';
    NBA95_VEC_DRIVE='1'; NBA95_CPU_VS_CPU='0'; NBA95_VEC_FRAMES='30000';
    NBA95_VEC_DELAY='0'; NBA95_VEC_SHARED_EXITS='1';
    NBA95_VEC_FORCE_PLAY_REQUEST='0'; NBA95_VEC_FORCE_SUB_REQUEST='0';
    NBA95_VEC_PREGAME='0'
}
$previous=@{}
try {
    foreach ($name in $variables.Keys) {
        $previous[$name]=[Environment]::GetEnvironmentVariable($name,'Process')
        [Environment]::SetEnvironmentVariable($name,$variables[$name],'Process')
    }
    $args=@('--testrunner','--timeout=300',('"'+$resolvedRom.Replace('\','/')+'"'),('"'+$script+'"'))
    $process=Start-Process -FilePath $mesen -ArgumentList $args -PassThru -WindowStyle Hidden
    $process.WaitForExit()
    $sentinel=Join-Path $capture 'inbound-internal-complete.txt'
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $sentinel)) {
        throw "Incomplete internal capture: exit=$($process.ExitCode) directory=$capture"
    }
    if ((Get-Content -Raw -LiteralPath $sentinel).Trim() -ne "complete_calls=$Calls") {
        throw 'Internal capture count does not match the requested bounded run.'
    }
    $manifest=[ordered]@{
        captured_utc=[DateTime]::UtcNow.ToString('o'); calls=$Calls;
        oracle='native original ROM in Mesen';
        rom_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $resolvedRom).Hash.ToLowerInvariant();
        mesen_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $mesen).Hash.ToLowerInvariant();
        script_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $script).Hash.ToLowerInvariant();
        driver_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $PSScriptRoot 'mesen_func_vectors.lua')).Hash.ToLowerInvariant();
        trace_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $capture 'inbound-internal.jsonl')).Hash.ToLowerInvariant();
        runtime_state_injection=$false; setup_mode_injection='Exhibition';
        controller_mode='native default, recorded per call'; environment=$variables;
        git_head=(& git -C (Join-Path $PSScriptRoot '..') rev-parse HEAD);
        git_status=@(& git -C (Join-Path $PSScriptRoot '..') status --short)
    }
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $capture 'run.json') -Encoding utf8
    Get-Content -LiteralPath $sentinel
} finally {
    foreach ($name in $previous.Keys) {
        [Environment]::SetEnvironmentVariable($name,$previous[$name],'Process')
    }
}
