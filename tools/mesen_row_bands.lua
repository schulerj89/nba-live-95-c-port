-- Screenshot the Game Setup screen with the cursor on each row, so the
-- scanline band that colour math highlights can be measured per row.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local log = assert(io.open(out .. "/log.txt", "wb"))
log:write("# loaded\n") log:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local FIRST_DOWN = 1900
local DOWN_GAP = 40      -- press Down every 40 frames
local ROWS = 6
local LAST_FRAME = FIRST_DOWN + DOWN_GAP * (ROWS + 1)

emu.addEventCallback(function()
    local down = false
    if frame >= FIRST_DOWN then
        local phase = (frame - FIRST_DOWN) % DOWN_GAP
        if phase < 5 then down = true end
    end
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    elseif down then
        emu.setInput({ down = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    frame = frame + 1
    -- shoot just before each Down, i.e. settled on the current row
    if frame >= FIRST_DOWN and (frame - FIRST_DOWN) % DOWN_GAP == DOWN_GAP - 2 then
        local row = (frame - FIRST_DOWN) // DOWN_GAP
        local g = assert(io.open(string.format("%s/row_%d.png", out, row), "wb"))
        g:write(emu.takeScreenshot())
        g:close()
        log:write(string.format("shot row %d at frame %d\n", row, frame))
        log:flush()
    end
    if frame >= LAST_FRAME then log:write("# done\n") log:close() emu.stop(0) end
end, emu.eventType.endFrame)
