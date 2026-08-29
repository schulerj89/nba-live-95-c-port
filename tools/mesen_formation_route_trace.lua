-- Adds parent/child branch evidence to the generic `$85:AD6B` vector run.
-- The generic driver owns menu input, full snapshots, and termination.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local trace = assert(io.open(out .. "/formation_route.children.jsonl", "wb"))
local active = false
local stack = {}

local function word(address)
    local lo = emu.read(address, emu.memType.snesWorkRam, false) or 0
    local hi = emu.read(address + 1, emu.memType.snesWorkRam, false) or 0
    return lo | (hi << 8)
end

emu.addMemoryCallback(function() active = true end, emu.callbackType.exec,
    0x87A47A, 0x87A47A, emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    if not active then return end
    local actor = word(0x0096)
    stack[#stack + 1] = {
        frame = emu.getState()["ppu.frameCount"] or 0,
        slot = word(0x00C2), actor = actor, child = "none",
        target_x = word(actor + 0x56), target_y = word(actor + 0x58),
    }
end, emu.callbackType.exec, 0x85AD6B, 0x85AD6B,
    emu.cpuType.snes, emu.memType.snesMemory)

local function child(name)
    if #stack == 0 then return end
    local call = stack[#stack]
    call.child = name
    call.aa = word(0x00AA)
    call.ae = word(0x00AE)
end

emu.addMemoryCallback(function() child("b402") end, emu.callbackType.exec,
    0x85B402, 0x85B402, emu.cpuType.snes, emu.memType.snesMemory)
emu.addMemoryCallback(function() child("b3aa") end, emu.callbackType.exec,
    0x85B3AA, 0x85B3AA, emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function()
    local call = table.remove(stack)
    if not call then return end
    local actor = call.actor
    trace:write(string.format(
        '{"frame":%d,"slot":%d,"child":"%s","target_in":[%d,%d],"child_dp":[%d,%d],"target_out":[%d,%d],"velocity_out":[%d,%d]}\n',
        call.frame, call.slot, call.child, call.target_x, call.target_y,
        call.aa or 0, call.ae or 0, word(actor + 0x56), word(actor + 0x58),
        word(actor + 0x0E), word(actor + 0x10)))
    trace:flush()
end, emu.callbackType.exec, 0x85AF5B, 0x85AF5B,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv("NBA95_VECTOR_DRIVER")))
