-- Controlled genuine-entry capture for the CPU free-throw scene.
-- The live Exhibition driver initializes every actor/resource record.  Once
-- its first on-court `$87:A47A` draw preparation, this script requests a native
-- two-attempt CPU stripe scene.  All subsequent state transitions execute
-- through `$87:9CBF`, its `$87:A15C` lane helper, and `$85:9530` unchanged.
local function put(address, value)
    emu.write(address, value & 0xff, emu.memType.snesWorkRam)
    emu.write(address + 1, (value >> 8) & 0xff,
              emu.memType.snesWorkRam)
end

local seeded = false
local draw_calls = 0
local attempts = tonumber(os.getenv('NBA95_FT_ATTEMPTS')) or 2
emu.addMemoryCallback(function()
    draw_calls = draw_calls + 1
    if draw_calls < 1200 then return end
    if seeded then return end
    seeded = true
    -- Actor zero is a native CPU actor after the generic driver clears human
    -- assignments. `$492F` is the stripe shooter selected by 9CBF/A15C.
    put(0x492f, 0)
    put(0x093e, 0)
    put(0x3501, 0xffff) -- actor zero `+$16`: CPU-owned stripe attempt
    put(0x0978, 1)
    put(0x097a, attempts)
    put(0x08de, 0xffff)
    put(0x097c, 0)
    put(0x0972, 0)
end, emu.callbackType.exec, 0x87a47a, 0x87a47a,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv('NBA95_TOOL_DIR')) .. '/mesen_func_vectors.lua')
