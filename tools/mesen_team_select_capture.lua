-- Reproducible Exhibition Team Select capture.
-- Drives Title -> Game Setup -> Team Select and records the transition,
-- settled PPU memory, controller-owned WRAM, and executed CPU ranges.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")

local single_button = os.getenv("NBA95_TEAM_PROBE_BUTTON")
local logo_capture = os.getenv("NBA95_TEAM_LOGOS") == "1"
local alignment_capture = os.getenv("NBA95_TEAM_ALIGNMENT") == "1"
local panel_anim_capture = os.getenv("NBA95_TEAM_PANEL_ANIM") == "1"
local valid_buttons = { up=true, down=true, left=true, right=true, a=true, b=true,
    x=true, y=true, l=true, r=true, select=true, start=true }
assert(not single_button or valid_buttons[single_button],
    "NBA95_TEAM_PROBE_BUTTON is not a supported SNES button")
local probe_navigation = os.getenv("NBA95_TEAM_NAV") == "1" or
    single_button ~= nil or logo_capture or alignment_capture or panel_anim_capture
local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local writes = assert(io.open(out .. "/wram_writes.txt", "wb"))
local ppu = assert(io.open(out .. "/ppu_states.txt", "wb"))
local global_frame, title_frame, setup_frame = 0, -1, -1
local team_select_entered = false
local PRESS_TITLE_AT, PRESS_SETUP_AT = 850, 400
local LAST_FRAME = single_button and 720 or (probe_navigation and 1190 or 760)
local seen_exec, last_wram, nav_exec, panel_exec = {}, {}, {}, {}
local nav_steps = single_button and {
    { name = "probe_" .. single_button, frame = 650, button = single_button, settle = 690 },
} or {
    { name = "alphabetical_right", frame = 650, button = "right", settle = 690 },
    { name = "alphabetical_left", frame = 720, button = "left", settle = 760 },
    { name = "down_to_scoring", frame = 790, button = "down", settle = 830 },
    { name = "scoring_rank_right", frame = 860, button = "right", settle = 900 },
    { name = "toggle_left_preserve_scoring", frame = 930, button = "l", settle = 970 },
    { name = "up_to_left_name", frame = 1000, button = "up", settle = 1040 },
    { name = "left_alphabetical", frame = 1070, button = "left", settle = 1110 },
    { name = "left_up_to_overall", frame = 1140, button = "up", settle = 1180 },
}
if panel_anim_capture then
    nav_steps = {}
    LAST_FRAME = 710
elseif alignment_capture then
    nav_steps = {
        { name = "activate_left", frame = 650, button = "l", settle = 690 },
        { name = "left_cleveland", frame = 720, button = "right", settle = 750 },
        { name = "left_dallas", frame = 770, button = "right", settle = 800 },
        { name = "left_denver", frame = 820, button = "right", settle = 850 },
        { name = "left_detroit", frame = 870, button = "right", settle = 900 },
        { name = "left_golden_state", frame = 920, button = "right", settle = 960 },
        { name = "activate_right", frame = 1000, button = "l", settle = 1040 },
        { name = "right_philadelphia", frame = 1070, button = "right", settle = 1110 },
    }
    LAST_FRAME = 1130
