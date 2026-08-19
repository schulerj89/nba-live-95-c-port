$desktop = [Environment]::GetFolderPath('Desktop')
$shortcutPath = Join-Path $desktop "NBA Live '95 (C Port).lnk"
$targetExe = "C:\Users\joshs\Projects\nba-live-95-c-port\build\nba95_port.exe"
$workingDir = "C:\Users\joshs\Projects\nba-live-95-c-port"
$arguments = '--rom "F:\Games\SNES\NBA Live 95 (USA).sfc" --assets "C:\Users\joshs\Projects\nba-live-95-c-port\build\nba95_assets.pak"'

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
