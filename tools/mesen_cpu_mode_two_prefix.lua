-- Controlled owner/receiver witnesses for mode two's $86:F6CD prefix.
-- Each case changes documented input WRAM at mode two's genuine $87:9C21
-- wrapper, after $87:9244 has selected it and immediately before its native
-- JSL to $86:F6CD. PC, stack, CPU flags, ROM, RNG, and child results remain
-- untouched.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local labels = assert(io.open(out .. "/cpu-mode-two-prefix-cases.jsonl", "wb"))
local traces = assert(io.open(out .. "/cpu-mode-two-prefix-pcs.jsonl", "wb"))
local cases = {
    {name="owner_and_receiver_unset", owner=0xffff, receiver=0xffff,
     animation=8, base=10, expected_base=3},
    {name="named_owner", owner=0, receiver=0xffff,
     animation=10, base=8, expected_base=8},
    {name="named_receiver", owner=0xffff, receiver=0,
     animation=8, base=10, expected_base=10},
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
    assert(actor == 0x34eb + slot * 0x100, "unexpected actor pointer")
    assert(word(0x00c8) == 0x20, "unexpected actor dispatch delta")
    assert(word(actor + 0x5e) == 2, "dispatcher did not select mode two")
    local saved = {}
    local function change_byte(address, value)
        if saved[address] == nil then saved[address] = byte(address) end
        put_byte(address, value)
    end
    local function change_word(address, value)
        change_byte(address, value)
        change_byte(address + 1, value >> 8)
    end
    change_word(actor + 0x60, 0x0040)
    change_word(actor + 0x64, 0x0040)
    change_word(actor + 0x30, case.animation)
    change_word(actor + 0x38, case.base)
    change_word(actor + 0x4e, 1)
    change_word(actor + 0x50, 6)
    change_word(0x093e, case.owner)
    change_word(0x0946, case.receiver)
    pending_case = {saved=saved, path={}, slot=slot, actor=actor, case=case}
    labels:write(string.format(
        '{"case":%d,"name":"%s","slot":%d,"actor":%d,' ..
        '"owner":%d,"receiver":%d,"animation":%d,"base":%d,' ..
        '"expected_base":%d}\n', index, case.name, slot, actor,
        case.owner, case.receiver, case.animation, case.base,
        case.expected_base))
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
            '{"case":%d,"executed":[%s]}\n', index, table.concat(parts, ",")))
        traces:flush()
    end
end, emu.callbackType.exec, 0x86f6cd, 0x86f793,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv("NBA95_TOOL_DIR")) .. "/mesen_func_vectors.lua")

local function restore()
    if not pending_case then return end
    assert(word(pending_case.actor + 0x5e) == 2,
        "native mode-two parent changed control mode")
    assert(word(pending_case.actor + 0x38) ==
        pending_case.case.expected_base, "unexpected native base state")
    for address, value in pairs(pending_case.saved) do put_byte(address, value) end
    pending_case = nil
end
hook(0x86f793, restore)
