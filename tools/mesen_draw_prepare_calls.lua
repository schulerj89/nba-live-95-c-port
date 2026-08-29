-- Capture the portable inputs selected by `$87:A47A-$A6A4` immediately
-- before its `$80:AD92` sprite compositor call. The generic vector driver
-- below only supplies the verified Exhibition/CPU route.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local max_calls = tonumber(os.getenv("NBA95_DRAW_CALLS")) or 2000
local file = assert(io.open(out .. "/draw-prepare-calls.jsonl", "wb"))
local calls, current_base, selected_direction = 0, nil, nil
local candidate_dx, candidate_dy, candidate_valid, done = 0, 0, false, false

local function w(address)
    local lo = emu.read(address, emu.memType.snesWorkRam, false) or 0
    local hi = emu.read(address + 1, emu.memType.snesWorkRam, false) or 0
    return lo | (hi << 8)
end

local function cpu(name)
    return emu.getState()["cpu." .. name] or 0
end

emu.addMemoryCallback(function()
    local y = w(0x0096)
    current_base = w(0x7e44 + y)
    candidate_valid = false
end, emu.callbackType.exec, 0x87A47A, 0x87A47A,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    candidate_dx, candidate_dy = w(0x00aa), w(0x00ae)
    candidate_valid = true
end, emu.callbackType.exec, 0x87A5B6, 0x87A5B6,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    selected_direction = cpu("a") & 7
end, emu.callbackType.exec, 0x87A5FA, 0x87A5FA,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    selected_direction = cpu("y") & 7
end, emu.callbackType.exec, 0x87A60C, 0x87A60C,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    local actor = w(0x00be) // 2
    local resolved_base = 0x34eb + actor * 0x100
    if done or actor < 0 or actor >= 10 then return end
    calls = calls + 1
    -- `$A64D-$A651` publishes the authoritative actor index in DP BE.
    -- Derive the record from that output rather than depending on callback
    -- ordering at the loop's `$A47A` re-entry.
    local b = resolved_base
    file:write(string.format(
        '{"call":%d,"actor":%d,"base":%d,' ..
        '"input":{"controller":%d,"status":%d,"upper":%d,"lower":%d,' ..
        '"upper_state":%d,"lower_state":%d,"world_x":%d,"world_y":%d,' ..
        '"z":%d,"screen_y":%d,' ..
        '"screen_x":%d,"facing":%d,"move_facing":%d,"mode":%d,' ..
        '"head_base":%d,"palette_offset":%d,"anchor_direction":%d,' ..
        '"candidate_valid":%d,"candidate_dx":%d,"candidate_dy":%d,' ..
        '"target_x":%d,"target_y":%d,' ..
        '"selected_actor":%d,"effect_gate":%d,"effect_resource":%d},' ..
        '"expected":{"attr":%d,"x":%d,"y":%d,"flags":%d,' ..
        '"lower":%d,"upper":%d,"number":%d,"head":%d,' ..
        '"head_resource":%d,"selected_direction":%d,' ..
        '"draw_facing":%d,"move_facing":%d}}\n',
        calls, (b - 0x34eb) // 0x100, b,
        w(b + 0x16), w(b + 0x28), w(b + 0x2a), w(b + 0x2c),
        w(b + 0x30), w(b + 0x32), w(b + 0x04), w(b + 0x08),
        w(b + 0x0c), w(b + 0x68),
        w(b + 0x6a), w(b + 0x4e), w(b + 0x52), w(b + 0x5e),
        w(b + 0x2e), w(b + 0xac), w(b + 0x88),
        candidate_valid and 1 or 0, candidate_dx, candidate_dy,
        w(0x3eef), w(0x3ef3),
        w(0x0940), w(0x3f33), w(0x4015),
        cpu("a"), cpu("x"), cpu("y"), w(0x0047),
        w(0x00d4), w(0x00d6), w(0x00d8), w(0x00da),
        w(0x009a), selected_direction or 0xffff, w(0x00c2), w(0x00c0)))
    if calls >= max_calls then
        done = true
        file:close()
        local sentinel = assert(io.open(out .. "/draw_calls_complete.txt", "wb"))
        sentinel:write(string.format("calls=%d\n", calls)); sentinel:close()
        emu.stop(0)
    end
end, emu.callbackType.exec, 0x87A6A4, 0x87A6A4,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv("NBA95_VECTOR_DRIVER")))
