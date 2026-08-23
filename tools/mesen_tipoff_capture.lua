-- Focused Starting Lineup -> on-court tip-off discovery trace.
-- Drives the verified Exhibition path, skips presentation cards with spaced
-- Start pulses, then stops all synthetic input when the first on-court player
-- draw at $87:A47A executes. Screenshots and traces are evidence;
-- no captured art is used by the C port.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")

local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local global_frame, title_frame, setup_frame = 0, -1, -1
local gameplay_frame = -1
local player_load_seen = false
local PRESS_TITLE_AT, PRESS_SETUP_AT = 850, 400
local LAST_GAMEPLAY_FRAME = tonumber(os.getenv("NBA95_TIPOFF_FRAMES")) or 900
local segments = {
    { name="formation", first=0, last=139 },
    { name="jump_ball", first=140, last=219 },
    { name="possession", first=220, last=399 },
    { name="live", first=400, last=LAST_GAMEPLAY_FRAME },
}
local exec_seen, wram_summary, actor_write_sites, state_write_sites = {}, {}, {}, {}
local routine_log = assert(io.open(out .. "/routine_hits.txt", "wb"))
local actor_log = assert(io.open(out .. "/actor_states.txt", "wb"))
local placement_log = assert(io.open(out .. "/placement_writes.txt", "wb"))
local draw_log = assert(io.open(out .. "/draw_origins.txt", "wb"))
local gameplay_jsonl = assert(io.open(out .. "/gameplay_rom.jsonl", "wb"))
local current_draw_actor = 0xffff
local draw_screen, previous_pad, routine_hits_frame = {}, {}, {}
local previous_actor_world, previous_ball_world = {}, nil
for pad = 0, 4 do previous_pad[pad] = 0 end
for _, segment in ipairs(segments) do exec_seen[segment.name] = {} end

local function pulse(frame, at)
    return frame >= at and frame < at + 3
end

local function shot(name)
    local file = assert(io.open(out .. "/" .. name, "wb"))
    file:write(emu.takeScreenshot()); file:close()
end