elseif logo_capture then
    nav_steps = {}
    local logo_step = tonumber(os.getenv("NBA95_TEAM_LOGO_STEP")) or 50
    for row = 1, 5 do
        nav_steps[#nav_steps + 1] = {
            name = "overall_row_" .. row, frame = 620 + (row - 1) * 25,
            button = "down", settle = 0
        }
    end
    for rank = 1, 32 do
        local at = 790 + (rank - 1) * logo_step
        nav_steps[#nav_steps + 1] = {
            name = "overall_next_" .. rank, frame = at,
            button = "right", settle = at + 38
        }
    end
    -- Exhibition normally caps alphabetical navigation at Washington, but the
    -- ROM's native team/name/rank tables continue with East (27) and West
    -- (28). Seed the live selector immediately before an ordinary D-pad redraw
    -- so Mesen captures the ROM-rendered PPU state for those two table entries.
    local special_at = nav_steps[#nav_steps].settle + logo_step
    nav_steps[#nav_steps + 1] = {
        name = "east", frame = special_at, button = "left",
        force_team = 28, settle = special_at + 38
    }
    nav_steps[#nav_steps + 1] = {
        name = "west", frame = special_at + logo_step, button = "right",
        force_team = 27, settle = special_at + logo_step + 38
    }
    LAST_FRAME = nav_steps[#nav_steps].settle + 12
end
local captured_teams = {}

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
    if title_frame >= PRESS_TITLE_AT and setup_frame < 0 then
        setup_frame = 0
        log:write(string.format("entered setup global=%d\n", global_frame)); log:flush()
    end
end, emu.callbackType.exec, 0x80A2BF, 0x80A2BF,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    if title_frame < 0 then title_frame = 0 end
end, emu.callbackType.exec, 0x80E1B1, 0x80E1B1,
    emu.cpuType.snes, emu.memType.snesMemory)

-- $80:DBF6 dispatches here only after the ROM accepts Start on Exhibition.
emu.addMemoryCallback(function()
    if not team_select_entered then
        team_select_entered = true
        log:write(string.format("entered team select global=%d setup=%d\n",
            global_frame, setup_frame)); log:flush()
    end
end, emu.callbackType.exec, 0x82809A, 0x82809A,
    emu.cpuType.snes, emu.memType.snesMemory)

for bank = 0x80, 0x8F do
    emu.addMemoryCallback(function(address)
        if setup_frame >= PRESS_SETUP_AT - 10 then
            seen_exec[address] = true
            if probe_navigation then
                for _, step in ipairs(nav_steps) do
                    if setup_frame >= step.frame - 2 and setup_frame <= step.frame + 8 then
                        nav_exec[step.name] = nav_exec[step.name] or {}
                        nav_exec[step.name][address] = true
                    end
                end
            end
            if panel_anim_capture and setup_frame >= 624 and setup_frame <= 629 then
                panel_exec[setup_frame] = panel_exec[setup_frame] or {}
                panel_exec[setup_frame][address] = true
            end
        end
    end, emu.callbackType.exec, bank * 0x10000 + 0x8000,
        bank * 0x10000 + 0xFFFF, emu.cpuType.snes, emu.memType.snesMemory)
end

emu.addMemoryCallback(function(address, value)
    if setup_frame < PRESS_SETUP_AT - 2 then return end
    if last_wram[address] ~= value then
        writes:write(string.format("%d %04X %02X\n", setup_frame, address, value))
        last_wram[address] = value
    end
end, emu.callbackType.write, 0x0000, 0x1FFF,
    emu.cpuType.snes, emu.memType.snesWorkRam)

emu.addEventCallback(function()
    local input = {}
    if setup_frame < 0 then
        if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
    elseif pulse(setup_frame, PRESS_SETUP_AT) then
        input.start = true
    elseif probe_navigation then
        for _, step in ipairs(nav_steps) do
            if setup_frame == step.frame and step.force_team then
                emu.write(0x16fd, step.force_team, emu.memType.snesWorkRam)
                emu.write(0x16fe, 0, emu.memType.snesWorkRam)
                emu.write(0x1695, step.force_team, emu.memType.snesWorkRam)
                emu.write(0x1696, 0, emu.memType.snesWorkRam)
            end
            if pulse(setup_frame, step.frame) then input[step.button] = true end
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

    if frame >= PRESS_SETUP_AT - 10 then
        local st = emu.getState()
        ppu:write(string.format(
            "%d %s %s %s %s %s %s %s %s %s %s\n", frame,
            tostring(st["ppu.screenBrightness"]),
            tostring(st["ppu.mainScreenLayers"]), tostring(st["ppu.subScreenLayers"]),
            tostring(st["ppu.layers[0].hscroll"]), tostring(st["ppu.layers[0].vscroll"]),
            tostring(st["ppu.layers[1].hscroll"]), tostring(st["ppu.layers[1].vscroll"]),
            tostring(st["ppu.layers[2].hscroll"]), tostring(st["ppu.layers[2].vscroll"]),
            tostring(st["ppu.mode"])))
        if frame % 5 == 0 and (not logo_capture or frame <= 620) then
            shot(string.format("frame_%04d.png", frame))
        end
        if panel_anim_capture and frame >= 620 and frame <= 700 then
            dump_mem(string.format("panel_%04d_cgram.bin", frame),
                emu.memType.snesCgRam, 0x200)
            dump_mem(string.format("panel_%04d_oam.bin", frame),
                emu.memType.snesSpriteRam, 0x220)
            shot(string.format("panel_%04d.png", frame))
        end
    end

    if frame == 650 then
        assert(team_select_entered,
            "Team Select $82:809A was not entered; refusing mislabeled capture")
        dump_mem("team_select_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("team_select_cgram.bin", emu.memType.snesCgRam, 0x200)
        dump_mem("team_select_oam.bin", emu.memType.snesSpriteRam, 0x220)
        dump_mem("team_select_wram.bin", emu.memType.snesWorkRam, 0x20000)
        shot("team_select_settled.png")
    end

    if probe_navigation then
        for _, step in ipairs(nav_steps) do
            if frame == step.settle and not logo_capture then
                shot(step.name .. ".png")
                dump_mem(step.name .. "_wram.bin", emu.memType.snesWorkRam, 0x20000)
            end
        end
    end

    if logo_capture and (frame == 700) then
        local team = emu.read(0x16fd, emu.memType.snesWorkRam, false) or 0xff
        if team <= 28 and not captured_teams[team] then
            captured_teams[team] = true
            dump_mem(string.format("team_%02d_vram.bin", team), emu.memType.snesVideoRam, 0x10000)
            dump_mem(string.format("team_%02d_cgram.bin", team), emu.memType.snesCgRam, 0x200)
            dump_mem(string.format("team_%02d_oam.bin", team), emu.memType.snesSpriteRam, 0x220)
            shot(string.format("team_%02d.png", team))
        end
    elseif logo_capture then
        for _, step in ipairs(nav_steps) do
            if step.settle > 0 and frame == step.settle then
                local team = emu.read(0x16fd, emu.memType.snesWorkRam, false) or 0xff
                if team <= 28 and not captured_teams[team] then
                    captured_teams[team] = true
                    dump_mem(string.format("team_%02d_vram.bin", team), emu.memType.snesVideoRam, 0x10000)
                    dump_mem(string.format("team_%02d_cgram.bin", team), emu.memType.snesCgRam, 0x200)
                    dump_mem(string.format("team_%02d_oam.bin", team), emu.memType.snesSpriteRam, 0x220)
                    shot(string.format("team_%02d.png", team))
                end
            end
        end
    end

    if frame >= LAST_FRAME then
        local addresses = {}
        for address in pairs(seen_exec) do addresses[#addresses + 1] = address end
        table.sort(addresses)
        local exec = assert(io.open(out .. "/exec_trace.txt", "wb"))
        local first, previous = nil, nil
        for _, address in ipairs(addresses) do
            if not first then first, previous = address, address
            elseif address == previous + 1 then previous = address
            else
                exec:write(string.format("%06X-%06X\n", first, previous))
                first, previous = address, address
            end
        end
        if first then exec:write(string.format("%06X-%06X\n", first, previous)) end
        exec:close(); log:write("capture done\n"); log:close(); writes:close(); ppu:close()
        for name, trace in pairs(nav_exec) do
            local nav = assert(io.open(out .. "/exec_" .. name .. ".txt", "wb"))
            local nav_addresses = {}
            for address in pairs(trace) do nav_addresses[#nav_addresses + 1] = address end
            table.sort(nav_addresses)
            for _, address in ipairs(nav_addresses) do
                nav:write(string.format("%06X\n", address))
            end
            nav:close()
        end
        for frame, trace in pairs(panel_exec) do
            local panel = assert(io.open(out .. string.format(
                "/panel_exec_%04d.txt", frame), "wb"))
            local panel_addresses = {}
            for address in pairs(trace) do
                panel_addresses[#panel_addresses + 1] = address
            end
            table.sort(panel_addresses)
            for _, address in ipairs(panel_addresses) do
                panel:write(string.format("%06X\n", address))
            end
            panel:close()
        end
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n"); done:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
