-- Record every 65816 write to the APU ports $2140-$2143 around the Game Setup
-- screen. These four bytes are the whole CPU->sound-driver interface, so the
-- captured stream is what the port must replay to make the driver sequence.
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

for port = 0, 3 do
    for _, base in ipairs({ 0x002140, 0x802140 }) do
        local a = base + port
        emu.addMemoryCallback(function(addr, value)
            if logging then
                f:write(string.format("%d %d %02X\n", frame, port, value))
                f:flush()
            end
        end, emu.callbackType.write, a, a, emu.cpuType.snes, emu.memType.snesMemory)
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
