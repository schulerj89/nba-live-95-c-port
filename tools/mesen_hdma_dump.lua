-- Dump HDMA channel configuration and table contents on the settled Game Setup
-- screen, to find what drives the gold selected-row highlight.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup_capture"
local f = assert(io.open(out .. "/hdma_dump.txt", "wb"))
f:write("# loaded\n")
f:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local DUMP_AT = 1900
local LAST_FRAME = 1920

local function rd(a) return emu.read(a, emu.memType.snesMemory, false) or 0 end
local function rd16(a) return rd(a) | (rd(a + 1) << 8) end

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    frame = frame + 1
    if frame == DUMP_AT then
        local hdmaen = rd(0x420C)
        f:write(string.format("$420C HDMAEN = %02X\n", hdmaen))
        for ch = 0, 7 do
            local b = 0x4300 + ch * 0x10
            local params = rd(b + 0)
            local dest = rd(b + 1)
            local tbl = rd16(b + 2)
            local bank = rd(b + 4)
            local enabled = (hdmaen & (1 << ch)) ~= 0
            f:write(string.format(
                "ch%d %s params=%02X dest=$21%02X table=%02X:%04X indirectBank=%02X\n",
                ch, enabled and "ON " or "off", params, dest, bank, tbl, rd(b + 7)))
            if enabled then
                -- walk the HDMA table: [lineCount][data...] until a 00 terminator
                local addr = tbl
                local mode = params & 0x07
                local indirect = (params & 0x40) ~= 0
                local unitSize = ({ [0]=1, [1]=2, [2]=2, [3]=4, [4]=4, [5]=4, [6]=2, [7]=4 })[mode] or 1
                f:write(string.format("   mode=%d indirect=%s unitSize=%d\n",
                    mode, tostring(indirect), unitSize))
                local line = 0
                for entry = 1, 40 do
                    local count = rd((bank << 16) | addr)
                    addr = (addr + 1) & 0xFFFF
                    if count == 0 then
                        f:write(string.format("   [%02d] terminator at line %d\n", entry, line))
                        break
                    end
                    local reps = count & 0x7F
                    local repeatMode = (count & 0x80) ~= 0
                    local bytes = {}
                    local n = indirect and 2 or (repeatMode and reps * unitSize or unitSize)
                    for i = 0, math.min(n, 12) - 1 do
                        bytes[#bytes + 1] = string.format("%02X", rd((bank << 16) | ((addr + i) & 0xFFFF)))
                    end
                    addr = (addr + n) & 0xFFFF
                    f:write(string.format("   [%02d] line %3d count=%02X (%d lines%s) data=%s\n",
                        entry, line, count, reps, repeatMode and ", per-line" or ", repeat",
                        table.concat(bytes, " ")))
                    line = line + reps
                    if line > 240 then break end
                end
            end
        end
        f:flush()
    end
    if frame >= LAST_FRAME then
        f:write("# done\n") f:close() emu.stop(0)
    end
end, emu.eventType.endFrame)
