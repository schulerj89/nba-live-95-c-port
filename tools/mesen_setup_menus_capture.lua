-- Capture Game Setup -> Set Rules/Set Options navigation with the evidence
-- needed to port it: screenshots, WRAM snapshots, executed ROM ranges, and
-- CPU-to-APU writes. Set NBA95_CAPTURE_MENU to "rules" or "options".
local out = os.getenv("NBA95_CAPTURE_DIR")
local menu = os.getenv("NBA95_CAPTURE_MENU") or "rules"
local scroll_mode = os.getenv("NBA95_CAPTURE_SCROLL") == "1"
local variant_mode = os.getenv("NBA95_CAPTURE_VARIANTS") == "1"
local value_mode = os.getenv("NBA95_CAPTURE_VALUES") == "1"
local call_mode = os.getenv("NBA95_CAPTURE_CALLS") == "1"
local every_frame_mode = os.getenv("NBA95_CAPTURE_EVERY_FRAME") == "1"
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
assert(menu == "rules" or menu == "options", "NBA95_CAPTURE_MENU must be rules or options")

local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local exec_log = assert(io.open(out .. "/exec_trace.txt", "wb"))
local apu_log = assert(io.open(out .. "/apu_ports.txt", "wb"))
local wram_log = assert(io.open(out .. "/wram_writes.txt", "wb"))
local dsp_log = assert(io.open(out .. "/dsp_writes.txt", "wb"))
local ppu_log = assert(io.open(out .. "/menu_transition_ppu.txt", "wb"))
local call_log = assert(io.open(out .. "/menu_transition_calls.txt", "wb"))
local open_vram_writes = assert(io.open(out .. "/open_transition_vram_writes.txt", "wb"))
local open_cgram_writes = assert(io.open(out .. "/open_transition_cgram_writes.txt", "wb"))
local return_vram_writes = assert(io.open(out .. "/return_transition_vram_writes.txt", "wb"))
local return_cgram_writes = assert(io.open(out .. "/return_transition_cgram_writes.txt", "wb"))
local open_ppu_states = assert(io.open(out .. "/open_transition_ppu_states.txt", "wb"))
local return_ppu_states = assert(io.open(out .. "/return_transition_ppu_states.txt", "wb"))
local global_frame = 0
local title_frame = -1
local setup_frame = -1
local PRESS_TITLE_AT = 850
local LAST_SETUP_FRAME = value_mode and 960 or 1050
local target_row = menu == "rules" and 4 or 5
local open_vram, open_cgram, return_vram, return_cgram

