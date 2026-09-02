-- Capture every early Tipoff frame and court-upload inputs for one home team.
-- Drives the ordinary ROM flow and writes only raw PPU memories plus a PNG
-- oracle. Runtime assets are decoded from VRAM/CGRAM, never from the PNG.
local out = os.getenv("NBA95_CAPTURE_DIR")
local requested_team = tonumber(os.getenv("NBA95_HOME_TEAM"))
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
assert(requested_team and requested_team >= 0 and requested_team <= 28,
    "NBA95_HOME_TEAM must be in [0, 28]")

local home = assert(io.open(out .. "/observed-script-data-folder.txt", "wb"))
home:write(emu.getScriptDataFolder() .. "\n"); home:close()
local requested_frames = tonumber(os.getenv("NBA95_COURT_FRAMES")) or 180
assert(requested_frames >= 160 and requested_frames <= 400)
local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local uploads = assert(io.open(out .. "/uploads.jsonl", "wb"))
local upload_index = 0
local global_frame, title_frame, setup_frame = 0, -1, -1
local team_select_entered, nav_done, nav_pressing = false, false, false
local nav_current, nav_next_at, confirm_team_at = -1, 700, -1
local lineup_started, oncourt_frame, captured = false, -1, false
local graphics_calls = {}
local PRESS_TITLE_AT, PRESS_SETUP_AT = 850, 400
local TEAM_WORD = 0x16fd

local function pulse(frame, at) return frame >= at and frame < at + 3 end

