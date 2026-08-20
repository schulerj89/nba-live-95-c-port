-- Log per-frame BG scroll + HDMA state on the Game Setup screen.
-- Mesen's emu.getState() returns a FLAT table keyed by dotted paths, e.g.
-- st["ppu.layers[0].hscroll"].
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local f = assert(io.open(out .. "/scroll_log.txt", "wb"))
f:write("# loaded\n")
f:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local FROM = 1600
local TO = 2000

local function rd(a)
    return emu.read(a, emu.memType.snesMemory, false) or 0
end

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
        local ok, st = pcall(emu.getState)
        if ok and type(st) == "table" then
            local function g(k) return tostring(st["ppu." .. k]) end
            f:write(string.format(
                "%d bg1=%s/%s bg2=%s/%s bg3=%s/%s bright=%s main=%s sub=%s hdma=%02X\n",
                frame,
                g("layers[0].hscroll"), g("layers[0].vscroll"),
                g("layers[1].hscroll"), g("layers[1].vscroll"),
                g("layers[2].hscroll"), g("layers[2].vscroll"),
                g("screenBrightness"), g("mainScreenLayers"), g("subScreenLayers"),
                rd(0x420C)))
            f:flush()
        end
    end
    if frame > TO then
        f:write("# done\n")
        f:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
