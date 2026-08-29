[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RomPath,
    [Parameter(Mandatory=$true)][string]$GhidraHome,
    [Parameter(Mandatory=$true)][string]$JdkHome,
    [string]$RecompPath='',
    [int]$Passes=4
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$out=Join-Path $root '.analysis\full-rom-census'
$projects=Join-Path $root 'ghidra-projects'
$headless=Join-Path $GhidraHome 'support\analyzeHeadless.bat'
foreach($path in @($headless,$RomPath,(Join-Path $JdkHome 'bin\java.exe'))){
    if(!(Test-Path -LiteralPath $path)){throw "Required census input not found: $path"}
}
if(!$RecompPath){$RecompPath=Join-Path (Split-Path $root -Parent) 'NBA-Live-95-Recomp'}
New-Item -ItemType Directory -Force $out,$projects | Out-Null
& python (Join-Path $root 'tools\full_rom_census.py') prepare `
    --rom $RomPath --out $out --recomp $RecompPath
if($LASTEXITCODE -ne 0){throw 'Full-ROM census preparation failed.'}
$env:JAVA_HOME=$JdkHome
for($pass=1;$pass -le $Passes;$pass++){
    Write-Host "Full-ROM recursive census pass $pass/$Passes" -ForegroundColor Cyan
    Get-ChildItem (Join-Path $out 'seeds\bank_*.txt') |
        Where-Object Length -gt 0 | ForEach-Object {
        $bank=[regex]::Match($_.BaseName,'([0-9A-F]{2})$').Groups[1].Value
        $binary=Join-Path $out ("banks\bank_{0}.bin" -f $bank)
        & $headless $projects ("NbaLive95FullRom{0}" -f $bank) `
            -import $binary -overwrite -loader BinaryLoader -loader-baseAddr 0x8000 `
            -processor '65816:LE:16:default' -scriptPath $PSScriptRoot -noanalysis `
            -postScript 'DumpFullRomCensus.java' $out $bank $_.FullName
        if($LASTEXITCODE -ne 0){throw "Full-ROM Ghidra census failed for bank $bank"}
    }
    & python (Join-Path $root 'tools\full_rom_census.py') merge --out $out
    if($LASTEXITCODE -ne 0){throw 'Cross-bank census merge failed.'}
}
& python (Join-Path $root 'tools\full_rom_census.py') report --out $out `
    --write-md (Join-Path $root 'docs\full-rom-instruction-census.md') `
    --write-json (Join-Path $root 'docs\full-rom-instruction-census.json')
if($LASTEXITCODE -ne 0){throw 'Full-ROM census report failed.'}
Write-Host "Full-ROM census complete: docs\full-rom-instruction-census.md" -ForegroundColor Green
