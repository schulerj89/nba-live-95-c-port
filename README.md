# NBA Live '95 - SNES Native C Port

A high-performance, native C port and decompilation of **NBA Live '95** for the Super Nintendo Entertainment System (SNES).

## Architecture & Subsystems

- **Native C Implementation**: Direct C99 architecture with no heavy third-party dependencies.
- **Hardware Register Emulation**: Replicates the 65816 boot sequence (`$80:8020`), PPU forced blanking (`$2100 = 0x8F`), VMAIN configuration (`$2115 = 0x80`), FastROM enablement (`$420D = 0x01`), NMI & auto-joypad interrupts (`$4200 = 0x81`), VRAM/CGRAM management, and frame pacing.
- **Accurate Font & Graphics Pipeline**: Reconstructs authentic SNES font glyphs and BGR555 color palettes.
- **Native Win32 Presentation**: Low-latency 59.94 Hz frame timer (`QueryPerformanceCounter` & `timeBeginPeriod(1)`) with `StretchDIBits` rendering.

## Building & Running

### Prerequisites
- CMake 3.16+
- Visual Studio 2022 (MSVC C/C++ compiler) or Clang/GCC

### Build Command
```powershell
.\build.ps1
```

### Running GUI
```powershell
.\build.ps1 -Run -RomPath "F:\Games\SNES\NBA Live 95 (USA).sfc"
```

### Headless Frame Capture / Automated Test
```powershell
.\build.ps1 -Headless -RomPath "F:\Games\SNES\NBA Live 95 (USA).sfc" -DumpFrame "build\nintendo_license.bmp"
```

## Controls

| SNES Button | Keyboard Key |
|---|---|
| **D-Pad Up** | Up Arrow / W |
| **D-Pad Down** | Down Arrow / S |
| **D-Pad Left** | Left Arrow / A |
| **D-Pad Right** | Right Arrow / D |
| **Button A** | X / K |
| **Button B** | Z / J |
| **Button X** | V / I |
| **Button Y** | C / U |
| **L Trigger** | Q |
| **R Trigger** | E |
| **Start** | Enter |
| **Select** | Space / Shift |
