-- Capture the complete title -> Game Setup handoff from the running ROM.
--
-- The capture origin is the last visible title-fade frame.  From there this
-- records the forced-blank loading interval, the Setup PPU entrance registers,
-- an SPC snapshot, and every mirrored $2140-$2143 write with an SPC-cycle
-- timestamp.  The port packs these hardware/control bytes; no WAV or video is
-- used at runtime.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local ppu = assert(io.open(out .. "/ppu_trace.txt", "wb"))
local entrance_vram_writes = assert(io.open(out .. "/entrance_vram_writes.txt", "wb"))
local entrance_cgram_writes = assert(io.open(out .. "/entrance_cgram_writes.txt", "wb"))
entrance_vram_writes:write("# transition_frame address value\n")
entrance_cgram_writes:write("# transition_frame address value\n")

local title_frame = -1
local transition_frame = -1
local fade_seen = false
local PRESS_AT = 850
local PRESS_LEN = 8
local CAPTURE_FRAMES = 1800 -- 30 seconds of the CPU-driven Setup sequence
local capture_frames = os.getenv("NBA95_CAPTURE_MOTION") == "1"

local capturing = false
local base_cycle = 0
local event_count = 0
local events = {}
local event_file = nil
local entrance_vram = nil
local entrance_cgram = nil

emu.addEventCallback(function()
    if title_frame >= PRESS_AT and title_frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addMemoryCallback(function()
    if title_frame < 0 then title_frame = 0 end
end, emu.callbackType.exec, 0x80E1B1, 0x80E1B1,
    emu.cpuType.snes, emu.memType.snesMemory)

-- $80:E600 calls the 15-step master-brightness fade. The snapshot is armed
-- here, then taken on the last visible (brightness 1) frame.
emu.addMemoryCallback(function()
    fade_seen = true
end, emu.callbackType.exec, 0x80E600, 0x80E600,
    emu.cpuType.snes, emu.memType.snesMemory)

local function dump_mem(name, mem_type, size)
    local bytes = {}
    for i = 0, size - 1 do
        bytes[#bytes + 1] = string.char(emu.read(i, mem_type, false) or 0)
    end
    local f = assert(io.open(out .. "/" .. name, "wb"))
    f:write(table.concat(bytes))
    f:close()
end

local function flush_events()
    if event_file and #events > 0 then
        event_file:write(table.concat(events, "\n") .. "\n")
        events = {}
    end
end

local function on_apu_write(addr, value)
    if not capturing then return end
    local ok, state = pcall(emu.getState)
    local cycle = ok and type(state) == "table" and state["spc.cycle"] or nil
    if type(cycle) ~= "number" then return end
    local delta = cycle - base_cycle
    if delta < 0 then return end
    event_count = event_count + 1
    events[#events + 1] = string.format("%d %d %02X", delta, addr & 3, value)
    if #events >= 2048 then flush_events() end
end

-- APU ports are mirrored in banks $00-$3F and $80-$BF.
for bank = 0, 0xBF do
    if bank <= 0x3F or bank >= 0x80 then
        local first = bank * 0x10000 + 0x2140
        emu.addMemoryCallback(on_apu_write, emu.callbackType.write,
            first, first + 3, emu.cpuType.snes, emu.memType.snesMemory)
    end
end

emu.addEventCallback(function()
    if title_frame >= 0 and transition_frame < 0 then title_frame = title_frame + 1 end
    local ok, state = pcall(emu.getState)

    if transition_frame < 0 and fade_seen and ok and type(state) == "table" and
       state["ppu.screenBrightness"] == 1 then
        transition_frame = 0
        base_cycle = assert(state["spc.cycle"], "SPC cycle unavailable")
        dump_mem("spc_ram.bin", emu.memType.spcRam, 0x10000)
        dump_mem("spc_dsp.bin", emu.memType.spcDspRegisters, 0x80)
        local f = assert(io.open(out .. "/spc_state.txt", "wb"))
        f:write(string.format(
            "pc=%s a=%s x=%s y=%s sp=%s ps=%s cycle=%s frames=%d\n",
            tostring(state["spc.pc"]), tostring(state["spc.a"]),
            tostring(state["spc.x"]), tostring(state["spc.y"]),
            tostring(state["spc.sp"]), tostring(state["spc.ps"]),
            tostring(base_cycle), CAPTURE_FRAMES))
        f:close()
        event_file = assert(io.open(out .. "/apu_cycle_trace.txt", "wb"))
        event_file:write("# cycle_delta port value\n")
        capturing = true
        log:write(string.format("snapshot brightness=1 cycle=%d\n", base_cycle))
        log:flush()
    end

    if transition_frame < 0 then return end
    local frame = transition_frame
    transition_frame = transition_frame + 1

    if capture_frames and frame <= 230 then
        local image = assert(io.open(out .. string.format("/reference_%03d.png", frame), "wb"))
        image:write(emu.takeScreenshot())
        image:close()
    end

    -- $80:A2BF has finished all loading and released forced blank here. Keep
    -- this entrance-time PPU memory separate from the later settled capture:
    -- off-screen tilemap cells are subsequently reused and become visible as
    -- garbage if that later VRAM image is scrolled through the entrance.
    if frame == 106 then
        dump_mem("entrance_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("entrance_cgram.bin", emu.memType.snesCgRam, 0x200)
        entrance_vram = {}
        entrance_cgram = {}
        for address = 0, 0xFFFF do
            entrance_vram[address] = emu.read(address, emu.memType.snesVideoRam, false) or 0
        end
        for address = 0, 0x1FF do
            entrance_cgram[address] = emu.read(address, emu.memType.snesCgRam, false) or 0
        end
    elseif frame > 106 and frame <= 166 and entrance_vram then
        for address = 0, 0xFFFF do
            local value = emu.read(address, emu.memType.snesVideoRam, false) or 0
            if value ~= entrance_vram[address] then
                entrance_vram_writes:write(string.format("%d %04X %02X\n", frame, address, value))
                entrance_vram[address] = value
            end
        end
        for address = 0, 0x1FF do
            local value = emu.read(address, emu.memType.snesCgRam, false) or 0
            if value ~= entrance_cgram[address] then
                entrance_cgram_writes:write(string.format("%d %04X %02X\n", frame, address, value))
                entrance_cgram[address] = value
            end
        end
    end

    if frame <= 230 then
        if ok and type(state) == "table" then
            ppu:write(string.format(
                "%d forced=%s bright=%s main=%s sub=%s bg1=%s,%s bg2=%s,%s bg3=%s,%s\n",
                frame, tostring(state["ppu.forcedBlank"]),
                tostring(state["ppu.screenBrightness"]),
                tostring(state["ppu.mainScreenLayers"]),
                tostring(state["ppu.subScreenLayers"]),
                tostring(state["ppu.layers[0].hscroll"]),
                tostring(state["ppu.layers[0].vscroll"]),
                tostring(state["ppu.layers[1].hscroll"]),
                tostring(state["ppu.layers[1].vscroll"]),
                tostring(state["ppu.layers[2].hscroll"]),
                tostring(state["ppu.layers[2].vscroll"])))
        end
    end

    if frame >= CAPTURE_FRAMES then
        capturing = false
        flush_events()
        if event_file then event_file:close() end
        ppu:close()
        entrance_vram_writes:write("# done\n"); entrance_vram_writes:close()
        entrance_cgram_writes:write("# done\n"); entrance_cgram_writes:close()
        log:write(string.format("done frames=%d events=%d\n", CAPTURE_FRAMES, event_count))
        log:close()
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n"); done:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
