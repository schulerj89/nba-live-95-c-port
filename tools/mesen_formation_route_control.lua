-- Boundary-only branch controller for `$85:AD6B-$AF5B` differential proof.
-- It changes documented WRAM inputs before the generic driver snapshots them;
-- ROM, PC, stack, code, and RNG are never patched.
local mode = assert(os.getenv("NBA95_FORMATION_CONTROL"))

local function word(address)
    local lo = emu.read(address, emu.memType.snesWorkRam, false) or 0
    local hi = emu.read(address + 1, emu.memType.snesWorkRam, false) or 0
    return lo | (hi << 8)
end

local function write_word(address, value)
    emu.write(address, value & 0xff, emu.memType.snesWorkRam)
    emu.write(address + 1, (value >> 8) & 0xff, emu.memType.snesWorkRam)
end

emu.addMemoryCallback(function()
    local slot = word(0x00C2)
    local actor = word(0x0096)
    if mode == "early" then
        write_word(0x093E, slot)
        write_word(0x0968, 1)
    elseif mode == "inbound" then
        write_word(0x093E, 0xFFFF)
        write_word(0x0968, 0)
        write_word(0x0936, 0x0082)
        write_word(0x0954, slot)
        write_word(0x09A2, 0xFFFF)
        write_word(actor + 0x7E, word(actor + 0x7E) & 0xFFF7)
    elseif mode == "special" then
        write_word(0x093E, 0xFFFF)
        write_word(0x0968, 0)
        write_word(0x09A2, slot)
    elseif mode == "edge" then
        write_word(0x093E, 0xFFFF)
        write_word(0x0968, 0)
        write_word(0x0948, 1)
        write_word(0x097C, 0)
    elseif mode == "timer" then
        write_word(0x093E, 0xFFFF)
        write_word(0x0968, 0)
        write_word(0x005C, 1)
        write_word(actor + 0x5C, 0)
    else
        error("unknown NBA95_FORMATION_CONTROL=" .. mode)
    end
end, emu.callbackType.exec, 0x85AD6B, 0x85AD6B,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv("NBA95_FORMATION_TRACE")))
