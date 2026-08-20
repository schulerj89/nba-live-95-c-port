-- Capture the complete title -> Game Setup handoff from the running ROM.
--
-- The capture origin is the last visible title-fade frame.  From there this
-- records the forced-blank loading interval, the Setup PPU entrance registers,
-- an SPC snapshot, and every mirrored $2140-$2143 write with an SPC-cycle
-- timestamp.  The port packs these hardware/control bytes; no WAV or video is
-- used at runtime.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup_transition"
local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local ppu = assert(io.open(out .. "/ppu_trace.txt", "wb"))

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local SNAP_AT = 1637
local CAPTURE_FRAMES = 1800 -- 30 seconds of the CPU-driven Setup sequence
local LAST_FRAME = SNAP_AT + CAPTURE_FRAMES

local capturing = false
local base_cycle = 0
local event_count = 0
local events = {}
local event_file = nil

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

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
    frame = frame + 1

    if frame >= 1600 and frame <= 1830 then
        local ok, state = pcall(emu.getState)
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

    if frame == SNAP_AT then
        local ok, state = pcall(emu.getState)
        assert(ok and type(state) == "table", "SPC state unavailable")
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
        log:write(string.format("snapshot frame=%d cycle=%d\n", frame, base_cycle))
        log:flush()
    end

    if frame >= LAST_FRAME then
        capturing = false
        flush_events()
        if event_file then event_file:close() end
        ppu:close()
        log:write(string.format("done frames=%d events=%d\n", CAPTURE_FRAMES, event_count))
        log:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
