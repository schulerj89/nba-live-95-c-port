-- Press Start mid-build (snap the title complete), then press Start again
-- during the hold, to see whether the second press short-circuits the fade.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local log = assert(io.open(out .. "/double.txt", "wb"))
log:write("# loaded\n") log:flush()

local frame = 0
local PRESS1 = 1450
local PRESS2 = 1500
local LEN = 6
local LAST_FRAME = 1700

emu.addEventCallback(function()
    local down = (frame >= PRESS1 and frame < PRESS1 + LEN)
               or (frame >= PRESS2 and frame < PRESS2 + LEN)
    if down then emu.setInput({ start = true }, 0) else emu.setInput({}, 0) end
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    frame = frame + 1
    if frame >= 1440 and frame <= LAST_FRAME then
        local ok, st = pcall(emu.getState)
        if ok and type(st) == "table" then
            log:write(string.format("%d bright=%s main=%s\n", frame,
                tostring(st["ppu.screenBrightness"]), tostring(st["ppu.mainScreenLayers"])))
            log:flush()
        end
    end
    if frame >= LAST_FRAME then log:write("# done\n") log:close() emu.stop(0) end
end, emu.eventType.endFrame)
