-- Trace title-time writes to BG scroll/window registers. This isolates raster
-- effects that are not represented by a single end-of-frame PPU state value.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/title_capture/ppu_register_writes.txt"
local file = assert(io.open(out, "wb"))
file:write("# title_frame register value\n")
file:flush()

local title_frame = -1
emu.addMemoryCallback(function()
    if title_frame < 0 then title_frame = 0 end
end, emu.callbackType.exec, 0x80E1B1, 0x80E1B1,
    emu.cpuType.snes, emu.memType.snesMemory)

local function on_write(addr, value)
    if title_frame >= 0 then
        file:write(string.format("%d %04X %02X\n", title_frame, addr & 0xFFFF, value))
    end
end

for bank = 0, 0xBF do
    if bank <= 0x3F or bank >= 0x80 then
        local base = bank * 0x10000
        emu.addMemoryCallback(on_write, emu.callbackType.write,
            base + 0x210D, base + 0x2112, emu.cpuType.snes, emu.memType.snesMemory)
        emu.addMemoryCallback(on_write, emu.callbackType.write,
            base + 0x2126, base + 0x212F, emu.cpuType.snes, emu.memType.snesMemory)
    end
end

emu.addEventCallback(function()
    if title_frame >= 0 then title_frame = title_frame + 1 end
    if title_frame >= 2160 then
        file:write("# done\n")
        file:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
