-- Capture every editable value on the main Game Setup page, including exact
-- BG3 pixels, WRAM writes, and the CPU paths reached by each Right press.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")

local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local writes = assert(io.open(out .. "/wram_writes.txt", "wb"))
local exec_log = assert(io.open(out .. "/exec_trace.txt", "wb"))
local global_frame, title_frame, setup_frame = 0, -1, -1
local PRESS_TITLE_AT, LAST_SETUP_FRAME = 850, 1220
local pc_ring, seen_exec = {}, {}

local actions = {
    {400, "right", 0, 1}, {440, "right", 0, 2}, {480, "right", 0, 3},
    {520, "down"},
    {560, "right", 1, 1}, {600, "right", 1, 2}, {640, "right", 1, 3},
    {680, "down"},
    {720, "right", 2, 1}, {760, "right", 2, 2}, {800, "right", 2, 3},
    {840, "right", 2, 4}, {880, "down"},
    {920, "right", 3, 1}, {960, "right", 3, 2}, {1000, "right", 3, 3},
    {1040, "right", 3, 4}, {1080, "right", 3, 5}, {1120, "right", 3, 6},
}

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

local function pulse(frame, at) return frame >= at and frame < at + 3 end

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

emu.addMemoryCallback(function(address)
    if setup_frame < 350 then return end
    pc_ring[#pc_ring + 1] = address
    if #pc_ring > 20 then table.remove(pc_ring, 1) end
    if address >= 0x809D00 and address <= 0x80ABFF then seen_exec[address] = true end
end, emu.callbackType.exec, 0x808000, 0x80BFFF,
    emu.cpuType.snes, emu.memType.snesMemory)

emu.addMemoryCallback(function(address, value)
    if setup_frame < 350 then return end
    for _, action in ipairs(actions) do
        if setup_frame >= action[1] and setup_frame <= action[1] + 5 then
            local pcs = {}
            for _, pc in ipairs(pc_ring) do pcs[#pcs + 1] = string.format("%06X", pc) end
            writes:write(string.format("frame=%d addr=7E%04X value=%02X pcs=%s\n",
                setup_frame, address, value, table.concat(pcs, ",")))
            break
        end
    end
end, emu.callbackType.write, 0x0000, 0x1FFF,
    emu.cpuType.snes, emu.memType.snesWorkRam)

emu.addEventCallback(function()
    local input = {}
    if setup_frame < 0 then
        if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
    else
        for _, action in ipairs(actions) do
            if pulse(setup_frame, action[1]) then input[action[2]] = true end
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
    if frame == 380 then
        dump_mem("main_default_vram.bin", emu.memType.snesVideoRam, 0x10000)
        dump_mem("main_default_wram.bin", emu.memType.snesWorkRam, 0x20000)
        shot("main_default.png")
    end
    for _, action in ipairs(actions) do
        if action[2] == "right" and frame == action[1] + 12 then
            local stem = string.format("row%d_step%d", action[3], action[4])
            dump_mem(stem .. "_vram.bin", emu.memType.snesVideoRam, 0x10000)
            dump_mem(stem .. "_wram.bin", emu.memType.snesWorkRam, 0x20000)
            shot(stem .. ".png")
        end
    end
    if frame >= LAST_SETUP_FRAME then
        local addresses = {}
        for address in pairs(seen_exec) do addresses[#addresses + 1] = address end
        table.sort(addresses)
        for _, address in ipairs(addresses) do exec_log:write(string.format("%06X\n", address)) end
        log:write("capture done\n"); log:close(); writes:close(); exec_log:close()
        local done = assert(io.open(out .. "/capture_complete.txt", "wb"))
        done:write("ok\n"); done:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
