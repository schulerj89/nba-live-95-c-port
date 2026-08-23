-- Extract one team's five ROM-built Starting Lineup portraits.
--
-- The harness drives the ordinary Title -> Game Setup -> Team Select path,
-- seeds the live right-team selector immediately before a real D-pad redraw,
-- confirms that selection, then waits for $87:BE92 to construct each card.
-- Only raw VRAM/CGRAM/OAM state is saved; screenshots are diagnostic evidence.
local out = os.getenv("NBA95_CAPTURE_DIR")
local requested_team = tonumber(os.getenv("NBA95_INTRO_TEAM"))
local requested_side = os.getenv("NBA95_INTRO_SIDE") or "home"
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
assert(requested_team and requested_team >= 0 and requested_team <= 28,
    "NBA95_INTRO_TEAM must be in [0, 28]")
assert(requested_side == "home" or requested_side == "away",
    "NBA95_INTRO_SIDE must be home or away")
local capture_away = requested_side == "away"

local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local global_frame, title_frame, setup_frame = 0, -1, -1
local team_select_entered = false
local selected_team = -1
local lineup_card = 0
local pending_capture = -1
local stopped_advancing = false
local nav_done, nav_pressing, selector_captured = false, false, false
local nav_current, nav_next_at, confirm_team_at = -1, 700, -1
local PRESS_TITLE_AT, PRESS_SETUP_AT = 850, 400
local DEFAULT_TEAM = capture_away and 3 or 18
local TEAM_WORD = capture_away and 0x16fb or 0x16fd
local FIRST_CAPTURE_CARD = capture_away and 1 or 6
local LAST_CAPTURE_CARD = capture_away and 5 or 10
local nav_direction = "left"

local function pulse(frame, at)
    return frame >= at and frame < at + 3
end

local function dump_mem(name, mem_type, size)
    local chunks = {}
    for i = 0, size - 1 do
        chunks[#chunks + 1] = string.char(emu.read(i, mem_type, false) or 0)
    end
    local file = assert(io.open(out .. "/" .. name, "wb"))
    file:write(table.concat(chunks)); file:close()
end

local function shot(name)
    local file = assert(io.open(out .. "/" .. name, "wb"))
    file:write(emu.takeScreenshot()); file:close()
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
    team_select_entered = true
    log:write(string.format("entered team select global=%d setup=%d\n",
        global_frame, setup_frame)); log:flush()
end, emu.callbackType.exec, 0x82809A, 0x82809A,
    emu.cpuType.snes, emu.memType.snesMemory)

-- Ghidra label: $87:BE92, the repeated Starting Lineup card constructor.
emu.addMemoryCallback(function()
    lineup_card = lineup_card + 1
    stopped_advancing = true
    local lineup_team = emu.read(TEAM_WORD, emu.memType.snesWorkRam, false) or -1
    if lineup_card == 1 and lineup_team ~= requested_team then
        local failed = assert(io.open(out .. "/capture_failed.txt", "wb"))
        failed:write(string.format("requested=%d lineup=%d\n",
            requested_team, lineup_team)); failed:close()
        log:write("capture failed: lineup team mismatch\n"); log:close()
        emu.stop(1); return
    end
    if lineup_card >= FIRST_CAPTURE_CARD and lineup_card <= LAST_CAPTURE_CARD then
        pending_capture = 20
    end
    log:write(string.format("lineup card=%d setup=%d team=%d side=%s\n",
        lineup_card, setup_frame, lineup_team, requested_side))
    log:flush()
end, emu.callbackType.exec, 0x87BE92, 0x87BE92,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addEventCallback(function()
    local input = {}
    if setup_frame < 0 then
        if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
    elseif pulse(setup_frame, PRESS_SETUP_AT) then
        input.start = true
    elseif setup_frame >= 0 then
        -- Toggle to the visitor selector before beginning its state-driven walk.
        if capture_away and pulse(setup_frame, 650) then
            input.l = true
        end
        if not nav_done and setup_frame >= nav_next_at then
            nav_pressing = true
            input[nav_direction] = true
        end
        if nav_done and not stopped_advancing and setup_frame >= confirm_team_at and
               ((setup_frame - confirm_team_at) % 200) < 3 then
            input.start = true
        end
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    global_frame = global_frame + 1
    if title_frame >= 0 and setup_frame < 0 then title_frame = title_frame + 1 end
    if setup_frame < 0 then return end
    local frame = setup_frame
    setup_frame = setup_frame + 1

    -- The ROM persists the Game Setup Mode. Pin its verified working value to
    -- Exhibition before Start so an existing Mesen save cannot route this
    -- extraction through Season's Team Schedule screen.
    if frame >= 300 and frame < PRESS_SETUP_AT then
        emu.write(0x16fb, 0, emu.memType.snesWorkRam)
        emu.write(0x16fc, 0, emu.memType.snesWorkRam)
    end

    if frame == 650 then
        assert(team_select_entered,
            "Team Select $82:809A was not entered; refusing mislabeled capture")
        nav_current = emu.read(TEAM_WORD, emu.memType.snesWorkRam, false) or -1
        log:write(string.format("navigate default=%d observed=%d requested=%d direction=%s\n",
            DEFAULT_TEAM, nav_current, requested_team, nav_direction)); log:flush()
        if nav_current == requested_team then
            nav_done, selected_team, confirm_team_at = true, requested_team, 710
        end
    end

    if nav_pressing then
        local observed = emu.read(TEAM_WORD, emu.memType.snesWorkRam, false) or -1
        if observed ~= nav_current then
            log:write(string.format("selector step frame=%d from=%d to=%d\n",
                frame, nav_current, observed)); log:flush()
            nav_current, nav_pressing = observed, false
            if observed == requested_team then
                nav_done, selected_team, confirm_team_at = true, observed, frame + 60
            else
                nav_next_at = frame + 20
            end
        end
    end

    if nav_done and not selector_captured then
        selector_captured = true
        log:write(string.format("selected requested=%d observed=%d redraw=%d\n",
            requested_team, selected_team,
            emu.read(0x1695, emu.memType.snesWorkRam, false) or -1)); log:flush()
        shot("team_select.png")
    end
    if confirm_team_at >= 0 and (frame == confirm_team_at + 200 or
       frame == confirm_team_at + 400 or frame == confirm_team_at + 600) then
        shot(string.format("handoff_%04d.png", frame))
    end

    if pending_capture >= 0 then
        pending_capture = pending_capture - 1
        if pending_capture == 0 then
            local slot = lineup_card - FIRST_CAPTURE_CARD
            dump_mem(string.format("slot_%d_vram.bin", slot),
                emu.memType.snesVideoRam, 0x10000)
            dump_mem(string.format("slot_%d_cgram.bin", slot),
                emu.memType.snesCgRam, 0x200)
            dump_mem(string.format("slot_%d_oam.bin", slot),
                emu.memType.snesSpriteRam, 0x220)
            shot(string.format("slot_%d.png", slot))
            log:write(string.format("captured slot=%d setup=%d\n", slot, frame));
            log:flush()
            pending_capture = -1
            if lineup_card == LAST_CAPTURE_CARD then
                local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
                done:write(string.format("requested=%d observed=%d side=%s cards=%d\n",
                    requested_team, selected_team, requested_side, lineup_card)); done:close()
                log:write("capture done\n"); log:close(); emu.stop(0)
            end
        end
    end

    assert(frame < 9000, "Timed out before five home-team lineup portraits")
end, emu.eventType.endFrame)
