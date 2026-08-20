-- Capture a complete APU snapshot while the Game Setup music is playing:
-- 64 KiB SPC RAM, the 128 DSP registers, and the SPC700 CPU registers.
-- Together these are everything an SPC player needs to resume the ROM's own
-- sequencer, so the port can stream the sample bank instead of a WAV rip.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local log = assert(io.open(out .. "/spc_capture.txt", "wb"))
log:write("# loaded\n")
log:flush()

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

local function dumpMem(name, memType, size)
    local t = {}
    for i = 0, size - 1 do
        t[#t + 1] = string.char(emu.read(i, memType, false) or 0)
    end
    local g = assert(io.open(out .. "/" .. name, "wb"))
    g:write(table.concat(t))
    g:close()
    log:write(string.format("dumped %s (%d bytes)\n", name, size))
    log:flush()
end

emu.addEventCallback(function()
    frame = frame + 1
    if frame == DUMP_AT then
        dumpMem("spc_ram.bin", emu.memType.spcRam, 0x10000)
        dumpMem("spc_dsp.bin", emu.memType.spcDspRegisters, 0x80)

        local ok, st = pcall(emu.getState)
        if ok and type(st) == "table" then
            local keys = {}
            for k, v in pairs(st) do
                if type(k) == "string" and k:sub(1, 4) == "spc." and type(v) ~= "table" then
                    keys[#keys + 1] = k .. "=" .. tostring(v)
                end
            end
            table.sort(keys)
            local g = assert(io.open(out .. "/spc_state.txt", "wb"))
            g:write(table.concat(keys, "\n") .. "\n")
            g:close()
            log:write("wrote spc_state.txt (" .. #keys .. " keys)\n")
        end
        log:flush()
    end
    if frame >= LAST_FRAME then
        log:write("# done\n")
        log:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
