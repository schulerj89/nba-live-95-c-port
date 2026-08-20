-- Press Start midway through the title build and capture every frame, to see
-- whether the ROM snaps the title to its finished state before transitioning.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/title_input"
local log = assert(io.open(out .. "/log.txt", "wb"))
log:write("# loaded\n") log:flush()

local frame = 0
local PRESS_AT = 1450      -- mid-build: only the first letters are up
local PRESS_LEN = 6
local SHOT_FROM = 1430
local SHOT_TO = 1820
local LAST_FRAME = 1840

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    frame = frame + 1
    if frame >= SHOT_FROM and frame <= SHOT_TO and frame % 2 == 0 then
        local g = assert(io.open(string.format("%s/t_%04d.png", out, frame), "wb"))
        g:write(emu.takeScreenshot())
        g:close()
    end
    if frame >= SHOT_FROM and frame <= SHOT_TO then
        local ok, st = pcall(emu.getState)
        if ok and type(st) == "table" then
            log:write(string.format("%d bright=%s main=%s bg1=%s/%s bg2=%s/%s\n", frame,
                tostring(st["ppu.screenBrightness"]), tostring(st["ppu.mainScreenLayers"]),
                tostring(st["ppu.layers[0].hscroll"]), tostring(st["ppu.layers[0].vscroll"]),
                tostring(st["ppu.layers[1].hscroll"]), tostring(st["ppu.layers[1].vscroll"])))
            log:flush()
        end
    end
    if frame >= LAST_FRAME then
        log:write("# done\n") log:close() emu.stop(0)
    end
end, emu.eventType.endFrame)
