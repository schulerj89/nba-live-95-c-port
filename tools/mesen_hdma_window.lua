-- Walk HDMA channel 7's window table ($7F:6800 -> $2126/$2127) on the Game
-- Setup screen. Dumps it with the cursor on row 0 and again after moving down,
-- so the band that produces the gold selected-row highlight can be located.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local f = assert(io.open(out .. "/hdma_window.txt", "wb"))
f:write("# loaded\n")
f:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local DOWN_AT = 1950
local DOWN_LEN = 6
local DUMP1 = 1900
local DUMP2 = 2000
local LAST_FRAME = 2020

local function rd(a) return emu.read(a, emu.memType.snesMemory, false) or 0 end
local function rd16(a) return rd(a) | (rd(a + 1) << 8) end

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    elseif frame >= DOWN_AT and frame < DOWN_AT + DOWN_LEN then
        emu.setInput({ down = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

local function dumpTable(label)
    local b = 0x4300 + 7 * 0x10
    local bank = rd(b + 4)
    local addr = rd16(b + 2)
    f:write(string.format("=== %s: ch7 dest=$21%02X table=%02X:%04X params=%02X ===\n",
        label, rd(b + 1), bank, addr, rd(b)))
    local line = 0
    for entry = 1, 60 do
        local count = rd((bank << 16) | addr)
        addr = (addr + 1) & 0xFFFF
        if count == 0 then
            f:write(string.format("  terminator after line %d\n", line))
            break
        end
        local reps = count & 0x7F
        local perLine = (count & 0x80) ~= 0
        local n = perLine and reps * 2 or 2
        local parts = {}
        for i = 0, math.min(n, 16) - 1 do
            parts[#parts + 1] = string.format("%02X", rd((bank << 16) | ((addr + i) & 0xFFFF)))
        end
        addr = (addr + n) & 0xFFFF
        f:write(string.format("  lines %3d-%3d  count=%02X%s  WH0/WH1=%s\n",
            line, line + reps - 1, count, perLine and " per-line" or " repeat",
            table.concat(parts, " ")))
        line = line + reps
        if line > 240 then break end
    end
    f:write(string.format("  fixedColor=%s  colorMathEnabled=%s subtract=%s\n",
        tostring(select(2, pcall(function() return emu.getState()["ppu.fixedColor"] end))),
        tostring(select(2, pcall(function() return emu.getState()["ppu.colorMathEnabled"] end))),
        tostring(select(2, pcall(function() return emu.getState()["ppu.colorMathSubtractMode"] end)))))
    f:flush()
end

emu.addEventCallback(function()
    frame = frame + 1
    if frame == DUMP1 then dumpTable("cursor row 0") end
    if frame == DUMP2 then dumpTable("cursor row 1 (after Down)") end
    if frame >= LAST_FRAME then f:write("# done\n") f:close() emu.stop(0) end
end, emu.eventType.endFrame)
