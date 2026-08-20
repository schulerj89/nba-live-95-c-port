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
on_exec(0x82F2FE, "ea_stage_1.png")
on_exec(0x82F37E, "ea_stage_2.png")
on_exec(0x82F43A, "ea_stage_3.png")
on_exec(0x82F492, "ea_stage_4.png")

emu.addEventCallback(function()
    if pending then
        local file = assert(io.open(out .. "/" .. pending, "wb"))
        file:write(emu.takeScreenshot())
        file:close()
        captured[pending] = true
        pending = nil
    end

    if captured["legal.png"] and captured["ea_stage_1.png"] and
       captured["ea_stage_2.png"] and captured["ea_stage_3.png"] and
       captured["ea_stage_4.png"] then
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n")
        done:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
