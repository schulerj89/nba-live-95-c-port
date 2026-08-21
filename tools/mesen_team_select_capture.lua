-- Reproducible Exhibition Team Select capture.
-- Drives Title -> Game Setup -> Team Select and records the transition,
-- settled PPU memory, controller-owned WRAM, and executed CPU ranges.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")

local confirm = os.getenv("NBA95_TEAM_CONFIRM") or "start"
assert(confirm == "start" or confirm == "a", "NBA95_TEAM_CONFIRM must be start or a")
local probe_navigation = os.getenv("NBA95_TEAM_NAV") == "1"
local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local writes = assert(io.open(out .. "/wram_writes.txt", "wb"))
local ppu = assert(io.open(out .. "/ppu_states.txt", "wb"))
local global_frame, title_frame, setup_frame = 0, -1, -1
local PRESS_TITLE_AT, PRESS_SETUP_AT = 850, 400
local LAST_FRAME = probe_navigation and 1020 or 760
local seen_exec, last_wram, nav_exec = {}, {}, {}
local nav_steps = {
    { name = "left_team_next", frame = 650, button = "down", settle = 690 },
    { name = "activate_right", frame = 720, button = "right", settle = 760 },
    { name = "right_team_next", frame = 790, button = "down", settle = 830 },
    { name = "activate_left", frame = 860, button = "left", settle = 900 },
    { name = "left_team_previous", frame = 930, button = "up", settle = 970 },
}

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
        input[confirm] = true
    elseif probe_navigation then
        for _, step in ipairs(nav_steps) do
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
        if frame % 5 == 0 then shot(string.format("frame_%04d.png", frame)) end
    end

    if frame == 650 then
        dump_mem("team_select_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("team_select_cgram.bin", emu.memType.snesCgRam, 0x200)
        dump_mem("team_select_oam.bin", emu.memType.snesSpriteRam, 0x220)
        dump_mem("team_select_wram.bin", emu.memType.snesWorkRam, 0x20000)
        shot("team_select_settled.png")
    end

    if probe_navigation then
        for _, step in ipairs(nav_steps) do
            if frame == step.settle then
                shot(step.name .. ".png")
                dump_mem(step.name .. "_wram.bin", emu.memType.snesWorkRam, 0x20000)
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
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n"); done:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
