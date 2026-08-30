[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RomPath,
    [Parameter(Mandatory=$true)][string]$GhidraHome,
    [Parameter(Mandatory=$true)][string]$JdkHome,
    [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if(!$OutputDirectory){
    $OutputDirectory=Join-Path $root '.analysis\human-free-throw-ghidra'
}
$projects=Join-Path $root 'ghidra-projects'
$headless=Join-Path $GhidraHome 'support\analyzeHeadless.bat'
foreach($path in @($headless,$RomPath,(Join-Path $JdkHome 'bin\java.exe'))){
    if(!(Test-Path -LiteralPath $path)){throw "Required input not found: $path"}
}
New-Item -ItemType Directory -Force $projects,$OutputDirectory|Out-Null
$env:JAVA_HOME=$JdkHome
$rom=[IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $RomPath))
$romOffset=0
if(($rom.Length % 0x8000)-eq 512){$romOffset=512}
if($rom.Length-lt($romOffset+8*0x8000)){throw 'ROM is too small to contain LoROM bank $87.'}
$bankBytes=New-Object byte[] 0x8000
[Array]::Copy($rom,$romOffset+7*0x8000,$bankBytes,0,$bankBytes.Length)
$bankPath=Join-Path $OutputDirectory 'bank_87.bin'
[IO.File]::WriteAllBytes($bankPath,$bankBytes)
& $headless $projects 'NbaLive95HumanFreeThrow87' `
    -import $bankPath -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
    -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
    -postScript 'DumpHumanFreeThrow.java' $OutputDirectory '87'
if($LASTEXITCODE-ne 0){throw 'Ghidra human free-throw dump failed.'}
Write-Host "Human free-throw dump written to: $OutputDirectory" -ForegroundColor Green
