-- Reproducible discovery capture for the first in-game player sprites.
-- Drives boot -> Game Setup -> Team Select, then advances later screens with
-- spaced Start pulses. Screenshots are evidence only; raw PPU memories are the
-- inputs used to identify ROM sprite/tile/palette construction.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")

local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local global_frame, title_frame, setup_frame = 0, -1, -1
local PRESS_TITLE_AT, PRESS_SETUP_AT = 850, 400
local LAST_SETUP_FRAME = tonumber(os.getenv("NBA95_GAMEPLAY_LAST_FRAME")) or 6000
local ANIMATION_TRACE = os.getenv("NBA95_PLAYER_ANIMATION_TRACE") == "1"
local FAST_ANIMATION_TRACE = os.getenv("NBA95_PLAYER_FAST_ANIMATION") == "1"
local PLAYER_INTRO_TRACE = os.getenv("NBA95_PLAYER_INTRO_TRACE") == "1"
local PLAYER_INTRO_AUDIO_TRACE = os.getenv("NBA95_PLAYER_INTRO_AUDIO_TRACE") == "1"
local ANIMATION_INPUT_START = tonumber(os.getenv("NBA95_PLAYER_ANIMATION_INPUT_START")) or 3180
local advance_frames = { 650, 850, 1050, 1250, 1450, 1650, 1850, 2050, 2250 }
if PLAYER_INTRO_TRACE then
    -- Stop advancing after entering Starting Lineup. Later Start pulses skip
    -- the ROM's remaining player cards, which defeats introduction discovery.
    advance_frames[#advance_frames + 1] = 2550
elseif ANIMATION_TRACE then
    advance_frames[#advance_frames + 1] = 2550
    advance_frames[#advance_frames + 1] = 2850
else
    for at = 2550, 5550, 300 do advance_frames[#advance_frames + 1] = at end
end
local TRACE_FIRST = tonumber(os.getenv("NBA95_PLAYER_TRACE_FIRST")) or 4100
local TRACE_LAST = tonumber(os.getenv("NBA95_PLAYER_TRACE_LAST")) or 4210
local SNAPSHOT_FIRST = tonumber(os.getenv("NBA95_PLAYER_SNAPSHOT_FIRST")) or -1
local SNAPSHOT_LAST = tonumber(os.getenv("NBA95_PLAYER_SNAPSHOT_LAST")) or -1
local DEEP_TRACE = os.getenv("NBA95_PLAYER_DEEP_TRACE") == "1"
local trace_exec, trace_vram, trace_cgram, trace_registers, trace_roster_reads =
    {}, {}, {}, {}, {}
local trace_deep = {}
local trace_resources = {}
local trace_animation = {}
local trace_palette_calls = {}
local trace_scene_hits = {}
local trace_intro = {}
local trace_intro_font = {}
local trace_intro_audio = {}
local state_dumped, register_pc_frame = false, -1
local intro_audio_capturing, intro_audio_base_cycle = false, 0
local intro_audio_dsp_addr = 0
local intro_audio_dsp_events = {}

local function flush_intro_audio_dsp_events()
    if not intro_audio_dsp_file or #intro_audio_dsp_events == 0 then return end
    intro_audio_dsp_file:write(table.concat(intro_audio_dsp_events, "\n") .. "\n")
    intro_audio_dsp_events = {}
end

if PLAYER_INTRO_AUDIO_TRACE then
    local function on_apu_write(address, value)
        if setup_frame >= 1400 and setup_frame <= 2800 then
            local state = emu.getState()
            trace_intro_audio[#trace_intro_audio + 1] = string.format(
                "frame=%d APU port=%d value=%02X pc=%02X:%04X\n",
                setup_frame, address & 3, value, state["cpu.k"] or 0,
                state["cpu.pc"] or 0)
        end
    end
    for bank = 0, 0xBF do
        if bank <= 0x3F or bank >= 0x80 then
            local first = bank * 0x10000 + 0x2140
            emu.addMemoryCallback(on_apu_write, emu.callbackType.write,
                first, first + 3, emu.cpuType.snes, emu.memType.snesMemory)
        end
    end
    for _, address in ipairs({ 0x809DF3, 0x80A9E3, 0x80AACD }) do
        emu.addMemoryCallback(function(hit)
            if setup_frame >= 1400 and setup_frame <= 2800 then
                local state = emu.getState()
                trace_intro_audio[#trace_intro_audio + 1] = string.format(
                    "frame=%d EXEC=%06X a=%04X x=%04X y=%04X\n",
                    setup_frame, hit, state["cpu.a"] or 0,
                    state["cpu.x"] or 0, state["cpu.y"] or 0)
            end
        end, emu.callbackType.exec, address, address,
            emu.cpuType.snes, emu.memType.snesMemory)
    end

    emu.addMemoryCallback(function(_, value)
        intro_audio_dsp_addr = value & 0x7F
    end, emu.callbackType.write, 0x00F2, 0x00F2,
        emu.cpuType.spc, emu.memType.spcMemory)

    emu.addMemoryCallback(function(_, value)
        if not intro_audio_capturing then return end
        local state = emu.getState()
        local cycle = state["spc.cycle"]
        if type(cycle) ~= "number" then return end
        local delta = cycle - intro_audio_base_cycle
        if delta < 0 then return end
        intro_audio_dsp_events[#intro_audio_dsp_events + 1] = string.format(
            "%d %02X %02X", delta, intro_audio_dsp_addr, value)
        if #intro_audio_dsp_events >= 2048 then flush_intro_audio_dsp_events() end
    end, emu.callbackType.write, 0x00F3, 0x00F3,
        emu.cpuType.spc, emu.memType.spcMemory)
end

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
    if setup_frame < 0 then
        setup_frame = 0
        log:write(string.format("entered setup global=%d\n", global_frame)); log:flush()
    end
end, emu.callbackType.exec, 0x80A2BF, 0x80A2BF,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    log:write(string.format("entered team select global=%d setup=%d\n",
        global_frame, setup_frame)); log:flush()
end, emu.callbackType.exec, 0x82809A, 0x82809A,
    emu.cpuType.snes, emu.memType.snesMemory)

-- High-level matchup-screen probes used to separate Team Select confirmation,
-- the shared transition interpreter, Player Setup construction, and its live
-- controller-assignment redraw from the much larger gameplay discovery trace.
local scene_probe_addresses = {
    0x828553, 0x81C41E, 0x80E95B, 0x81A489, 0x81B404, 0x81B545,
    0x81B592, 0x81B5FF, 0x81B62C, 0x81B680, 0x81B719, 0x81B7C1,
    0x81BF55, 0x81C0BA,
}
for _, probe in ipairs(scene_probe_addresses) do
    emu.addMemoryCallback(function(address)
        if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
            local state = emu.getState()
            trace_scene_hits[#trace_scene_hits + 1] = string.format(
                "%d %06X a=%04X x=%04X y=%04X\n", setup_frame, address,
                state["cpu.a"] or 0, state["cpu.x"] or 0, state["cpu.y"] or 0)
        end
    end, emu.callbackType.exec, probe, probe,
        emu.cpuType.snes, emu.memType.snesMemory)
end

local function intro_trace_frame(frame)
    return (frame >= 2558 and frame <= 2572) or
           (frame >= 2996 and frame <= 3005) or
           (frame >= 3430 and frame <= 3439) or
           (frame >= 4734 and frame <= 4745)
end

if PLAYER_INTRO_TRACE then
    local intro_ranges = {
        { 0x80B300, 0x80C800 }, { 0x819700, 0x81A520 },
        { 0x83F700, 0x83FFFF }, { 0x84B200, 0x84B400 },
        { 0x87BD00, 0x87C200 },
    }
    for _, range in ipairs(intro_ranges) do
        emu.addMemoryCallback(function(address)
            if intro_trace_frame(setup_frame) then
                local state = emu.getState()
                trace_intro[#trace_intro + 1] = string.format(
                    "%d %06X a=%04X x=%04X y=%04X d=%04X dbr=%02X\n",
                    setup_frame, address, state["cpu.a"] or 0,
                    state["cpu.x"] or 0, state["cpu.y"] or 0,
                    state["cpu.d"] or 0, state["cpu.dbr"] or 0)
            end
        end, emu.callbackType.exec, range[1], range[2],
            emu.cpuType.snes, emu.memType.snesMemory)
    end
    emu.addMemoryCallback(function(address)
        if setup_frame < 2040 or setup_frame > 3000 then return end
        local state = emu.getState()
        local direct = state["cpu.d"] or 0
        local function byte(at)
            return emu.read((direct + at) & 0x1ffff,
                emu.memType.snesWorkRam, false) or 0
        end
        local function word(at) return byte(at) | (byte(at + 1) << 8) end
        trace_intro_font[#trace_intro_font + 1] = string.format(
            "frame=%d pc=%06X font=%02X:%04X text=%02X:%04X x=%04X y=%04X " ..
            "style=%04X\n", setup_frame, address, byte(0x0e), word(0x0c),
            byte(0x1a), word(0x18), state["cpu.x"] or 0,
            state["cpu.y"] or 0, word(0x02))
    end, emu.callbackType.exec, 0x819756, 0x819756,
        emu.cpuType.snes, emu.memType.snesMemory)
end

if not FAST_ANIMATION_TRACE then for bank = 0x80, 0xBF do
    emu.addMemoryCallback(function(address)
        if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
            trace_exec[address] = true
            if DEEP_TRACE and (bank == 0x80 or bank == 0x84 or
                               bank == 0x85 or bank == 0x86 or
                               bank == 0x87) then
                trace_deep[#trace_deep + 1] = string.format(
                    "%d EXEC %06X\n", setup_frame, address)
            end
        end
    end, emu.callbackType.exec, bank * 0x10000 + 0x8000,
        bank * 0x10000 + 0xFFFF, emu.cpuType.snes, emu.memType.snesMemory)
end
end

if DEEP_TRACE then
    emu.addMemoryCallback(function(address, value)
        if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
            local low = address & 0xffff
            if low <= 0x1fff or (low >= 0x3400 and low <= 0x47ff) then
                local state = emu.getState()
                trace_deep[#trace_deep + 1] = string.format(
                    "%d WRAM %06X %02X pc=%04X bank=%02X\n", setup_frame,
                    address, value, state["cpu.pc"] or 0, state["cpu.k"] or 0)
            end
        end
    end, emu.callbackType.write, 0x7e0000, 0x7e47ff,
        emu.cpuType.snes, emu.memType.snesMemory)
end

emu.addMemoryCallback(function(address, value)
    if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
        trace_vram[#trace_vram + 1] = string.format("%d %04X %02X\n",
            setup_frame, address, value)
    end
end, emu.callbackType.write, 0, 0xFFFF,
    emu.cpuType.snes, emu.memType.snesVideoRam)

emu.addMemoryCallback(function(address, value)
    if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
        trace_cgram[#trace_cgram + 1] = string.format("%d %03X %02X\n",
            setup_frame, address, value)
    end
end, emu.callbackType.write, 0, 0x1FF,
    emu.cpuType.snes, emu.memType.snesCgRam)

emu.addMemoryCallback(function(address, value)
    if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
        if address == 0x2121 or address == 0x2122 then
            local state = emu.getState()
            trace_registers[#trace_registers + 1] = string.format(
                "%d %04X %02X pc=%04X bank=%02X dbr=%02X\n", setup_frame,
                address, value, state["cpu.pc"] or 0, state["cpu.k"] or 0,
                state["cpu.dbr"] or 0)
        else
            trace_registers[#trace_registers + 1] = string.format("%d %04X %02X\n",
                setup_frame, address, value)
        end
        if (address == 0x2104 or address == 0x2118) and
           register_pc_frame ~= setup_frame then
            register_pc_frame = setup_frame
            local state = emu.getState()
            trace_registers[#trace_registers + 1] = string.format(
                "%d PC port=%04X cpu.pc=%s cpu.k=%s cpu.dbr=%s\n", setup_frame,
                address,
                tostring(state["cpu.pc"]), tostring(state["cpu.k"]),
                tostring(state["cpu.dbr"]))
        end
    end
end, emu.callbackType.write, 0x2100, 0x437F,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function(address)
    if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
        local state = emu.getState()
        local function word(at)
            return (emu.read(at, emu.memType.snesWorkRam, false) or 0) |
                   ((emu.read(at + 1, emu.memType.snesWorkRam, false) or 0) << 8)
        end
        trace_resources[#trace_resources + 1] = string.format(
            "%d pc=%06X a=%04X x=%04X y=%04X d=%04X table=%02X:%04X " ..
            "part=%02X:%04X player=%04X slot=%04X attr=%04X\n", setup_frame, address,
            state["cpu.a"] or 0, state["cpu.x"] or 0, state["cpu.y"] or 0,
            state["cpu.d"] or 0, word(0x12) & 0xff, word(0x10),
            word(0x0e) & 0xff, word(0x0c), word(0x96), word(0xc2), word(0x14))
    end
end, emu.callbackType.exec, 0x80B348, 0x80B348,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function(address)
    if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
        local state = emu.getState()
        local direct = state["cpu.d"] or 0
        local function byte(at)
            return emu.read((direct + at) & 0x1ffff,
                emu.memType.snesWorkRam, false) or 0
        end
        local source = byte(0x0c) | (byte(0x0d) << 8)
        local bank = byte(0x0e)
        trace_palette_calls[#trace_palette_calls + 1] = string.format(
            "%d source=%02X:%04X colors=%04X cgram=%04X caller=%02X:%04X\n",
            setup_frame, bank, source, state["cpu.x"] or 0,
            state["cpu.y"] or 0, state["cpu.k"] or 0,
            state["cpu.pc"] or 0)
    end
end, emu.callbackType.exec, 0x808A02, 0x808A02,
    emu.cpuType.snes, emu.memType.snesMemory)

if not FAST_ANIMATION_TRACE then for bank = 0xAC, 0xAF do
    emu.addMemoryCallback(function(address, value)
        if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
            local state = emu.getState()
            trace_roster_reads[#trace_roster_reads + 1] = string.format(
                "%d %06X %02X pc=%04X bank=%02X dbr=%02X\n", setup_frame,
                address, value, state["cpu.pc"] or 0, state["cpu.k"] or 0,
                state["cpu.dbr"] or 0)
        end
    end, emu.callbackType.read, bank * 0x10000 + 0x8000,
        bank * 0x10000 + 0xFFFF, emu.cpuType.snes, emu.memType.snesMemory)
end
end

-- Palette and queued graphics streams live outside the roster banks. Restrict
-- the broader ROM watch to the proven generic DMA source read ($80:8BCD) and
-- queued graphics copy ($80:82A3) so traces stay compact while still exposing
-- the exact ROM bytes that become gameplay OBJ tiles.
if not FAST_ANIMATION_TRACE then for bank = 0x80, 0xBF do
    if bank < 0xAC or bank > 0xAF then
        emu.addMemoryCallback(function(address, value)
            if setup_frame >= TRACE_FIRST and setup_frame <= TRACE_LAST then
                local state = emu.getState()
                local pc = state["cpu.pc"] or 0
                if pc == 0x8BCD or pc == 0x82A3 or
                   (pc >= 0xB300 and pc <= 0xB7D0) then
                    trace_roster_reads[#trace_roster_reads + 1] = string.format(
                        "%d %06X %02X pc=%04X bank=%02X dbr=%02X\n", setup_frame,
                        address, value, pc, state["cpu.k"] or 0,
                        state["cpu.dbr"] or 0)
                end
            end
        end, emu.callbackType.read, bank * 0x10000 + 0x8000,
            bank * 0x10000 + 0xFFFF, emu.cpuType.snes, emu.memType.snesMemory)
    end
end
end

emu.addEventCallback(function()
    local input = {}
    if setup_frame < 0 then
        if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
    elseif pulse(setup_frame, PRESS_SETUP_AT) then
        input.start = true
    elseif ANIMATION_TRACE and setup_frame >= ANIMATION_INPUT_START then
        local phase = setup_frame - ANIMATION_INPUT_START
        if phase < 120 then input.right = true
        elseif phase < 240 then input.down = true
        elseif phase < 360 then input.left = true
        elseif phase < 480 then input.up = true
        elseif phase >= 500 and phase < 510 then input.b = true
        elseif phase >= 580 and phase < 590 then input.a = true
        elseif phase >= 660 and phase < 670 then input.x = true
        elseif phase >= 740 and phase < 750 then input.y = true
        end
    else
        for _, at in ipairs(advance_frames) do
            if pulse(setup_frame, at) then input.start = true end
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

    -- The Player Introduction upload through $80:98CD completes immediately
    -- before this frame.  Snapshot the ROM's completed ARAM bank and resident
    -- SPC state before $80:A9E3 sends command $0BFC two frames later, then
    -- retain the exact cycle-timed S-DSP program for asset-pack replay.
    if PLAYER_INTRO_AUDIO_TRACE and frame == 2040 then
        local state = emu.getState()
        intro_audio_base_cycle = assert(state["spc.cycle"], "SPC cycle unavailable")
        dump_mem("player_intro_spc_ram.bin", emu.memType.spcRam, 0x10000)
        dump_mem("player_intro_spc_dsp.bin", emu.memType.spcDspRegisters, 0x80)
        local spc_state = assert(io.open(out .. "/player_intro_spc_state.txt", "wb"))
        spc_state:write(string.format(
            "pc=%s a=%s x=%s y=%s sp=%s ps=%s cycle=%s frames=%d\n",
            tostring(state["spc.pc"]), tostring(state["spc.a"]),
            tostring(state["spc.x"]), tostring(state["spc.y"]),
            tostring(state["spc.sp"]), tostring(state["spc.ps"]),
            tostring(intro_audio_base_cycle), LAST_SETUP_FRAME - frame))
        spc_state:close()
        intro_audio_dsp_file = assert(io.open(
            out .. "/player_intro_dsp_cycle_trace.txt", "wb"))
        intro_audio_dsp_file:write("# cycle_delta register value\n")
        intro_audio_capturing = true
    end

    if PLAYER_INTRO_AUDIO_TRACE and frame >= 1400 and frame <= 2800 and
       frame % 30 == 0 then
        local voices = {}
        for voice = 0, 7 do
            local base = voice * 0x10
            voices[#voices + 1] = string.format("v%d:s%02X/e%02X", voice,
                emu.read(base + 4, emu.memType.spcDspRegisters, false) or 0,
                emu.read(base + 8, emu.memType.spcDspRegisters, false) or 0)
        end
        trace_intro_audio[#trace_intro_audio + 1] = string.format(
            "frame=%d DSP kon=%02X koff=%02X %s\n", frame,
            emu.read(0x4c, emu.memType.spcDspRegisters, false) or 0,
            emu.read(0x5c, emu.memType.spcDspRegisters, false) or 0,
            table.concat(voices, " "))
    end

    if ANIMATION_TRACE and frame >= TRACE_FIRST and frame <= TRACE_LAST then
        local function word(at)
            return (emu.read(at, emu.memType.snesWorkRam, false) or 0) |
                   ((emu.read(at + 1, emu.memType.snesWorkRam, false) or 0) << 8)
        end
        local fields = { "frame=" .. frame,
            string.format("controlled=%04X", word(0x0940)) }
        for index = 0, 9 do
            local base = 0x34eb + index * 0x100
            fields[#fields + 1] = string.format(
                "p%d:id%04X,a%04X,f%04X,us%04X,ls%04X,d%04X,h%04X,pal%04X",
                index, word(base), word(base + 0x2a), word(base + 0x2c),
                word(base + 0x30), word(base + 0x32), word(base + 0x52),
                word(base + 0x2e), word(base + 0xac))
        end
        trace_animation[#trace_animation + 1] = table.concat(fields, " ") .. "\n"
        if frame % 15 == 0 then shot(string.format("anim_%04d.png", frame)) end
    end

    if frame >= TRACE_FIRST and not state_dumped then
        state_dumped = true
        local keys, state = {}, emu.getState()
        for key in pairs(state) do keys[#keys + 1] = key end
        table.sort(keys)
        local state_file = assert(io.open(out .. "/state_keys.txt", "wb"))
        for _, key in ipairs(keys) do
            state_file:write(string.format("%s=%s\n", key, tostring(state[key])))
        end
        state_file:close()
    end

    if frame >= 550 and frame % (frame <= 2800 and 50 or 100) == 0 then
        shot(string.format("frame_%04d.png", frame))
        dump_mem(string.format("frame_%04d_vram.bin", frame),
            emu.memType.snesVideoRam, 0x10000)
        dump_mem(string.format("frame_%04d_cgram.bin", frame),
            emu.memType.snesCgRam, 0x200)
        dump_mem(string.format("frame_%04d_oam.bin", frame),
            emu.memType.snesSpriteRam, 0x220)
        log:write(string.format("snapshot setup=%d\n", frame)); log:flush()
    end
    if PLAYER_INTRO_TRACE and frame >= 2550 and frame <= LAST_SETUP_FRAME and
       frame % 15 == 0 then
        shot(string.format("lineup_%04d.png", frame))
    end
    if frame >= SNAPSHOT_FIRST and frame <= SNAPSHOT_LAST then
        dump_mem(string.format("frame_%04d_vram.bin", frame),
            emu.memType.snesVideoRam, 0x10000)
        dump_mem(string.format("frame_%04d_cgram.bin", frame),
            emu.memType.snesCgRam, 0x200)
        dump_mem(string.format("frame_%04d_oam.bin", frame),
            emu.memType.snesSpriteRam, 0x220)
        dump_mem(string.format("frame_%04d_wram.bin", frame),
            emu.memType.snesWorkRam, 0x20000)
    end

    if frame >= LAST_SETUP_FRAME then
        local addresses = {}
        for address in pairs(trace_exec) do addresses[#addresses + 1] = address end
        table.sort(addresses)
        local exec = assert(io.open(out .. "/player_exec_trace.txt", "wb"))
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
        exec:close()
        local vram = assert(io.open(out .. "/player_vram_writes.txt", "wb"))
        vram:write(table.concat(trace_vram)); vram:close()
        local cgram = assert(io.open(out .. "/player_cgram_writes.txt", "wb"))
        cgram:write(table.concat(trace_cgram)); cgram:close()
        local registers = assert(io.open(out .. "/player_ppu_dma_writes.txt", "wb"))
        registers:write(table.concat(trace_registers)); registers:close()
        local roster = assert(io.open(out .. "/player_roster_reads.txt", "wb"))
        roster:write(table.concat(trace_roster_reads)); roster:close()
        local resources = assert(io.open(out .. "/player_resource_calls.txt", "wb"))
        resources:write(table.concat(trace_resources)); resources:close()
        local palettes = assert(io.open(out .. "/player_palette_calls.txt", "wb"))
        palettes:write(table.concat(trace_palette_calls)); palettes:close()
        local scene_hits = assert(io.open(out .. "/player_setup_scene_hits.txt", "wb"))
        scene_hits:write(table.concat(trace_scene_hits)); scene_hits:close()
        if PLAYER_INTRO_TRACE then
            local intro = assert(io.open(out .. "/player_intro_exec_trace.txt", "wb"))
            intro:write(table.concat(trace_intro)); intro:close()
            local font = assert(io.open(out .. "/player_intro_font_trace.txt", "wb"))
            font:write(table.concat(trace_intro_font)); font:close()
        end
        if PLAYER_INTRO_AUDIO_TRACE then
            flush_intro_audio_dsp_events()
            if intro_audio_dsp_file then intro_audio_dsp_file:close() end
            local audio = assert(io.open(out .. "/player_intro_audio_trace.txt", "wb"))
            audio:write(table.concat(trace_intro_audio)); audio:close()
        end
        if ANIMATION_TRACE then
            local animation = assert(io.open(out .. "/player_animation_states.txt", "wb"))
            animation:write(table.concat(trace_animation)); animation:close()
        end
        if DEEP_TRACE then
            local deep = assert(io.open(out .. "/player_deep_trace.txt", "wb"))
            deep:write(table.concat(trace_deep)); deep:close()
        end
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n"); done:close(); log:write("capture done\n"); log:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