local function dump_mem(name, mem_type, size)
    local chunks = {}
    for i = 0, size - 1 do
        chunks[#chunks + 1] = string.char(emu.read(i, mem_type, false) or 0)
    end
    local file = assert(io.open(out .. "/" .. name, "wb"))
    file:write(table.concat(chunks)); file:close()
end

local function current_segment()
    if gameplay_frame < 0 then return nil end
    for _, segment in ipairs(segments) do
        if gameplay_frame >= segment.first and gameplay_frame <= segment.last then
            return segment.name
        end
    end
    return nil
end

emu.addMemoryCallback(function()
    if title_frame < 0 then title_frame = 0 end
end, emu.callbackType.exec, 0x80E1B1, 0x80E1B1,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    if title_frame >= PRESS_TITLE_AT and setup_frame < 0 then
        setup_frame = 0
        log:write(string.format("entered setup global=%d\n", global_frame)); log:flush()
    end
end, emu.callbackType.exec, 0x80A2BF, 0x80A2BF,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    if not player_load_seen then
        player_load_seen = true
        log:write(string.format(
            "gameplay player records begin global=%d setup=%d pc=$86:D7B8\n",
            global_frame, setup_frame)); log:flush()
    end
end, emu.callbackType.exec, 0x86D7B8, 0x86D7B8,
    emu.cpuType.snes, emu.memType.snesMemory)

-- $87:A47A is the first per-player sprite draw preparation entry. Unlike the
-- much earlier roster/appearance preload, it does not execute until the live
-- on-court player records are being rendered.
emu.addMemoryCallback(function()
    if gameplay_frame < 0 then
        gameplay_frame = 0
        log:write(string.format(
            "on-court player draw begins global=%d setup=%d pc=$87:A47A\n",
            global_frame, setup_frame)); log:flush()
    end
end, emu.callbackType.exec, 0x87A47A, 0x87A47A,
    emu.cpuType.snes, emu.memType.snesMemory)

local probes = {
    [0x82ff3f]="$82:FF3F script->scheduler", [0x809f0f]="$80:9F0F scheduler",
    [0x859763]="$85:9763", [0x859767]="$85:9767", [0x859770]="$85:9770",
    [0x85b100]="$85:B100", [0x85b130]="$85:B130", [0x85b1b4]="$85:B1B4",
    [0x85b1cb]="$85:B1CB", [0x85b21c]="$85:B21C", [0x85b245]="$85:B245",
    [0x85b359]="$85:B359", [0x85b377]="$85:B377",
    [0x869b80]="$86:9B80", [0x869846]="$86:9846",
    [0x86cf49]="$86:CF49", [0x87b47a]="$87:B47A",
    [0x86d3f9]="$86:D3F9", [0x86b04c]="$86:B04C",
    [0x86ecf4]="$86:ECF4", [0x87b3bd]="$87:B3BD",
    [0x87b538]="$87:B538", [0x87b555]="$87:B555", [0x87b832]="$87:B832",
}
for address, name in pairs(probes) do
    emu.addMemoryCallback(function()
        if gameplay_frame >= 90 and gameplay_frame <= 260 then
            routine_log:write(string.format("f=%03d %s\n", gameplay_frame, name))
        end
        if gameplay_frame >= 0 then routine_hits_frame[address] = true end
    end, emu.callbackType.exec, address, address,
        emu.cpuType.snes, emu.memType.snesMemory)
end

local function word(address)
    local lo = emu.read(address, emu.memType.snesWorkRam, false) or 0
    local hi = emu.read(address + 1, emu.memType.snesWorkRam, false) or 0
    return lo + hi * 0x100
end

local function signed_word(address)
    local value = word(address)
    return value >= 0x8000 and value - 0x10000 or value
end

local function trace_draw_frame()
    return (gameplay_frame >= 89 and gameplay_frame <= 92) or
           (gameplay_frame >= 218 and gameplay_frame <= 222)
end

emu.addMemoryCallback(function()
    local state = emu.getState()
    local direct = state["cpu.d"] or 0
    local list_index = word((direct + 0x96) & 0xffff)
    current_draw_actor = word(0x7e44 + list_index)
    if trace_draw_frame() then
        draw_log:write(string.format(
            "f=%03d prepare list=%04X actor=%04X slot=%d world=%04X,%04X,%04X\n",
            gameplay_frame, list_index, current_draw_actor,
            current_draw_actor >= 0x34eb and
                math.floor((current_draw_actor - 0x34eb) / 0x100) or -1,
            word(current_draw_actor + 4), word(current_draw_actor + 8),
            word(current_draw_actor + 12)))
    end
end, emu.callbackType.exec, 0x87a47a, 0x87a47a,
    emu.cpuType.snes, emu.memType.snesMemory)


emu.addMemoryCallback(function()
    local state = emu.getState()
    local direct = state["cpu.d"] or 0
    local slot = current_draw_actor >= 0x34eb and
        math.floor((current_draw_actor - 0x34eb) / 0x100) or -1
    if slot >= 0 and slot <= 9 then
        draw_screen[slot] = {
            x = state["cpu.x"] or -32768,
            y = signed_word((direct + 0xa0) & 0xffff)
        }
    end
    if trace_draw_frame() then
        local function dpword(offset)
            return word((direct + offset) & 0xffff)
        end
        draw_log:write(string.format(
            "f=%03d actor=%04X res=%04X dp8e=%04X dp90=%04X dp92=%04X dp94=%04X " ..
            "dpa0=%04X dpc0=%04X dpc2=%04X a=%04X x=%04X y=%04X\n",
            gameplay_frame, current_draw_actor, dpword(0x9e), dpword(0x8e), dpword(0x90),
            dpword(0x92), dpword(0x94), dpword(0xa0), dpword(0xc0),
            dpword(0xc2), state["cpu.a"] or 0, state["cpu.x"] or 0,
            state["cpu.y"] or 0))
    end
end, emu.callbackType.exec, 0x80ad92, 0x80ad92,
    emu.cpuType.snes, emu.memType.snesMemory)

local function dump_actor_states(frame)
    actor_log:write(string.format("f=%03d", frame))
    -- Ten player records are consecutive $100-byte structures at $34EB;
    -- the identically-shaped ball record immediately follows at $3EEB.
    for actor = 0, 10 do
        local base = 0x34eb + actor * 0x100
        actor_log:write(string.format(
            " a%02d[x=%04X y=%04X z=%04X act=%04X anim=%04X/%04X vel=%04X,%04X,%04X fl=%04X]",
            actor, word(base + 4), word(base + 8), word(base + 12),
            word(base + 0x18), word(base + 0x30), word(base + 0x32),
            word(base + 0x38), word(base + 0x3a), word(base + 0x3c), word(base + 0x28)))
    end
    actor_log:write("\n")
end

local function dump_gameplay_jsonl(frame)
    local held, pressed, released = {}, {}, {}
    for pad = 0, 4 do
        held[pad] = word(0x0576 + pad * 2)
        pressed[pad] = held[pad] & (~previous_pad[pad] & 0xffff)
        released[pad] = previous_pad[pad] & (~held[pad] & 0xffff)
        previous_pad[pad] = held[pad]
    end
    local phase = frame < 140 and 0 or frame < 220 and 1 or
                  frame < 400 and 2 or 3
    gameplay_jsonl:write(string.format(
        "{\"source\":\"rom\",\"frame\":%d,\"scene_frame\":%d," ..
        "\"simulation_tick\":%d,\"phase\":%d," ..
        "\"input\":{\"pressed\":%u,\"held\":%u,\"released\":%u}," ..
        "\"controllers\":{\"active_raw\":%d,\"selected_raw\":%d," ..
        "\"held_raw\":[%u,%u,%u,%u,%u]," ..
        "\"assignment_raw\":[%u,%u,%u,%u,%u]," ..
        "\"repeat_raw\":[%u,%u,%u,%u,%u]},",
        frame, frame, frame, phase, pressed[0], held[0], released[0],
        signed_word(0x1615), signed_word(0x095e),
        held[0], held[1], held[2], held[3], held[4],
        word(0x08d4), word(0x08d6), word(0x08d8), word(0x08da), word(0x08dc),
        word(0x15f7), word(0x15f9), word(0x15fb), word(0x15fd), word(0x15ff)))
    gameplay_jsonl:write(string.format(
        "\"control\":{\"actor\":%d,\"side_raw\":%d," ..
        "\"initial_slot_raw\":%d,\"selected_slot_raw\":%d," ..
        "\"actor_pointer_raw\":%u}," ..
        "\"possession\":{\"actor\":%d,\"team\":%d," ..
        "\"candidate_raw\":%d,\"play_code_raw\":%u," ..
        "\"rng_state_raw\":65535}," ..
        "\"camera\":{\"x\":%d,\"y\":%d,\"routine\":%u," ..
        "\"raw_085c\":%u,\"raw_085e\":%u,\"raw_0860\":%u," ..
        "\"raw_0862\":%u,\"raw_086c\":%u,\"raw_086e\":%u," ..
        "\"raw_0874\":%u,\"raw_0876\":%u,\"raw_0878\":%u," ..
        "\"raw_087a\":%u},",
        signed_word(0x093e), signed_word(0x093a), signed_word(0x0954),
        signed_word(0x093e), word(0x0940), signed_word(0x0946),
        signed_word(0x093a), signed_word(0x0946), word(0x0996),
        signed_word(0x085c), signed_word(0x0860), 0x858ee6,
        word(0x085c), word(0x085e), word(0x0860), word(0x0862),
        word(0x086c), word(0x086e), word(0x0874), word(0x0876),
        word(0x0878), word(0x087a)))
    local ball = 0x3eeb
    local ball_x, ball_y, ball_z = signed_word(ball + 4),
        signed_word(ball + 8), signed_word(ball + 12)
    local ball_vx, ball_vy, ball_vz = 0, 0, 0
    if previous_ball_world then
        ball_vx = ball_x - previous_ball_world.x
        ball_vy = ball_y - previous_ball_world.y
        ball_vz = ball_z - previous_ball_world.z
    end
    gameplay_jsonl:write(string.format(
        "\"collision\":{\"a\":-1,\"b\":-1,\"routine\":0}," ..
        "\"ball\":{\"x\":%d,\"y\":%d,\"z\":%d," ..
        "\"screen_x\":-32768,\"screen_y\":-32768," ..
        "\"vx\":%d,\"vy\":%d,\"vz\":%d,\"owner\":%d," ..
        "\"state\":%u,\"flags_raw\":%u,\"routine\":%u}," ..
        "\"routines\":{\"controller\":%u,\"selection\":%u," ..
        "\"possession\":%u},\"routine_hits\":[",
        ball_x, ball_y, ball_z, ball_vx, ball_vy, ball_vz,
        signed_word(0x0946), word(ball + 0x18),
        word(ball + 0x28), 0x86e054, 0x80cb8f, 0x85c37d,
        frame >= 200 and 0x86d3f9 or 0))
    local hit_addresses = {}
    for address in pairs(routine_hits_frame) do hit_addresses[#hit_addresses + 1] = address end
    table.sort(hit_addresses)
    for index, address in ipairs(hit_addresses) do
        gameplay_jsonl:write(string.format("%s%u", index > 1 and "," or "", address))
    end
    gameplay_jsonl:write("],\"actors\":[")
    for actor = 0, 9 do
        local base = 0x34eb + actor * 0x100
        local screen = draw_screen[actor]
        local actor_x, actor_y, actor_z = signed_word(base + 4),
            signed_word(base + 8), signed_word(base + 12)
        local actor_vx, actor_vy, actor_vz = 0, 0, 0
        if previous_actor_world[actor] then
            actor_vx = actor_x - previous_actor_world[actor].x
            actor_vy = actor_y - previous_actor_world[actor].y
            actor_vz = actor_z - previous_actor_world[actor].z
        end
        gameplay_jsonl:write(string.format(
            "%s{\"id\":%d,\"team\":%d,\"roster\":%d," ..
            "\"control\":%d,\"visible\":%s," ..
            "\"x\":%d,\"y\":%d,\"z\":%d," ..
            "\"screen_x\":%d,\"screen_y\":%d," ..
            "\"vx\":%d,\"vy\":%d,\"vz\":%d," ..
            "\"direction\":%u,\"animation\":%u," ..
            "\"lower_animation\":%u,\"ai_state\":%u," ..
            "\"ai_target\":255,\"actor_routine\":%u," ..
            "\"ai_routine\":%u,\"raw\":{\"base\":%u,\"id\":%u," ..
            "\"action\":%u,\"flags\":%u,\"upper_resource\":%u," ..
            "\"lower_resource\":%u,\"head_resource\":%u," ..
            "\"motion_38\":%u,\"motion_3a\":%u,\"motion_3c\":%u," ..
            "\"direction_4e\":%u,\"direction_50\":%u," ..
            "\"direction_52\":%u,\"control_mode\":%u," ..
            "\"control_mode_saved\":%u,\"side_group\":%u," ..
            "\"assignment_base\":%u,\"assignment_current\":%u," ..
            "\"assignment_alternate\":%u,\"assignment_distance\":%u," ..
            "\"assignment_direction\":%u,\"pair_distance\":%u," ..
            "\"reaction_threshold\":%u,\"upper_restart\":%u," ..
            "\"lower_restart\":%u,\"upper_phase\":%u," ..
            "\"lower_phase\":%u,\"behavior_flags\":%u," ..
            "\"palette\":%u}}",
            actor > 0 and "," or "", actor, actor >= 5 and 1 or 0,
            actor % 5, signed_word(0x093e) == actor and 1 or 0,
            screen and "true" or "false", actor_x, actor_y, actor_z,
            screen and screen.x or -32768, screen and screen.y or -32768,
            actor_vx, actor_vy, actor_vz,
            word(base + 0x4e), word(base + 0x30), word(base + 0x32),
            word(base + 0x5e), 0x80ad92, 0x87a160, base, word(base),
            word(base + 0x18), word(base + 0x28), word(base + 0x2a),
            word(base + 0x2c), word(base + 0x2e), word(base + 0x38),
            word(base + 0x3a), word(base + 0x3c), word(base + 0x4e),
            word(base + 0x50), word(base + 0x52), word(base + 0x5e),
            word(base + 0x84), word(base + 0x6e), word(base + 0x76),
            word(base + 0x74), word(base + 0x78), word(base + 0x8e),
            word(base + 0x86), word(base + 0x8a), word(base + 0x60),
            word(base + 0x46), word(base + 0x48), word(base + 0x3a),
            word(base + 0x44), word(base + 0x7e), word(base + 0xac)))
        previous_actor_world[actor] = { x=actor_x, y=actor_y, z=actor_z }
    end
    gameplay_jsonl:write("]}\n")
    gameplay_jsonl:flush()
    previous_ball_world = { x=ball_x, y=ball_y, z=ball_z }
    routine_hits_frame = {}
end

for actor = 0, 10 do
    for _, offset in ipairs({4, 5, 8, 9, 12, 13}) do
        local address = 0x34eb + actor * 0x100 + offset
        emu.addMemoryCallback(function(_, value)
            if player_load_seen and gameplay_frame <= 1 then
                local state = emu.getState()
                placement_log:write(string.format(
                    "gf=%d a=%02d off=%02X val=%02X pc=$%02X:%04X dbr=%02X\n",
                    gameplay_frame, actor, offset, value,
                    state["cpu.k"] or 0, state["cpu.pc"] or 0,
                    state["cpu.dbr"] or 0))
            end
        end, emu.callbackType.write, address, address,
            emu.cpuType.snes, emu.memType.snesWorkRam)
    end
end

-- Coordinate writes, rather than the actor +$38/+$3A/+$3C animation words,
-- identify the real movement/ball integrators. Aggregate the writer PCs so a
-- long CPU-only run remains compact enough to feed back into headless Ghidra.
local function record_write_site(collection, key, value)
    local state = emu.getState()
    local pc = (state["cpu.k"] or 0) * 0x10000 + (state["cpu.pc"] or 0)
    local full_key = string.format("%06X:%s", pc, key)
    local item = collection[full_key]
    if not item then
        item = { pc=pc, key=key, first=gameplay_frame, last=gameplay_frame,
                 count=0, first_value=value, last_value=value }
        collection[full_key] = item
    end
    item.last, item.count, item.last_value = gameplay_frame, item.count + 1, value
end

for actor = 0, 10 do
    for _, offset in ipairs({4, 5, 8, 9, 12, 13}) do
        local address = 0x34eb + actor * 0x100 + offset
        emu.addMemoryCallback(function(_, value)
            if gameplay_frame >= 190 then
                record_write_site(actor_write_sites,
                    string.format("actor=%02d off=%02X", actor, offset), value)
            end
        end, emu.callbackType.write, address, address,
            emu.cpuType.snes, emu.memType.snesWorkRam)
    end
end

for _, address in ipairs({0x093a, 0x093b, 0x093e, 0x093f, 0x0940, 0x0941,
                           0x0946, 0x0947, 0x0996, 0x0997}) do
    emu.addMemoryCallback(function(_, value)
        if gameplay_frame >= 190 then
            record_write_site(state_write_sites,
                string.format("wram=%04X", address), value)
        end
    end, emu.callbackType.write, address, address,
        emu.cpuType.snes, emu.memType.snesWorkRam)
end

for bank = 0x80, 0x8f do
    emu.addMemoryCallback(function(address)
        local segment = current_segment()
        if segment then exec_seen[segment][address] = true end
    end, emu.callbackType.exec, bank * 0x10000 + 0x8000,
        bank * 0x10000 + 0xffff, emu.cpuType.snes, emu.memType.snesMemory)
end

emu.addMemoryCallback(function(address, value)
    if gameplay_frame < 0 then return end
    local item = wram_summary[address]
    if not item then
        item = { first=gameplay_frame, last=gameplay_frame, count=0,
                 min=value, max=value, value=value }
        wram_summary[address] = item
    end
    item.last, item.count, item.value = gameplay_frame, item.count + 1, value
    if value < item.min then item.min = value end
    if value > item.max then item.max = value end
end, emu.callbackType.write, 0x0000, 0x7fff,
    emu.cpuType.snes, emu.memType.snesWorkRam)

emu.addEventCallback(function()
    local input = {}
    if setup_frame < 0 then
        if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
    elseif pulse(setup_frame, PRESS_SETUP_AT) then
        input.start = true
    elseif gameplay_frame < 0 and setup_frame >= 650 and
           ((setup_frame - 650) % 200) < 3 then
        input.start = true
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    global_frame = global_frame + 1
    if title_frame >= 0 and setup_frame < 0 then title_frame = title_frame + 1 end
    if setup_frame >= 0 then
        local frame = setup_frame
        setup_frame = setup_frame + 1
        -- Game Setup persists Mode; force the verified Exhibition working word
        -- before Start so an old Mesen save cannot route through Season.
        if frame >= 300 and frame < PRESS_SETUP_AT then
            emu.write(0x16fb, 0, emu.memType.snesWorkRam)
            emu.write(0x16fc, 0, emu.memType.snesWorkRam)
        end
    end

    if gameplay_frame < 0 then
        assert(setup_frame < 8000, "Timed out before gameplay player initialization")
        return
    end

    local frame = gameplay_frame
    gameplay_frame = gameplay_frame + 1
    if frame <= 300 then dump_actor_states(frame) end
    dump_gameplay_jsonl(frame)
    if frame % 5 == 0 then shot(string.format("tipoff_%04d.png", frame)) end
    if frame == 0 or frame == 140 or frame == 220 or frame == 400 or
       frame == LAST_GAMEPLAY_FRAME then
        dump_mem(string.format("tipoff_%04d_vram.bin", frame),
            emu.memType.snesVideoRam, 0x10000)
        dump_mem(string.format("tipoff_%04d_cgram.bin", frame),
            emu.memType.snesCgRam, 0x200)
        dump_mem(string.format("tipoff_%04d_oam.bin", frame),
            emu.memType.snesSpriteRam, 0x220)
    end

    if frame >= LAST_GAMEPLAY_FRAME then
        for _, segment in ipairs(segments) do
            local addresses = {}
            for address in pairs(exec_seen[segment.name]) do
                addresses[#addresses + 1] = address
            end
            table.sort(addresses)
            local file = assert(io.open(out .. "/exec_" .. segment.name .. ".txt", "wb"))
            local first, previous = nil, nil
            for _, address in ipairs(addresses) do
                if not first then first, previous = address, address
                elseif address == previous + 1 then previous = address
                else
                    file:write(string.format("%06X-%06X\n", first, previous))
                    first, previous = address, address
                end
            end
            if first then file:write(string.format("%06X-%06X\n", first, previous)) end
            file:close()
        end

        local addresses = {}
        for address in pairs(wram_summary) do addresses[#addresses + 1] = address end
        table.sort(addresses)
        local wram = assert(io.open(out .. "/wram_write_summary.txt", "wb"))
        for _, address in ipairs(addresses) do
            local item = wram_summary[address]
            wram:write(string.format("%04X first=%d last=%d count=%d min=%02X max=%02X final=%02X\n",
                address, item.first, item.last, item.count, item.min, item.max, item.value))
        end
        wram:close()
        local function write_sites(name, collection)
            local keys = {}
            for key in pairs(collection) do keys[#keys + 1] = key end
            table.sort(keys)
            local file = assert(io.open(out .. "/" .. name, "wb"))
            for _, key in ipairs(keys) do
                local item = collection[key]
                file:write(string.format(
                    "pc=$%02X:%04X %s first=%d last=%d count=%d firstval=%02X lastval=%02X\n",
                    math.floor(item.pc / 0x10000), item.pc & 0xffff, item.key,
                    item.first, item.last, item.count,
                    item.first_value, item.last_value))
            end
            file:close()
        end
        write_sites("actor_coordinate_write_sites.txt", actor_write_sites)
        write_sites("gameplay_state_write_sites.txt", state_write_sites)
        actor_log:close(); routine_log:close(); placement_log:close(); draw_log:close()
        gameplay_jsonl:close()
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write(string.format("frames=%d\n", gameplay_frame)); done:close()
        log:write("capture done\n"); log:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
