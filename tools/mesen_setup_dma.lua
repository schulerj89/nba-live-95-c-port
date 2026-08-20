-- Log every DMA into VRAM/CGRAM while the Game Setup screen loads, plus the set
-- of routines executing once it is live. This yields the authoritative source
-- addresses and routine list to follow in Ghidra.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local dmalog = assert(io.open(out .. "/dma_log.txt", "wb"))
local log = assert(io.open(out .. "/dma_run_log.txt", "wb"))

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local LOG_FROM = 1490
local TRACE_FROM = 1880
local TRACE_TO = 1884
local LAST_FRAME = 1895

local logging = false
local tracing = false
local seen = {}
local nseen = 0

local function rd(a) return emu.read(a, emu.memType.snesMemory, false) or 0 end

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({ }, 0)
    end
end, emu.eventType.inputPolled)

-- $420B: MDMAEN. Snapshot each enabled channel's parameters at kick-off.
local function onDmaEnable(addr, value)
    if not logging or value == 0 then return end
    local vmaddr = rd(0x2116) | (rd(0x2117) << 8)
    for ch = 0, 7 do
        if (value & (1 << ch)) ~= 0 then
            local b = 0x4300 + ch * 0x10
            local param = rd(b + 0)
            local dest  = rd(b + 1)
            local src   = rd(b + 2) | (rd(b + 3) << 8)
            local srcbk = rd(b + 4)
            local size  = rd(b + 5) | (rd(b + 6) << 8)
            dmalog:write(string.format(
                "frame=%d ch=%d param=%02X dest=21%02X src=%02X:%04X size=%04X vmaddr=%04X\n",
                frame, ch, param, dest, srcbk, src, size, vmaddr))
        end
    end
    dmalog:flush()
end

for _, base in ipairs({ 0x00420B, 0x80420B }) do
    emu.addMemoryCallback(onDmaEnable, emu.callbackType.write, base, base,
        emu.cpuType.snes, emu.memType.snesMemory)
end

emu.addMemoryCallback(function(addr)
    if tracing and not seen[addr] then seen[addr] = true; nseen = nseen + 1 end
end, emu.callbackType.exec, 0x000000, 0xFFFFFF, emu.cpuType.snes, emu.memType.snesMemory)

emu.addEventCallback(function()
    frame = frame + 1
    logging = frame >= LOG_FROM
    tracing = (frame >= TRACE_FROM and frame <= TRACE_TO)

    if frame == TRACE_TO + 1 then
        local a = {}
        for k in pairs(seen) do a[#a + 1] = k end
        table.sort(a)
        local f = assert(io.open(out .. "/setup_exec_addrs.txt", "wb"))
        f:write(string.format("# unique executed addresses on Game Setup screen: %d\n", nseen))
        local runStart, prev = nil, nil
        for _, v in ipairs(a) do
            if runStart == nil then runStart = v
            elseif v > prev + 8 then
                f:write(string.format("%06X-%06X\n", runStart, prev)); runStart = v
            end
            prev = v
        end
        if runStart then f:write(string.format("%06X-%06X\n", runStart, prev)) end
        f:close()
        log:write("wrote setup_exec_addrs.txt n=" .. nseen .. "\n"); log:flush()
    end

    if frame >= LAST_FRAME then
        dmalog:close(); log:write("done\n"); log:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
