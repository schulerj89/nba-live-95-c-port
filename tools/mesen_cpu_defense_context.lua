-- Controlled CPU defense-context witnesses on genuine `$85:B130` calls.
-- Only documented input WRAM is changed. Native code advances the RNG and
-- executes every selected branch; PC, stack, CPU flags, and ROM stay intact.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local labels = assert(io.open(out .. "/cpu-defense-context-cases.jsonl", "wb"))
local traces = assert(io.open(out .. "/cpu-defense-context-pcs.jsonl", "wb"))
local cases = {
    {name="early_trailing", current=2, opponent=4, period=2,
     activity=1, seed=0x0040, initial=4, expected=1},
    {name="late_trailing_nonzero", current=2, opponent=4, period=3,
     activity=1, seed=0x0001, initial=4, expected=2},
    {name="late_trailing_zero", current=2, opponent=4, period=3,
     activity=1, seed=0x0040, initial=4, expected=0},
    {name="tied_rng_odd", current=4, opponent=4, period=3,
     activity=1, seed=0x8000, initial=4, expected=1},
    {name="tied_rng_even", current=4, opponent=4, period=3,
     activity=1, seed=0x0001, initial=4, expected=3},
    {name="inactive_preserves", current=2, opponent=4, period=3,
     activity=0, seed=0x0001, initial=4, expected=4},
}
local index, pending, active = 0, nil, false

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
hook(0x85b116, function()
    if active and index < #cases then put_word(0x0994, 1) end
end)
hook(0x85b130, function()
    if not active or index >= #cases then return end
    assert(not pending, "nested defense-context entry")
    index = index + 1
    local case = cases[index]
    local context = word(0x009e)
    assert(context == 0x46eb or context == 0x476b,
        "unexpected active team context")
    local opponent = word(context + 2)
    assert(opponent == 0x46eb or opponent == 0x476b,
        "unexpected opposing team context")
    assert(opponent ~= context, "team context points to itself")
    local saved = {}
    local function change_byte(address, value)
        if saved[address] == nil then saved[address] = byte(address) end
        put_byte(address, value)
    end
    local function change_word(address, value)
        change_byte(address, value)
        change_byte(address + 1, value >> 8)
    end
    change_word(context + 0x26, case.current)
    change_word(opponent + 0x26, case.opponent)
    change_word(0x0926, case.period)
    change_word(opponent + 0x39, case.activity)
    change_word(opponent + 0x30, case.initial)
    change_word(0x07f6, case.seed)
    pending = {saved=saved, path={}, context=context, opponent=opponent}
    labels:write(string.format(
        '{"case":%d,"name":"%s","current_score":%d,' ..
        '"opponent_score":%d,"period":%d,"activity":%d,' ..
        '"rng_seed":%d,"initial_mode":%d,"expected_mode":%d,' ..
        '"context":%d,"opponent_context":%d}\n',
        index, case.name, case.current, case.opponent, case.period,
        case.activity, case.seed, case.initial, case.expected,
        context, opponent))
    labels:flush()
end)

emu.addMemoryCallback(function(address)
    if not pending then return end
    local pc = address & 0xffffff
    pending.path[#pending.path + 1] = pc
    if pc == 0x85b176 then
        local parts = {}
        for _, reached in ipairs(pending.path) do
            parts[#parts + 1] = string.format('"%06x"', reached)
        end
        traces:write(string.format(
            '{"case":%d,"executed":[%s]}\n', index, table.concat(parts, ",")))
        traces:flush()
    end
end, emu.callbackType.exec, 0x85b130, 0x85b176,
    emu.cpuType.snes, emu.memType.snesMemory)

-- The generic driver snapshots after the controlled entry hook and before
-- the restoration hook at the shared exit.
dofile(assert(os.getenv("NBA95_TOOL_DIR")) .. "/mesen_func_vectors.lua")

hook(0x85b176, function()
    if not pending then return end
    for address, value in pairs(pending.saved) do put_byte(address, value) end
    pending = nil
end)
