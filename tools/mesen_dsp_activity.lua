-- Poll the DSP each frame on the Game Setup screen and log per-voice pitch and
-- envelope. If the driver is sequencing, these change continuously; if the
-- screen is silent or holding one chord, they do not.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup_capture"
local f = assert(io.open(out .. "/dsp_activity.txt", "wb"))
f:write("# loaded\n")
f:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local FROM = 1600
local TO = 2300

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    frame = frame + 1
    if frame >= FROM and frame <= TO then
        local parts = {}
        for v = 0, 7 do
            local b = v * 16
            local pitch = (emu.read(b + 2, emu.memType.spcDspRegisters, false) or 0)
                        | ((emu.read(b + 3, emu.memType.spcDspRegisters, false) or 0) << 8)
            local envx = emu.read(b + 8, emu.memType.spcDspRegisters, false) or 0
            local srcn = emu.read(b + 4, emu.memType.spcDspRegisters, false) or 0
            parts[#parts + 1] = string.format("%d:%d/%d/%d", v, pitch, envx, srcn)
        end
        f:write(frame .. " " .. table.concat(parts, " ") .. "\n")
        f:flush()
    end
    if frame > TO then
        f:write("# done\n")
        f:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
