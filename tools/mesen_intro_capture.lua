-- Capture ROM-rendered source frames for the legal screen and EA intro.
-- Set NBA95_CAPTURE_DIR to the repository's .analysis/intro_capture directory.
--
-- Do not synchronize these captures to emulator startup frames: Mesen can spend
-- a variable number of frames before the game reaches its startup sequence.
-- The addresses below are control-flow landmarks confirmed by the Ghidra dump:
--   $80:FEE6  legal notice hold begins after the fade-in
--   $82:F2FE  E zoom ($82:F56D) has completed
--   $82:F37E  A zoom ($82:F56D) has completed
--   $82:F43A  SPORTS zoom ($82:F56D) has completed
--   $82:F492  completed EA SPORTS logo hold begins
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")

local pending = nil
local captured = {}
local motion_enabled = os.getenv("NBA95_CAPTURE_MOTION") == "1"
local motion_frame = -1
local a_vram_captured = false
local e_vram_captured = false
local mode7_log = nil

local function dump_mem(name, mem_type, size)
    local chunks = {}
    for base = 0, size - 1, 4096 do
        local bytes = {}
        local limit = math.min(base + 4095, size - 1)
        for i = base, limit do
            bytes[#bytes + 1] = string.char(emu.read(i, mem_type, false) or 0)
        end
        chunks[#chunks + 1] = table.concat(bytes)
    end
    local f = assert(io.open(out .. "/" .. name, "wb"))
    f:write(table.concat(chunks)); f:close()
end

local function arm(name)
    if not captured[name] then
        pending = name
    end
end

local function on_exec(address, name)
    emu.addMemoryCallback(function()
        arm(name)
    end, emu.callbackType.exec, address, address,
        emu.cpuType.snes, emu.memType.snesMemory)
end

on_exec(0x80FEE6, "legal.png")
emu.addMemoryCallback(function()
    if motion_enabled and motion_frame < 0 then motion_frame = 0 end
end, emu.callbackType.exec, 0x82F2EA, 0x82F2EA,
    emu.cpuType.snes, emu.memType.snesMemory)
on_exec(0x82F2FE, "ea_stage_1.png")
on_exec(0x82F37E, "ea_stage_2.png")
on_exec(0x82F43A, "ea_stage_3.png")
on_exec(0x82F492, "ea_stage_4.png")

-- $82:F512 returns at $82:F52D after $80:8FA3 has written A's independent
-- Mode 7 tilegroup. Preserve the planar hardware source before the zoom loop;
-- this avoids trying to recover layer ownership from a flattened screenshot.
emu.addMemoryCallback(function()
    if not e_vram_captured then
        dump_mem("ea_e_mode7_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("ea_e_mode7_cgram.bin", emu.memType.snesCgRam, 0x200)
        e_vram_captured = true
    end
end, emu.callbackType.exec, 0x82F512, 0x82F512,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    if not a_vram_captured then
        dump_mem("ea_a_mode7_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("ea_a_mode7_cgram.bin", emu.memType.snesCgRam, 0x200)
        a_vram_captured = true
    end
end, emu.callbackType.exec, 0x82F52D, 0x82F52D,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addEventCallback(function()
    if motion_frame >= 0 and motion_frame < 303 then
        if not mode7_log then
            mode7_log = assert(io.open(out .. "/ea_mode7_state.txt", "wb"))
            mode7_log:write("frame,m7a,m7b,m7c,m7d,m7x,m7y,hscroll,vscroll\n")
        end
        local state = emu.getState()
        mode7_log:write(string.format("%d,%s,%s,%s,%s,%s,%s,%s,%s\n",
            motion_frame,
            tostring(state["ppu.mode7.matrix[0]"]),
            tostring(state["ppu.mode7.matrix[1]"]),
            tostring(state["ppu.mode7.matrix[2]"]),
            tostring(state["ppu.mode7.matrix[3]"]),
            tostring(state["ppu.mode7.centerX"]),
            tostring(state["ppu.mode7.centerY"]),
            tostring(state["ppu.mode7.hscroll"]),
            tostring(state["ppu.mode7.vscroll"])))
        mode7_log:flush()
        if motion_frame >= 95 then
            dump_mem(string.format("ea_motion_%03d_cgram.bin", motion_frame),
                     emu.memType.snesCgRam, 0x200)
        end
        local motion = assert(io.open(out .. string.format("/ea_motion_%03d.png", motion_frame), "wb"))
        motion:write(emu.takeScreenshot())
        motion:close()
        motion_frame = motion_frame + 1
    end
    if pending then
        local file = assert(io.open(out .. "/" .. pending, "wb"))
        file:write(emu.takeScreenshot())
        file:close()
        captured[pending] = true
        pending = nil
    end

    if captured["legal.png"] and captured["ea_stage_1.png"] and
       captured["ea_stage_2.png"] and captured["ea_stage_3.png"] and
       captured["ea_stage_4.png"] and
       (not motion_enabled or motion_frame >= 303) then
        if mode7_log then mode7_log:close(); mode7_log = nil end
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n")
        done:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
