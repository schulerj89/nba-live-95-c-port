-- Controlled signed-half witnesses for mode two's $86:F6EF-$F703 branch.
-- Each case changes documented input WRAM at mode two's genuine $87:9C21
-- wrapper, after $87:9244 has selected it and immediately before its native
-- JSL to $86:F6CD. PC, stack, CPU flags, ROM, RNG, and child results remain
-- untouched.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local labels = assert(io.open(out .. "/cpu-mode-two-half-cases.jsonl", "wb"))
local traces = assert(io.open(out .. "/cpu-mode-two-half-pcs.jsonl", "wb"))
local cases = {
    {name="negative_subpixel_opposite_half", integer=0xffff,
     fraction=0xff00, same_half=false},
    {name="zero_same_half", integer=0x0000,
     fraction=0x0000, same_half=true},
}
local index, pending_case = 0, nil

local function byte(address)
    return emu.read(address, emu.memType.snesWorkRam) or 0
end
local function word(address)
    return byte(address) | (byte(address + 1) << 8)
end
local function put_byte(address, value)
    emu.write(address, value & 0xff, emu.memType.snesWorkRam)
end
local function put_word(address, value)
    put_byte(address, value)
    put_byte(address + 1, value >> 8)
end
local function hook(pc, callback)
    emu.addMemoryCallback(callback, emu.callbackType.exec, pc, pc,
        emu.cpuType.snes, emu.memType.snesMemory)
end

hook(0x879c21, function()
    if pending_case or index >= #cases then return end
    local slot = word(0x00c2)
    local case = cases[index + 1]
    index = index + 1
    local actor = word(0x0096)
    local context = word(0x009e)
    assert(actor == 0x34eb + slot * 0x100, "unexpected actor pointer")
    assert(context == (slot < 5 and 0x46eb or 0x476b),
        "actor and side context disagree")
    assert(word(0x00c8) == 0x20, "unexpected actor dispatch delta")
    assert(word(actor + 0x5e) == 2, "dispatcher did not select mode two")
    local roster = word(0x00e0) | (byte(0x00e2) << 16)
    local profile_40 = emu.read((roster + 0x40) & 0xffffff,
        emu.memType.snesMemory) or 0
    local expected_timer = (profile_40 + 0x30 +
        (case.same_half and 0x20 or 0)) & 0xffff
    local saved = {}
    local function change_byte(address, value)
        if saved[address] == nil then saved[address] = byte(address) end
        put_byte(address, value)
    end
    local function change_word(address, value)
        change_byte(address, value)
        change_byte(address + 1, value >> 8)
    end
    -- Native compares signed actor +$04 against side context +$0A. The first
    -- case is -1/256: its integer word is still $FFFF even though nearest-pixel
    -- presentation rounding would produce zero.
    change_word(actor + 0x02, case.fraction)
    change_word(actor + 0x04, case.integer)
    change_word(context + 0x0a, 0x0150)
    change_word(actor + 0x60, 0x0020)
    change_word(actor + 0x7a, 0x0001)
    change_word(actor + 0x16, 0x0000)
    change_word(actor + 0x4e, 0x0001)
    change_word(actor + 0x50, 0x0006)
    change_word(0x093e, 0x0000)
    change_word(0x0946, 0x0000)
    pending_case = {
        saved=saved, path={}, slot=slot, actor=actor, context=context,
        expected_timer=expected_timer, case=case
    }
    labels:write(string.format(
        '{"case":%d,"name":"%s","slot":%d,"actor":%d,' ..
        '"context":%d,"integer":%d,"fraction":%d,"anchor":336,' ..
        '"profile_40":%d,"expected_timer":%d}\n',
        index, case.name, slot, actor, context, case.integer, case.fraction,
        profile_40, expected_timer))
    labels:flush()
end)

emu.addMemoryCallback(function(address)
    if not pending_case then return end
    pending_case.path[#pending_case.path + 1] = address & 0xffffff
    if address == 0x86f793 then
        local parts = {}
        for _, reached in ipairs(pending_case.path) do
            parts[#parts + 1] = string.format('"%06x"', reached)
        end
        traces:write(string.format(
            '{"case":%d,"executed":[%s]}\n', index,
            table.concat(parts, ",")))
        traces:flush()
    end
end, emu.callbackType.exec, 0x86f6cd, 0x86f793,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv("NBA95_TOOL_DIR")) .. "/mesen_func_vectors.lua")

local function restore()
    if not pending_case then return end
    assert(word(pending_case.actor + 0x5e) == 2,
        "native mode-two parent changed control mode")
    assert(word(pending_case.actor + 0x60) == pending_case.expected_timer,
        "unexpected native timer reload")
    assert(word(pending_case.actor + 0x4e) == 6,
        "native parent did not commit requested direction")
    for address, value in pairs(pending_case.saved) do put_byte(address, value) end
    pending_case = nil
end
hook(0x86f793, restore)
