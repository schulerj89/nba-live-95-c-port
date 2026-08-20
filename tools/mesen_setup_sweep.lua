-- Boot NBA Live '95, press Start on the title screen, and capture the frames
-- around the Game Setup transition.
-- NOTE: this Mesen build's signature is emu.setInput(inputTable, port).
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup_capture"
local log = assert(io.open(out .. "/sweep_log.txt", "wb"))

local frame = 0
local START_FROM = 1400
local SHOT_FROM = 1400
local LAST_FRAME = 3600
local SHOT_EVERY = 10

emu.addEventCallback(function()
    if frame >= START_FROM and (frame % 24) < 8 then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    frame = frame + 1
    if frame >= SHOT_FROM and frame % SHOT_EVERY == 0 then
        local g = assert(io.open(string.format("%s/sweep_%04d.png", out, frame), "wb"))
        g:write(emu.takeScreenshot())
        g:close()
    end
    if frame >= LAST_FRAME then
        log:write("sweep done at frame " .. frame .. "\n")
        log:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
