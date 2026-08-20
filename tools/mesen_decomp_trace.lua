-- Trace every call to the ROM decompressor ($80:C62B) and every VRAM/CGRAM DMA
-- while the Game Setup screen loads. Direct-page params: $0C/$0E = source
-- addr/bank, $10/$12 = destination addr/bank. Also records the VRAM address
-- ($2116/$2117), CGRAM address ($2121) and VMAIN ($2115) in effect per DMA, so
-- the whole load can be replayed offline from the ROM alone.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local f = assert(io.open(out .. "/decomp_trace.txt", "wb"))
f:write("# loaded\n")
f:flush()

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local LAST_FRAME = 1960

local function rd(a) return emu.read(a, emu.memType.snesMemory, false) or 0 end
local function rd16(a) return rd(a) | (rd(a + 1) << 8) end

local vmaddr = 0
local cgaddr = 0

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addMemoryCallback(function()
    f:write(string.format("frame=%d DECOMP src=%02X:%04X dst=%02X:%04X\n",
        frame, rd(0x0E), rd16(0x0C), rd(0x12), rd16(0x10)))
    f:flush()
end, emu.callbackType.exec, 0x80C62B, 0x80C62B, emu.cpuType.snes, emu.memType.snesMemory)

for _, a in ipairs({ 0x002116, 0x802116 }) do
    emu.addMemoryCallback(function(addr, value)
        vmaddr = (vmaddr & 0xFF00) | value
    end, emu.callbackType.write, a, a, emu.cpuType.snes, emu.memType.snesMemory)
end
for _, a in ipairs({ 0x002117, 0x802117 }) do
    emu.addMemoryCallback(function(addr, value)
        vmaddr = (vmaddr & 0x00FF) | (value << 8)
    end, emu.callbackType.write, a, a, emu.cpuType.snes, emu.memType.snesMemory)
end
for _, a in ipairs({ 0x002121, 0x802121 }) do
    emu.addMemoryCallback(function(addr, value)
        cgaddr = value
    end, emu.callbackType.write, a, a, emu.cpuType.snes, emu.memType.snesMemory)
end

for _, a in ipairs({ 0x00420B, 0x80420B }) do
    emu.addMemoryCallback(function(addr, value)
        if value == 0 then return end
        for ch = 0, 7 do
            if (value & (1 << ch)) ~= 0 then
                local b = 0x4300 + ch * 0x10
                f:write(string.format(
                    "frame=%d DMA ch=%d param=%02X dest=21%02X src=%02X:%04X size=%04X vram=%04X cg=%02X vmain=%02X\n",
                    frame, ch, rd(b), rd(b + 1), rd(b + 4), rd16(b + 2), rd16(b + 5),
                    vmaddr, cgaddr, rd(0x2115)))
            end
        end
        f:flush()
    end, emu.callbackType.write, a, a, emu.cpuType.snes, emu.memType.snesMemory)
end

emu.addEventCallback(function()
    frame = frame + 1
    if frame >= LAST_FRAME then
        f:write("# done\n"); f:close(); emu.stop(0)
    end
end, emu.eventType.endFrame)
