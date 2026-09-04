-- Controlled `$09D8` witnesses for the normal mode-three parent at $86:F23F.
-- Each case changes documented input WRAM at the genuine $87:9244 behavior
-- dispatch boundary. Native code selects and executes the parent; PC, stack,
-- CPU flags, ROM, RNG, and child routine results remain untouched.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local labels = assert(io.open(out .. "/cpu-mode-three-role-cases.jsonl", "wb"))
local traces = assert(io.open(out .. "/cpu-mode-three-role-pcs.jsonl", "wb"))
local cases = {
    {name="role_clear_ownerless_record", role_ownerless=0, named_owner=false},
    {name="role_set_named_owner", role_ownerless=1, named_owner=true},
}
local index, pending_case, active = 0, nil, false

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

hook(0x87a47a, function() active = true end)
hook(0x879244, function()
    if not active or pending_case or index >= #cases then return end
    index = index + 1
    local case = cases[index]
    local slot = word(0x00c2)
    local actor = word(0x0096)
    assert(slot < 10, "unexpected actor index")
    assert(actor == 0x34eb + slot * 0x100, "unexpected actor pointer")
    local saved = {}
    local function change_byte(address, value)
        if saved[address] == nil then saved[address] = byte(address) end
        put_byte(address, value)
    end
    local function change_word(address, value)
        change_byte(address, value)
        change_byte(address + 1, value >> 8)
    end
    local group = word(actor + 0x6e)
    assert(group == 0 or group == 5, "unexpected actor team group")
    local owner = case.named_owner and ((slot + 1) % 10) or 0xffff
    change_word(actor + 0x5e, 3)
    change_word(actor + 0x60, 0x0020)
    change_word(actor + 0x64, 0x002f)
    change_word(actor + 0x7a, 0)
    change_word(actor + 0x7e, 0x0044)
    change_word(actor + 0x16, 0xffff)
    change_word(actor + 0x4e, 1)
    change_word(actor + 0x50, 6)
    change_word(0x0936, 0x0082)
    change_word(0x093a, group ~ 5)
    change_word(0x093e, owner)
    change_word(0x0948, 1)
    change_word(0x0952, group)
    change_word(0x0978, 0)
    change_word(0x0996, 0)
    change_word(0x09d8, case.role_ownerless)
    pending_case = {saved=saved, path={}, slot=slot, actor=actor, case=case}
    labels:write(string.format(
        '{"case":%d,"name":"%s","slot":%d,"actor":%d,' ..
        '"role_ownerless":%d,"owner":%d}\n', index, case.name,
        slot, actor, case.role_ownerless, owner))
    labels:flush()
end)

emu.addMemoryCallback(function(address)
    if not pending_case then return end
    pending_case.path[#pending_case.path + 1] = address & 0xffffff
    if address == 0x86f2c0 or address == 0x86f2c9 then
        local parts = {}
        for _, reached in ipairs(pending_case.path) do
            parts[#parts + 1] = string.format('"%06x"', reached)
        end
        traces:write(string.format(
            '{"case":%d,"executed":[%s]}\n', index, table.concat(parts, ",")))
        traces:flush()
    end
end, emu.callbackType.exec, 0x86f23f, 0x86f2c9,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv("NBA95_TOOL_DIR")) .. "/mesen_func_vectors.lua")

local function restore()
    if not pending_case then return end
    assert(word(pending_case.actor + 0x5e) == 3,
        "native mode-three parent changed control mode")
    for address, value in pairs(pending_case.saved) do put_byte(address, value) end
    pending_case = nil
end
hook(0x86f2c0, restore)
hook(0x86f2c9, restore)
