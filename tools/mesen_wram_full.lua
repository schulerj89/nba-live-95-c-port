-- Dump the full 128 KiB of WRAM on the Game Setup screen (banks $7E and $7F),
-- so HDMA tables living in bank $7F can be read offline.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local log = assert(io.open(out .. "/wram_full_log.txt", "wb"))
log:write("# loaded\n") log:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local DUMP_AT = 1900
local LAST_FRAME = 1920

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
        local t = {}
        for i = 0, 0x20000 - 1 do
            t[#t + 1] = string.char(emu.read(i, emu.memType.snesWorkRam, false) or 0)
        end
        local g = assert(io.open(out .. "/wram_full.bin", "wb"))
        g:write(table.concat(t))
        g:close()
        log:write("dumped wram_full.bin (131072 bytes)\n")
        -- also record ch7's live table pointer
        local function rd(a) return emu.read(a, emu.memType.snesMemory, false) or 0 end
        log:write(string.format("ch7 params=%02X dest=$21%02X table=%02X:%02X%02X cur=%02X%02X\n",
            rd(0x4370), rd(0x4371), rd(0x4374), rd(0x4373), rd(0x4372),
            rd(0x4379), rd(0x4378)))
        log:flush()
    end
    if frame >= LAST_FRAME then log:write("# done\n") log:close() emu.stop(0) end
end, emu.eventType.endFrame)