local function dump_mem(name, mem_type, size)
    local chunks = {}
    for i = 0, size - 1 do
        chunks[#chunks + 1] = string.char(emu.read(i, mem_type, false) or 0)
    end
    local file = assert(io.open(out .. "/" .. name, "wb"))
    file:write(table.concat(chunks)); file:close()
end

local function shot(name)
    local pixels = emu.getScreenBuffer()
    assert(#pixels == 256 * 239, "unexpected native screen dimensions")
    local chunks = {}
    for i, color in ipairs(pixels) do
        chunks[i] = string.char((color >> 16) & 255, (color >> 8) & 255, color & 255)
    end
    local file = assert(io.open(out .. "/" .. name:gsub("%.png$", ".rgb"), "wb"))
    file:write(table.concat(chunks)); file:close()
end

local function word(address)
    return emu.read(address, emu.memType.snesWorkRam) |
           (emu.read(address + 1, emu.memType.snesWorkRam) << 8)
end

local function state_dump(name)
    local state, keys = emu.getState(), {}
    for key in pairs(state) do
        if key:sub(1, 4) == "ppu." then keys[#keys + 1] = key end
    end
    table.sort(keys)
    local file = assert(io.open(out .. "/" .. name .. "_state.txt", "wb"))
    for _, key in ipairs(keys) do file:write(key .. "=" .. tostring(state[key]) .. "\n") end
    for name, address in pairs({home=0x46eb, camera_x=0x85c, camera_y=0x860,
            coarse_x=0x86c, coarse_y=0x86e, destination=0x874,
            source=0x876, map_address=0xea, map_bank=0xec, descriptor=0xee}) do
        file:write(name .. "=" .. word(address) .. "\n")
    end
    file:close()
end

emu.addMemoryCallback(function()
    local state = emu.getState()
    upload_index = upload_index + 1
    uploads:write(string.format(
        '{"frame":%d,"pc":%d,"home":%d,"length":%d,"vram_word":%d,"source":%d}\n',
        global_frame, 0x84e592, word(0x46eb), state["cpu.x"],
        state["cpu.y"], word(0x0c) | (word(0x0e) << 16)))
    uploads:flush()
    local data = {}
    for i = 0, state["cpu.x"] - 1 do
        data[#data + 1] = string.char(emu.read(0x8fee + i, emu.memType.snesWorkRam))
    end
    local file = assert(io.open(out .. string.format("/court_upload_%02d.bin", upload_index), "wb"))
    file:write(table.concat(data)); file:close()
end, emu.callbackType.exec, 0x84e592, 0x84e592, emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function() if title_frame < 0 then title_frame = 0 end end,
    emu.callbackType.exec, 0x80E1B1, 0x80E1B1,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    if title_frame >= PRESS_TITLE_AT and setup_frame < 0 then setup_frame = 0 end
end, emu.callbackType.exec, 0x80A2BF, 0x80A2BF,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function() team_select_entered = true end,
    emu.callbackType.exec, 0x82809A, 0x82809A,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    lineup_started = true
    log:write(string.format("lineup home=%d setup=%d\n",
        emu.read(TEAM_WORD, emu.memType.snesWorkRam, false) or -1, setup_frame))
    log:flush()
end, emu.callbackType.exec, 0x87BE92, 0x87BE92,
    emu.cpuType.snes, emu.memType.snesMemory)

-- First live player compositor call after the lineup sequence.
emu.addMemoryCallback(function()
    if oncourt_frame < 0 then
        oncourt_frame = 0
        log:write(string.format("entered court setup=%d pc=$87:A47A\n", setup_frame))
        log:flush()
    end
end, emu.callbackType.exec, 0x87A47A, 0x87A47A,
    emu.cpuType.snes, emu.memType.snesMemory)

for _, address in ipairs({ 0x80C62B, 0x80BD1B, 0x80BBA8 }) do
    emu.addMemoryCallback(function(hit)
        if nav_done and not captured then
            local state = emu.getState()
            graphics_calls[#graphics_calls + 1] = string.format(
                "frame=%d pc=%06X a=%04X x=%04X y=%04X d=%04X dbr=%02X",
                setup_frame, hit, state["cpu.a"] or 0, state["cpu.x"] or 0,
                state["cpu.y"] or 0, state["cpu.d"] or 0,
                state["cpu.dbr"] or 0)
        end
    end, emu.callbackType.exec, address, address,
        emu.cpuType.snes, emu.memType.snesMemory)
end

emu.addEventCallback(function()
    local input = {}
    if setup_frame < 0 then
        if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
    elseif pulse(setup_frame, PRESS_SETUP_AT) then
        input.start = true
    elseif not nav_done and setup_frame >= nav_next_at then
        nav_pressing = true; input.left = true
    elseif nav_done and oncourt_frame < 0 and setup_frame >= confirm_team_at then
        -- Start advances Team Select/Player Setup and each presentation card.
        local cadence = lineup_started and 90 or 200
        if ((setup_frame - confirm_team_at) % cadence) < 3 then input.start = true end
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    global_frame = global_frame + 1
    if title_frame >= 0 and setup_frame < 0 then title_frame = title_frame + 1 end
    if setup_frame < 0 then return end
    local frame = setup_frame
    setup_frame = setup_frame + 1

    -- Pin the persistent Game Setup mode to Exhibition.
    if frame >= 300 and frame < PRESS_SETUP_AT then
        emu.write(0x16fb, 0, emu.memType.snesWorkRam)
        emu.write(0x16fc, 0, emu.memType.snesWorkRam)
    end
    if frame == 650 then
        assert(team_select_entered, "Team Select was not entered")
        nav_current = emu.read(TEAM_WORD, emu.memType.snesWorkRam, false) or -1
        if nav_current == requested_team then
            nav_done, confirm_team_at = true, 710
        end
    end
    if nav_pressing then
        local observed = emu.read(TEAM_WORD, emu.memType.snesWorkRam, false) or -1
        if observed ~= nav_current then
            nav_current, nav_pressing = observed, false
            if observed == requested_team then
                nav_done, confirm_team_at = true, frame + 60
            else
                nav_next_at = frame + 20
            end
        end
    end
    if oncourt_frame >= 0 then
        oncourt_frame = oncourt_frame + 1
        -- Capture the initial selected-map stream and the settled jump-ball
        -- view; raw PPU checkpoints independently verify the floor layout.
        if oncourt_frame <= requested_frames then
            shot(string.format("frame_%04d.png", oncourt_frame))
        end
        if oncourt_frame == 1 or oncourt_frame == 20 or oncourt_frame == 60 or
           oncourt_frame == 90 or oncourt_frame == 140 or oncourt_frame == 160 or
           oncourt_frame == requested_frames then
            local prefix = string.format("frame_%04d", oncourt_frame)
            dump_mem(prefix .. "_vram.bin", emu.memType.snesVideoRam, 0x10000)
            dump_mem(prefix .. "_cgram.bin", emu.memType.snesCgRam, 0x200)
            state_dump(prefix)
        end
        if oncourt_frame == requested_frames and not captured then
            local observed = emu.read(TEAM_WORD, emu.memType.snesWorkRam, false) or -1
            assert(observed == requested_team, "home selector changed before court capture")
            dump_mem("court_vram.bin", emu.memType.snesVideoRam, 0x10000)
            dump_mem("court_cgram.bin", emu.memType.snesCgRam, 0x200)
            shot("court.png")
            local calls = assert(io.open(out .. "/graphics_calls.txt", "wb"))
            calls:write(table.concat(graphics_calls, "\n") .. "\n"); calls:close()
            local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
            done:write(string.format("requested=%d observed=%d pc=87:A47A\n",
                requested_team, observed)); done:close()
            captured = true; log:write("capture done\n"); log:close(); uploads:close(); emu.stop(0)
        end
    end
    assert(frame < 12000, "Timed out before gameplay court capture")
end, emu.eventType.endFrame)
