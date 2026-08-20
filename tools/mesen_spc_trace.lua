-- Capture an APU snapshot and, from that exact instant, log the SPC700 PC for
-- the next N instructions. Running the same snapshot through the port's own
-- core and diffing the PC streams locates the first divergent instruction.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local log = assert(io.open(out .. "/spc_trace_log.txt", "wb"))
log:write("# loaded\n")
log:flush()

-- report which CPU/memory type names this build exposes
do
    local names = {}
    for k, v in pairs(emu.cpuType) do names[#names + 1] = tostring(k) .. "=" .. tostring(v) end
    table.sort(names)
    log:write("cpuType: " .. table.concat(names, " ") .. "\n")
    log:flush()
end

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local DUMP_AT = 1900
local TRACE_LEN = 60000

local tracing = false
local traced = 0
local pcfile = nil

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

local function dumpMem(name, memType, size)
    local t = {}
    for i = 0, size - 1 do
        t[#t + 1] = string.char(emu.read(i, memType, false) or 0)
    end
    local g = assert(io.open(out .. "/" .. name, "wb"))
    g:write(table.concat(t))
    g:close()
end

if emu.cpuType.spc ~= nil then
    emu.addMemoryCallback(function(addr)
        if tracing and traced < TRACE_LEN then
            pcfile:write(string.format("%04X\n", addr))
            traced = traced + 1
            if traced == TRACE_LEN then
                pcfile:close()
                log:write("trace complete: " .. traced .. " instructions\n")
                log:flush()
                tracing = false
            end
        end
    end, emu.callbackType.exec, 0x0000, 0xFFFF, emu.cpuType.spc, emu.memType.spcMemory)
else
    log:write("ERROR: emu.cpuType.spc is nil\n")
    log:flush()
end

emu.addEventCallback(function()
    frame = frame + 1
    if frame == DUMP_AT then
        dumpMem("trace_spc_ram.bin", emu.memType.spcRam, 0x10000)
        dumpMem("trace_spc_dsp.bin", emu.memType.spcDspRegisters, 0x80)
        local ok, st = pcall(emu.getState)
        if ok and type(st) == "table" then
            local g = assert(io.open(out .. "/trace_spc_state.txt", "wb"))
            g:write(string.format("pc=%s a=%s x=%s y=%s sp=%s ps=%s\n",
                tostring(st["spc.pc"]), tostring(st["spc.a"]), tostring(st["spc.x"]),
                tostring(st["spc.y"]), tostring(st["spc.sp"]), tostring(st["spc.ps"])))
            g:close()
            log:write(string.format("snapshot pc=%s a=%s x=%s y=%s sp=%s ps=%s\n",
                tostring(st["spc.pc"]), tostring(st["spc.a"]), tostring(st["spc.x"]),
                tostring(st["spc.y"]), tostring(st["spc.sp"]), tostring(st["spc.ps"])))
        end
        pcfile = assert(io.open(out .. "/mesen_pc.txt", "wb"))
        tracing = true
        log:write("tracing started\n")
        log:flush()
    end
    if frame >= DUMP_AT + 240 then
        if pcfile and tracing then pcfile:close() end
        log:write("# done traced=" .. traced .. "\n")
        log:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
