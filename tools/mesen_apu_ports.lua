-- Record every 65816 write to the APU ports $2140-$2143 around the Game Setup
-- screen. These four bytes are the whole CPU->sound-driver interface.
--
-- The ports are mirrored across banks $00-$3F and $80-$BF, so every mirror has
-- to be hooked; hooking only $00/$80 misses most of the traffic.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup_capture"
local f = assert(io.open(out .. "/apu_ports.txt", "wb"))
f:write("# loaded\n")
f:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local LOG_FROM = 1600
local LAST_FRAME = 2100

local logging = false

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

local function onWrite(addr, value)
    if logging then
        f:write(string.format("%d %d %02X\n", frame, addr & 3, value))
        f:flush()
    end
end

for bank = 0, 0xBF do
    if bank <= 0x3F or bank >= 0x80 then
        local base = bank * 0x10000 + 0x2140
        emu.addMemoryCallback(onWrite, emu.callbackType.write, base, base + 3,
            emu.cpuType.snes, emu.memType.snesMemory)
    end
end

emu.addEventCallback(function()
    frame = frame + 1
    logging = (frame >= LOG_FROM and frame <= LAST_FRAME)
    if frame >= LAST_FRAME then
        f:write("# done\n")
        f:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
