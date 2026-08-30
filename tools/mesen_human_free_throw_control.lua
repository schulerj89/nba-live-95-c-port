-- Genuine-input human free-throw capture.  The live Exhibition driver builds
-- all gameplay records.  At a real on-court draw boundary this script requests
-- a left-side human stripe attempt; B is then driven through Mesen's controller
-- API to exercise the native 3 -> 4 -> 5 -> 9 timing sequence.
local function put(address, value)
    emu.write(address, value & 0xff, emu.memType.snesWorkRam)
    emu.write(address + 1, (value >> 8) & 0xff,
              emu.memType.snesWorkRam)
end

local function word(address)
    return (emu.read(address, emu.memType.snesWorkRam, false) or 0) |
        ((emu.read(address + 1, emu.memType.snesWorkRam, false) or 0) << 8)
end

local seeded, draw_calls = false, 0
emu.addMemoryCallback(function()
    draw_calls = draw_calls + 1
    if seeded or draw_calls < 1200 then return end
    seeded = true
    put(0x492f, 0)      -- native stripe shooter
    put(0x093e, 0)      -- shooter owns the initialized ball
    put(0x3501, 0)      -- actor zero +$16: controller zero
    put(0x4726, 1)      -- left team context +$3B: human aim path
    put(0x0978, 1)
    put(0x097a, 1)
    put(0x08de, 0xffff)
    put(0x097c, 0)
    put(0x0972, 0)
end, emu.callbackType.exec, 0x87a47a, 0x87a47a,
    emu.cpuType.snes, emu.memType.snesMemory)

dofile(assert(os.getenv('NBA95_TOOL_DIR')) .. '/mesen_func_vectors.lua')

local phase, ticks = 'wait-first', 0
local first_delay = tonumber(os.getenv('NBA95_HUMAN_FT_FIRST_DELAY')) or 8
emu.addEventCallback(function()
    if not seeded then return end
    local state = word(0x0978)
    local input = {}
    if state == 3 then
        if phase == 'wait-first' then
            ticks = ticks + 1
            if ticks >= first_delay then phase = 'hold-first'; ticks = 0 end
        end
        if phase == 'hold-first' then input.b = true end
    elseif state == 4 then
        if phase == 'hold-first' then
            ticks = ticks + 1
            input.b = true
            if ticks >= 4 then phase = 'release-first'; ticks = 0 end
        end
    elseif state == 5 then
        if phase == 'release-first' then
            ticks = ticks + 1
            if ticks >= 6 then phase = 'hold-second'; ticks = 0 end
        end
        if phase == 'hold-second' then input.b = true end
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)
