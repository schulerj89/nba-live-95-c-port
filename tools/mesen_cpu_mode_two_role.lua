-- Controlled witnesses for mode two's $86:F721-$F793 continuation.
-- Each case changes documented input WRAM at mode two's genuine $87:9C21
-- wrapper, after $87:9244 has selected it and immediately before its native
-- JSL to $86:F6CD. PC, stack, CPU flags, ROM, RNG, and child results remain
-- untouched.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local labels = assert(io.open(out .. "/cpu-mode-two-role-cases.jsonl", "wb"))
local traces = assert(io.open(out .. "/cpu-mode-two-role-pcs.jsonl", "wb"))
local cases = {
    {name="role_clear_ownerless_record", role_ownerless=0,
     named_owner=false},
    {name="role_set_named_owner", role_ownerless=1,
     named_owner=true},
    {name="role_clear_uses_base_assignment", role_ownerless=0,
     named_owner=false, diverge_assignment=true},
    {name="boosted_cpu_calls_jump_reach", role_ownerless=0,
     named_owner=false, jump_reach=true},
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
    assert(slot < 10, "unexpected actor index")
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
    local owner = case.named_owner and ((slot + 1) % 10) or 0xffff
    local receiver = case.jump_reach and 0xffff or 0x0000
    local controller = case.jump_reach and 0xffff or 0x0000
    local assignment = word(actor + 0x74)
    local paired_slot = assignment >> 1
    local paired = 0x34eb + paired_slot * 0x100
    assert(paired_slot >= 5 and paired_slot < 10,
        "mode-two witness did not select an opposing actor")
    local current_slot = paired_slot
    if case.diverge_assignment then
        current_slot = 5 + ((paired_slot - 5 + 1) % 5)
    end
    local current_assignment = current_slot * 2
    local current_paired = 0x34eb + current_slot * 0x100
    -- A nonzero free-throw state makes the already verified $86:F0FD child
    -- accept exactly the actor named at $7E492F. This isolates whether the
    -- parent calls that child from $09D8; the owner record deliberately says
    -- the opposite thing in each case.
    change_word(actor + 0x60, 0x0020)
    change_word(actor + 0x7a, 0x0000)
    change_word(actor + 0x16, controller)
    change_word(actor + 0x4e, 0x0001)
    change_word(actor + 0x50, 0x0006)
    change_word(actor + 0x76, current_assignment)
    -- Keep the later, independently verified $86:E7DC target child neutral.
    -- The C bridge refreshes the pair cache at this boundary, while native
    -- code consumes the cache produced by $85:BC52/$85:AFC2.  These positions
    -- and cache words describe the same simple horizontal pairing in both:
    -- current=(0,0), paired=(100,0), coarse pair direction 2/distance 100,
    -- and paired-to-right-basket fine direction 4/distance 236.
    change_word(actor + 0x02, 0x0000)
    change_word(actor + 0x04, 0x0000)
    change_word(actor + 0x06, 0x0000)
    change_word(actor + 0x08, 0x0000)
    change_word(paired + 0x02, 0x0000)
    change_word(paired + 0x04, 0x0064)
    change_word(paired + 0x06, 0x0000)
    change_word(paired + 0x08, 0x0000)
    change_word(paired + 0x0e, 0x0000)
    change_word(paired + 0x10, 0x0000)
    change_word(actor + 0x86, 0x0002)
    change_word(actor + 0x8a, 0x0064)
    change_word(paired + 0x86, 0x0006)
    change_word(paired + 0x88, 0x0004)
    change_word(paired + 0x8a, 0x0064)
    change_word(paired + 0x8c, 0x00ec)
    change_word(paired + 0x92, 0x0000)
    if case.diverge_assignment then
        -- Base +$74 names the player at (+100,0), while mutable +$76 names a
        -- different valid opponent at (-100,0). The two choices reach
        -- different target families, so the native result identifies which
        -- assignment word F72E actually consumes.
        change_word(current_paired + 0x02, 0x0000)
        change_word(current_paired + 0x04, 0xff9c)
        change_word(current_paired + 0x06, 0x0000)
        change_word(current_paired + 0x08, 0x0000)
        change_word(current_paired + 0x0e, 0x0000)
        change_word(current_paired + 0x10, 0x0000)
        change_word(current_paired + 0x86, 0x0002)
        change_word(current_paired + 0x88, 0x0002)
        change_word(current_paired + 0x8a, 0x0064)
        change_word(current_paired + 0x8c, 0x01b4)
        change_word(current_paired + 0x92, 0x0000)
    end
    if case.jump_reach then
        -- Keep +$72 nonzero after E7DC's A82C movement step. EC32 does not
        -- read this boost timer: F780 gates only on signed controller +$16.
        -- The controlled ball arc reaches EC32's verified long-jump path and
        -- installs state $32, making a skipped call visible at the parent exit.
        change_word(actor + 0x0a, 0x0000)
        change_word(actor + 0x0c, 0x0000)
        change_word(actor + 0x0e, 0x007b)
        change_word(actor + 0x10, 0x01c8)
        change_word(actor + 0x12, 0x0315)
        change_word(actor + 0x30, 0x0000)
        change_word(actor + 0x32, 0x0000)
        change_word(actor + 0x72, 0x0010)
        change_word(actor + 0x8e, 0x000a)
        change_word(0x0046, 0x0000)
        change_word(0x07f6, 0x0001)
        change_word(0x0910, 0x3eeb)
        change_word(0x0948, 0x0000)
        change_word(0x0962, 0x0000)
        change_word(0x3eed, 0x0000)
        change_word(0x3eef, 0x0000)
        change_word(0x3ef1, 0x0000)
        change_word(0x3ef3, 0x0000)
        change_word(0x3ef5, 0x0000)
        change_word(0x3ef7, 0x0064)
        change_word(0x3efd, 0xff9c)
    end
    change_word(0x46eb + 0x0a, 0xfeb0)
    change_word(0x46eb + 0x30, 0x0004)
    change_word(0x46eb + 0x32, 0x0001)
    change_word(0x476b + 0x0a, 0x0150)
    change_word(0x0936, 0x0081)
    change_word(0x093e, owner)
    change_word(0x0946, receiver)
    change_word(0x0978, 0x0001)
    change_word(0x09d8, case.role_ownerless)
    change_word(0x492f, slot)
    pending_case = {saved=saved, path={}, slot=slot, actor=actor, case=case,
                    ec32_calls=0}
    labels:write(string.format(
        '{"case":%d,"name":"%s","slot":%d,"actor":%d,' ..
        '"role_ownerless":%d,"owner":%d,"foul_actor":%d,' ..
        '"assignment_base":%d,"assignment_current":%d,' ..
        '"receiver":%d,"controller":%d,"boost":%d,' ..
        '"paired_slot":%d,"paired":%d,"current_slot":%d,' ..
        '"current_paired":%d}\n',
        index, case.name, slot, actor, case.role_ownerless, owner, slot,
        assignment, current_assignment, receiver, controller,
        case.jump_reach and 0x0010 or word(actor + 0x72),
        paired_slot, paired, current_slot,
        current_paired))
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
            '{"case":%d,"ec32_calls":%d,"executed":[%s]}\n', index,
            pending_case.ec32_calls, table.concat(parts, ",")))
        traces:flush()
    end
end, emu.callbackType.exec, 0x86f6cd, 0x86f793,
    emu.cpuType.snes, emu.memType.snesMemory)

hook(0x86ec32, function()
    if pending_case then pending_case.ec32_calls = pending_case.ec32_calls + 1 end
end)

dofile(assert(os.getenv("NBA95_TOOL_DIR")) .. "/mesen_func_vectors.lua")

local function restore()
    if not pending_case then return end
    assert(word(pending_case.actor + 0x5e) == 2,
        "native mode-two parent changed control mode")
    for address, value in pairs(pending_case.saved) do put_byte(address, value) end
    pending_case = nil
end
hook(0x86f793, restore)
