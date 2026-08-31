param([Parameter(Mandatory=$true)][string]$RomPath,
      [Parameter(Mandatory=$true)][string]$AssetPack,
      [Parameter(Mandatory=$true)][string]$OutputDir)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$rom=(Resolve-Path -LiteralPath $RomPath).Path
$pack=(Resolve-Path -LiteralPath $AssetPack).Path
$output=[IO.Path]::GetFullPath($OutputDir)
if(Test-Path -LiteralPath $output){throw 'Configuration report directory must be new.'}
New-Item -ItemType Directory -Path $output|Out-Null
function Checked([string]$Program,[string[]]$Arguments){
    & $Program @Arguments
    if($LASTEXITCODE-ne 0){throw "$Program failed ($LASTEXITCODE)."}
}
foreach($test in @('test_setup_config_evidence.py','test_setup_config_runtime_verifier.py',
                  'test_setup_config_adjustments.py','test_setup_value_canvas.py')){
    Checked 'python' @((Join-Path $PSScriptRoot $test))
}
foreach($name in @('setup_config_runtime_probe','menu_input_regression','main_canvas_probe','rules_canvas_probe')){
    & (Join-Path $PSScriptRoot 'build_vector_probe.ps1') -Name $name
    if($LASTEXITCODE-ne 0){throw "Probe compilation failed: $name"}
}
Checked (Join-Path $root 'build/menu_input_regression.exe') @()
$probe=Join-Path $root 'build/setup_config_runtime_probe.exe'
$fixture=Join-Path $root 'tests/fixtures/setup-config-native-witnesses.json'
Checked 'python' @((Join-Path $PSScriptRoot 'verify_setup_config_runtime.py'),'--fixture',$fixture,
    '--probe',$probe,'--rom',$rom,'--pack',$pack,'--report',(Join-Path $output 'stable-primary.json'))
Checked 'python' @((Join-Path $PSScriptRoot 'verify_setup_config_adjustments.py'),'--fixture',$fixture,
    '--probe',$probe,'--rom',$rom,'--pack',$pack,'--report',(Join-Path $output 'adjustments-primary.json'))
foreach($entry in @(@('main-visual','main-v4'),@('input','input-v1'),@('faces','faces-v1'))){
    $fixture=Join-Path $root ('tests/fixtures/setup-config-'+$entry[0]+'-native-witnesses.json')
    Checked 'python' @((Join-Path $PSScriptRoot 'verify_setup_config_runtime.py'),'--fixture',$fixture,
        '--journey',$entry[1],'--probe',$probe,'--rom',$rom,'--pack',$pack,
        '--report',(Join-Path $output ('stable-'+$entry[0]+'.json')))
    if($entry[0]-eq 'input'){
        Checked 'python' @((Join-Path $PSScriptRoot 'verify_setup_config_adjustments.py'),'--fixture',$fixture,
            '--journey',$entry[1],'--probe',$probe,'--rom',$rom,'--pack',$pack,
            '--report',(Join-Path $output 'adjustments-input.json'))
    }
}
Checked 'python' @((Join-Path $PSScriptRoot 'verify_setup_main_canvas.py'),
    '--fixture',(Join-Path $root 'tests/fixtures/setup-config-main-visual-native-witnesses.json'),
    '--journey','main-v4','--probe',(Join-Path $root 'build/main_canvas_probe.exe'),
    '--pack',$pack,'--output-dir',(Join-Path $output 'main-canvases'))
Checked 'python' @((Join-Path $PSScriptRoot 'verify_setup_main_canvas.py'),
    '--fixture',(Join-Path $root 'tests/fixtures/setup-config-faces-native-witnesses.json'),
    '--journey','faces-v1','--page','rules','--probe',(Join-Path $root 'build/rules_canvas_probe.exe'),
    '--pack',$pack,'--output-dir',(Join-Path $output 'rules-canvases'))
Write-Host 'PASS bounded Setup configuration gates; disk/runtime consumers and transition timing excluded.'
