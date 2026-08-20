param(
    [Parameter(Mandatory = $true)]
    [string]$RomPath
)

$desktop = [Environment]::GetFolderPath('Desktop')
$shortcutPath = Join-Path $desktop "NBA Live '95 (C Port).lnk"
$workingDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$targetExe = Join-Path $workingDir 'build\nba95_port.exe'
$assetPack = Join-Path $workingDir 'build\nba95_assets.pak'
$arguments = "--rom `"$RomPath`" --assets `"$assetPack`""

$wshShell = New-Object -ComObject WScript.Shell
$shortcut = $wshShell.CreateShortcut($shortcutPath)
$shortcut.TargetPath = $targetExe
$shortcut.Arguments = $arguments
$shortcut.WorkingDirectory = $workingDir
$shortcut.Description = "NBA Live '95 Native C Port (SNES)"
$shortcut.Save()

if (Test-Path $shortcutPath) {
    Write-Host "Desktop shortcut created successfully at: $shortcutPath" -ForegroundColor Green
    
    # Trigger Windows Shell refresh
    python -c "import ctypes; ctypes.windll.shell32.SHChangeNotify(0x08000000, 0x1000, 0, 0)"
} else {
    Write-Error "Failed to create desktop shortcut."
}