local function dump_mem(name, mem_type, size)
    local chunks = {}
    for i = 0, size - 1 do
        chunks[#chunks + 1] = string.char(emu.read(i, mem_type, false) or 0)
    end
    local f = assert(io.open(out .. "/" .. name, "wb"))
    f:write(table.concat(chunks)); f:close()
end

local function dump_wram(name) dump_mem(name, emu.memType.snesWorkRam, 0x20000) end

local function snapshot_ppu(prefix)
    dump_mem(prefix .. "_vram.bin", emu.memType.snesVideoRam, 0x10000)
    dump_mem(prefix .. "_cgram.bin", emu.memType.snesCgRam, 0x200)
    local vram, cgram = {}, {}
    for address = 0, 0xFFFF do
        vram[address] = emu.read(address, emu.memType.snesVideoRam, false) or 0
    end
    for address = 0, 0x1FF do
        cgram[address] = emu.read(address, emu.memType.snesCgRam, false) or 0
    end
    return vram, cgram
end

local function trace_ppu(frame, vram, cgram, vram_file, cgram_file)
    for address = 0, 0xFFFF do
        local value = emu.read(address, emu.memType.snesVideoRam, false) or 0
        if value ~= vram[address] then
            vram_file:write(string.format("%d %04X %02X\n", frame, address, value))
            vram[address] = value
        end
    end
    for address = 0, 0x1FF do
        local value = emu.read(address, emu.memType.snesCgRam, false) or 0
        if value ~= cgram[address] then
            cgram_file:write(string.format("%d %04X %02X\n", frame, address, value))
            cgram[address] = value
        end
    end
end

local function trace_ppu_state(frame, file)
    local st = emu.getState()
    local values = { frame,
        st["ppu.screenBrightness"], st["ppu.mainScreenLayers"],
        st["ppu.subScreenLayers"] }
    for layer = 0, 2 do
        values[#values + 1] = st[string.format("ppu.layers[%d].hscroll", layer)]
        values[#values + 1] = st[string.format("ppu.layers[%d].vscroll", layer)]
        values[#values + 1] = st[string.format("ppu.layers[%d].tilemapAddress", layer)]
        values[#values + 1] = st[string.format("ppu.layers[%d].chrAddress", layer)]
        values[#values + 1] = st[string.format("ppu.layers[%d].doubleWidth", layer)] and 1 or 0
        values[#values + 1] = st[string.format("ppu.layers[%d].doubleHeight", layer)] and 1 or 0
    end
    file:write(table.concat(values, " ") .. "\n")
end

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
local seen_calls = { open = {}, back = {} }
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
    if call_mode and (phase == "open" or phase == "back") then
        local opcode = emu.read(address, emu.memType.snesMemory, false)
        local lo = emu.read(address + 1, emu.memType.snesMemory, false) or 0
        local hi = emu.read(address + 2, emu.memType.snesMemory, false) or 0
        local target = nil
        if opcode == 0x20 then
            target = (address & 0xFF0000) | lo | (hi << 8) -- JSR abs
        elseif opcode == 0x22 then
            local bank = emu.read(address + 3, emu.memType.snesMemory, false) or 0
            target = lo | (hi << 8) | (bank << 16) -- JSL long
        end
        if target then
            local key = string.format("%06X>%06X", address, target)
            if not seen_calls[phase][key] then
                seen_calls[phase][key] = true
                call_log:write(string.format("first_frame=%d phase=%s caller=%06X target=%06X\n",
                    setup_frame, phase, address, target))
            end
        end
    end
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
        if value_mode then
            -- Capture every Set Options discrete value independently from the
            -- settled default canvas.  Each changed row is restored before the
            -- next row is captured, making each VRAM delta safe to compose.
            if pulse(setup_frame, 650) or pulse(setup_frame, 662) then input.down = true end
            if pulse(setup_frame, 674) or pulse(setup_frame, 700) or
               pulse(setup_frame, 724) then input.right = true end
            if pulse(setup_frame, 742) then input.down = true end
            if pulse(setup_frame, 754) or pulse(setup_frame, 780) then input.right = true end
            if pulse(setup_frame, 798) then input.down = true end
            if pulse(setup_frame, 810) then input.right = true end
            if pulse(setup_frame, 836) then input.left = true end
            if pulse(setup_frame, 854) then input.down = true end
            if pulse(setup_frame, 866) then input.right = true end
            if pulse(setup_frame, 892) then input.left = true end
            if pulse(setup_frame, 910) then input.down = true end
            if pulse(setup_frame, 922) then input.right = true end
        elseif variant_mode then
            if pulse(setup_frame, 650) or pulse(setup_frame, 662) then input.down = true end
            if pulse(setup_frame, 674) then input.right = true end
            if menu == "options" then
                if pulse(setup_frame, 700) then input.right = true end
                if pulse(setup_frame, 730) or pulse(setup_frame, 742) or
                   pulse(setup_frame, 754) then input.down = true end
                if pulse(setup_frame, 770) then input.right = true end
                if pulse(setup_frame, 800) then input.down = true end
                if pulse(setup_frame, 812) then input.right = true end
            end
            if pulse(setup_frame, 830) then input.start = true end
        elseif scroll_mode then
            for i = 0, 11 do
                if pulse(setup_frame, 630 + i * 12) then input.down = true end
            end
            if pulse(setup_frame, 830) then input.start = true end
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
    if frame == 470 then
        open_vram, open_cgram = snapshot_ppu("open_transition")
    elseif frame >= 471 and frame <= 649 and open_vram then
        trace_ppu(frame, open_vram, open_cgram,
                  open_vram_writes, open_cgram_writes)
        trace_ppu_state(frame, open_ppu_states)
    end
    if not value_mode and frame == 830 then
        return_vram, return_cgram = snapshot_ppu("return_transition")
    elseif not value_mode and frame >= 831 and frame <= 1000 and return_vram then
        trace_ppu(frame, return_vram, return_cgram,
                  return_vram_writes, return_cgram_writes)
        trace_ppu_state(frame, return_ppu_states)
    end
    if frame == 649 then
        dump_wram("wram_before_change.bin"); shot("before_change.png")
        -- The transition does not finish constructing BG3 until after the
        -- frame-560 black hold.  Capture the settled menu image used by the
        -- port, immediately before the first value adjustment.
        dump_mem("menu_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("menu_cgram.bin", emu.memType.snesCgRam, 0x200)
        dump_mem("menu_oam.bin", emu.memType.snesSpriteRam, 0x220)
    end
    if frame == 675 then dump_wram("wram_after_right.bin"); shot("after_right.png") end
    if frame == 725 then dump_wram("wram_after_down.bin"); shot("after_down.png") end
    if frame == 775 then dump_wram("wram_after_left.bin"); shot("after_left.png") end
    if variant_mode and frame == 690 then
        dump_mem(menu .. "_off_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot(menu .. "_off.png")
    end
    if variant_mode and menu == "options" and frame == 715 then
        dump_mem("options_mono_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_mono.png")
    end
    if variant_mode and menu == "options" and frame == 790 then
        dump_mem("options_cpu_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_cpu.png")
    end
    if value_mode and menu == "options" and frame == 690 then
        dump_mem("options_mode_off_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_mode_off.png")
    end
    if value_mode and menu == "options" and frame == 715 then
        dump_mem("options_mode_mono_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_mode_mono.png")
    end
    if value_mode and menu == "options" and frame == 770 then
        dump_mem("options_crowd_off_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_crowd_off.png")
    end
    if value_mode and menu == "options" and frame == 826 then
        dump_mem("options_slow_on_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_slow_on.png")
    end
    if value_mode and menu == "options" and frame == 882 then
        dump_mem("options_shot_cpu_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_shot_cpu.png")
    end
    if value_mode and menu == "options" and frame == 938 then
        dump_mem("options_assistance_on_vram.bin", emu.memType.snesVideoRam, 0x10000)
        shot("options_assistance_on.png")
    end
    if scroll_mode and frame == 690 then shot("rules_scroll_mid.png") end
    if scroll_mode and frame == 780 then shot("rules_scroll_bottom.png") end
    if frame == 860 then dump_wram("wram_after_back.bin"); shot("after_back.png") end

    if not value_mode and ((frame >= 465 and frame <= 649) or
                           (frame >= 825 and frame <= 1000)) then
        local st = emu.getState()
        ppu_log:write(string.format(
            "%d bright=%s main=%s sub=%s bg1h=%s bg1v=%s bg2h=%s bg2v=%s bg3h=%s bg3v=%s bg3map=%s bg3chr=%s bg3wide=%s bg3tall=%s oamBase=%s oamMode=%s\n",
            frame, tostring(st["ppu.screenBrightness"]),
            tostring(st["ppu.mainScreenLayers"]), tostring(st["ppu.subScreenLayers"]),
            tostring(st["ppu.layers[0].hscroll"]), tostring(st["ppu.layers[0].vscroll"]),
            tostring(st["ppu.layers[1].hscroll"]), tostring(st["ppu.layers[1].vscroll"]),
            tostring(st["ppu.layers[2].hscroll"]), tostring(st["ppu.layers[2].vscroll"]),
            tostring(st["ppu.layers[2].tilemapAddress"]),
            tostring(st["ppu.layers[2].chrAddress"]),
            tostring(st["ppu.layers[2].doubleWidth"]),
            tostring(st["ppu.layers[2].doubleHeight"]),
            tostring(st["ppu.oamBaseAddress"]), tostring(st["ppu.oamMode"])))
        if every_frame_mode or frame % 4 == 1 then
            shot(string.format(frame < 700 and "open_step_%03d.png" or "close_step_%03d.png", frame))
        end
    end

    if frame >= LAST_SETUP_FRAME then
        write_exec_ranges()
        log:write("capture done\n"); log:close(); exec_log:close(); apu_log:close();
        wram_log:close(); dsp_log:close(); ppu_log:close(); call_log:close()
        open_vram_writes:close(); open_cgram_writes:close()
        return_vram_writes:close(); return_cgram_writes:close()
        open_ppu_states:close(); return_ppu_states:close()
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n"); done:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
