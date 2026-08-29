-- Boundary-only inputs for rare `$85:B678-$B8CA` branches.  This changes
-- documented WRAM before the generic entry snapshot; it never changes ROM,
-- PC, stack, status flags, or the native RNG state.
local mode = assert(os.getenv("NBA95_MODE11_CONTROL"))

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
    local context = word(0x009E)
    local actor = word(0x0096)
    if mode == "urgent" then
        write_word(context + 0x3B, 0)
        write_word(0x092C, 0x0077)
    elseif mode == "context-shot" then
        write_word(context + 0x3B, 1)
        write_word(0x47EB, word(0x093A))
        write_word(0x47F3, word(0x47F3) | 0x0040)
    elseif mode == "context-clear" or mode == "context-blocked" then
        write_word(context + 0x3B, 1)
        write_word(0x0928, 0x0200)
        write_word(0x092C, 0x0200)
        for i = 0, 4 do
            write_word(0x47EB + i * 0x40, 0xFFFF)
            write_word(0x47F3 + i * 0x40, 0)
        end
        local subject_x = word(actor + 4)
        local subject_y = word(actor + 8)
        local subject_slot = word(0x00C2)
        -- First remove every possible lane blocker, then add exactly one for
        -- the blocked family. This avoids depending on the live formation.
        for slot = 0, 9 do
            if slot ~= subject_slot then
                local record = 0x34EB + slot * 0x100
                write_word(record + 4, subject_x)
                write_word(record + 8, 0x0300)
            end
        end
        if mode == "context-blocked" then
            local other = (subject_slot < 5) and 5 or 0
            local blocker = 0x34EB + other * 0x100
            local anchor = word(context + 0x0A)
            write_word(blocker + 4, ((subject_x + anchor) // 2) & 0xFFFF)
            write_word(blocker + 8, subject_y)
        end
    else
        error("unknown NBA95_MODE11_CONTROL=" .. mode)
    end
end, emu.callbackType.exec, 0x85B678, 0x85B678,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv("NBA95_MODE11_TRACE")))
