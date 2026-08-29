-- Controlled real-entry cases for `$86:C493-$C4FD`.
-- Contact calls remain native.  At `$86:C4FE` we only provide deterministic
-- foul-classifier inputs so its real bookkeeping child is reached; at C493
-- we vary the child-owned WRAM predicates.  ROM, PC, stack and flags are not
-- patched.
local function read_word(address, kind)
    kind = kind or emu.memType.snesWorkRam
    local lo = emu.read(address, kind, false) or 0
    local hi = emu.read(address + 1, kind, false) or 0
    return lo | (hi << 8)
end

local function put(address, value)
    emu.write(address, value & 0xff, emu.memType.snesWorkRam)
    emu.write(address + 1, (value >> 8) & 0xff,
              emu.memType.snesWorkRam)
end

-- Make naturally reached classifier calls deterministic accepted fouls.
emu.addMemoryCallback(function()
    local cpu = emu.getState()
    local offender = cpu["cpu.y"] or 0
    put(0x0936, 0)
    put(0x0978, 0)
    put(0x09bc, 0)
    put(0x0964, 0)
    put(0x09b6, 0)
    put(0x0948, 0)
    put(0x07f6, 0)
    put(0x17d1, 0x40)
    put(0x17d3, 0x40)
    put(offender + 0x4c, 0x0300)
end, emu.callbackType.exec, 0x86c4fe, 0x86c4fe,
    emu.cpuType.snes, emu.memType.snesMemory)

local cases = {
    { personal = 5, team_count = 6, rule = 1, assignment = 0,
      stat = 9 },
    { personal = 5, team_count = 6, rule = 0, assignment = 0xffff,
      stat = 9 },
    { personal = 5, team_count = 5, rule = 1, assignment = 0xffff,
      stat = 9 },
    { personal = 0, team_count = 12, rule = 1, assignment = 0xffff,
      stat = 9 },
    { personal = 8, team_count = 12, rule = 1, assignment = 0xffff,
      stat = 9 },
}
local case_index = 0

emu.addMemoryCallback(function()
    case_index = case_index + 1
    local case = cases[case_index] or cases[#cases]
    local cpu = emu.getState()
    local actor = (cpu["cpu.a"] or 0) & 0xffff
    assert(actor < 10, "C493 actor index is outside the ten active records")
    local actor_record = read_word(0x879c7b + actor * 2,
                                   emu.memType.snesMemory)
    local player_record = read_word(0x3435 + actor * 2)
    local team_record = read_word(actor_record + 0x70)
    put(player_record + 0x14, case.personal)
    put(team_record + 0x54, case.team_count)
    put(actor_record + 0x16, case.assignment)
    put(0x17df, case.rule)
    put(0x09ca, 0)
    put(0x0a08, 0)
    if case.assignment ~= 0xffff then
        local stat_record = read_word(0x879c71 + case.assignment * 2,
                                      emu.memType.snesMemory)
        put(stat_record + 0x26, case.stat)
    end
end, emu.callbackType.exec, 0x86c493, 0x86c493,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv('NBA95_TOOL_DIR')) .. '/mesen_func_vectors.lua')
