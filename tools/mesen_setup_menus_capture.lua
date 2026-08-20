-- Capture Game Setup -> Set Rules/Set Options navigation with the evidence
-- needed to port it: screenshots, WRAM snapshots, executed ROM ranges, and
-- CPU-to-APU writes. Set NBA95_CAPTURE_MENU to "rules" or "options".
local out = os.getenv("NBA95_CAPTURE_DIR")
local menu = os.getenv("NBA95_CAPTURE_MENU") or "rules"
local scroll_mode = os.getenv("NBA95_CAPTURE_SCROLL") == "1"
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
assert(menu == "rules" or menu == "options", "NBA95_CAPTURE_MENU must be rules or options")

local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local exec_log = assert(io.open(out .. "/exec_trace.txt", "wb"))
local apu_log = assert(io.open(out .. "/apu_ports.txt", "wb"))
local wram_log = assert(io.open(out .. "/wram_writes.txt", "wb"))
local dsp_log = assert(io.open(out .. "/dsp_writes.txt", "wb"))
local global_frame = 0
local title_frame = -1
local setup_frame = -1
local PRESS_TITLE_AT = 850
local LAST_SETUP_FRAME = 900
local target_row = menu == "rules" and 4 or 5

local function dump_mem(name, mem_type, size)
    local chunks = {}
    for i = 0, size - 1 do
        chunks[#chunks + 1] = string.char(emu.read(i, mem_type, false) or 0)
    end
    local f = assert(io.open(out .. "/" .. name, "wb"))
    f:write(table.concat(chunks)); f:close()
end

local function dump_wram(name) dump_mem(name, emu.memType.snesWorkRam, 0x20000) end

local function shot(name)
    local f = assert(io.open(out .. "/" .. name, "wb"))
    f:write(emu.takeScreenshot()); f:close()
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

local seen_exec = {}
local pc_ring = {}
local function phase_for_frame(frame)
    if frame < 470 then return "parent" end
    if frame < 650 then return "open" end
    if frame < 700 then return "right" end
    if frame < 750 then return "down" end
    if frame < 830 then return "left" end
    return "back"
end

emu.addMemoryCallback(function(address)
    if setup_frame < 0 then return end
    pc_ring[#pc_ring + 1] = address
    if #pc_ring > 16 then table.remove(pc_ring, 1) end
    local phase = phase_for_frame(setup_frame)
    seen_exec[phase] = seen_exec[phase] or {}
    seen_exec[phase][address] = true
end, emu.callbackType.exec, 0x808000, 0xBFFFFF,
    emu.cpuType.snes, emu.memType.snesMemory)

local function write_exec_ranges()
    for _, phase in ipairs({"parent", "open", "right", "down", "left", "back"}) do
        exec_log:write("# " .. phase .. "\n")
        local addresses = {}
        for address in pairs(seen_exec[phase] or {}) do addresses[#addresses + 1] = address end
        table.sort(addresses)
        local first, last = nil, nil
        for _, address in ipairs(addresses) do
            if not first then
                first, last = address, address
            elseif address == last + 1 then
                last = address
            else
                exec_log:write(string.format("%06X-%06X\n", first, last))
                first, last = address, address
            end
        end
        if first then exec_log:write(string.format("%06X-%06X\n", first, last)) end
    end
end

emu.addMemoryCallback(function(address, value)
    if setup_frame >= 648 and
       ((setup_frame <= 657) or
        (setup_frame >= 698 and setup_frame <= 707) or
        (setup_frame >= 748 and setup_frame <= 757) or
        (setup_frame >= 828 and setup_frame <= 837)) then
        local pcs = {}
        for _, pc in ipairs(pc_ring) do pcs[#pcs + 1] = string.format("%06X", pc) end
        wram_log:write(string.format("frame=%d phase=%s addr=7E%04X value=%02X pcs=%s\n",
            setup_frame, phase_for_frame(setup_frame), address, value, table.concat(pcs, ",")))
    end
end, emu.callbackType.write, 0x1500, 0x1800,
    emu.cpuType.snes, emu.memType.snesWorkRam)

for bank = 0, 0xBF do
    if bank <= 0x3F or bank >= 0x80 then
        local first = bank * 0x10000 + 0x2140
        emu.addMemoryCallback(function(addr, value)
        if setup_frame >= 0 then
            apu_log:write(string.format("frame=%d addr=%06X value=%02X\n",
                setup_frame, addr, value)); apu_log:flush()
        end
        end, emu.callbackType.write, first, first + 3,
            emu.cpuType.snes, emu.memType.snesMemory)
    end
end


local dsp_addr = 0
emu.addMemoryCallback(function(_, value)
    dsp_addr = value & 0x7F
end, emu.callbackType.write, 0x00F2, 0x00F2,
    emu.cpuType.spc, emu.memType.spcMemory)
emu.addMemoryCallback(function(_, value)
    if setup_frame >= 450 then
        dsp_log:write(string.format("frame=%d reg=%02X value=%02X\n",
            setup_frame, dsp_addr, value))
    end
end, emu.callbackType.write, 0x00F3, 0x00F3,
    emu.cpuType.spc, emu.memType.spcMemory)

local function pulse(frame, at)
    return frame >= at and frame < at + 3
end

emu.addEventCallback(function()
    local input = {}
    if setup_frame < 0 then
        if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
    else
        for i = 0, target_row - 1 do
            if pulse(setup_frame, 380 + i * 12) then input.down = true end
        end
        if pulse(setup_frame, 470) then input.a = true end
        -- Exercise both value navigation and row navigation inside the menu.
        if scroll_mode then
            for i = 0, 11 do
                if pulse(setup_frame, 630 + i * 12) then input.down = true end
            end
        else
            if pulse(setup_frame, 650) then input.right = true end
            if pulse(setup_frame, 700) then input.down = true end
            if pulse(setup_frame, 750) then input.left = true end
            if pulse(setup_frame, 830) then input.start = true end
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

    if frame == 370 then dump_wram("wram_parent.bin"); shot("parent.png") end
    if frame == 469 then dump_wram("wram_before_open.bin"); shot("before_open.png") end
    if frame == 480 then shot("open_010.png") end
    if frame == 500 then shot("open_030.png") end
    if frame == 520 then shot("open_050.png") end
    if frame == 540 then shot("open_070.png") end
    if frame == 560 then
        dump_wram("wram_open.bin"); shot("menu_open.png")
    end
    if frame == 649 then
        dump_wram("wram_before_change.bin"); shot("before_change.png")
        -- The transition does not finish constructing BG3 until after the
        -- frame-560 black hold.  Capture the settled menu image used by the
        -- port, immediately before the first value adjustment.
        dump_mem("menu_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("menu_cgram.bin", emu.memType.snesCgRam, 0x200)
    end
    if frame == 675 then dump_wram("wram_after_right.bin"); shot("after_right.png") end
    if frame == 725 then dump_wram("wram_after_down.bin"); shot("after_down.png") end
    if frame == 775 then dump_wram("wram_after_left.bin"); shot("after_left.png") end
    if scroll_mode and frame == 690 then shot("rules_scroll_mid.png") end
    if scroll_mode and frame == 780 then shot("rules_scroll_bottom.png") end
    if frame == 860 then dump_wram("wram_after_back.bin"); shot("after_back.png") end

    if frame >= LAST_SETUP_FRAME then
        write_exec_ranges()
        log:write("capture done\n"); log:close(); exec_log:close(); apu_log:close();
        wram_log:close(); dsp_log:close()
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n"); done:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
