-- Observe native gameplay audio without changing the verified tip-off driver.
-- Records CPU->APU command traffic, per-frame DSP voice state, and the WRAM
-- gameplay fields needed to correlate sounds with dribbles, rim contact,
-- whistles, possession changes, and scoring. Captured data is evidence only;
-- it is never packed as runtime audio.
local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")
local driver = os.getenv("NBA95_TIPOFF_DRIVER")
assert(driver and driver ~= "", "NBA95_TIPOFF_DRIVER is not set")

local ports = assert(io.open(out .. "/gameplay_apu_ports.txt", "wb"))
local voices = assert(io.open(out .. "/gameplay_dsp_voices.txt", "wb"))
local commands = assert(io.open(out .. "/gameplay_sound_commands.txt", "wb"))
ports:write("# global_frame gameplay_frame port value pc\n")
voices:write("# global_frame gameplay_frame owner ball_activity rim whistle score voices\n")
commands:write("# global_frame gameplay_frame command x y owner ball_activity rim whistle score\n")

local global_frame = 0
local gameplay_frame = -1
local previous_voice = {}
local gameplay_spc_dumped = false
local variant_probe = os.getenv("NBA95_AUDIO_VARIANT_PROBE") == "1"
local variant_log = variant_probe and
    assert(io.open(out .. "/gameplay_audio_variant_injections.txt", "wb")) or nil

local function word(address)
    local lo = emu.read(address, emu.memType.snesWorkRam, false) or 0
    local hi = emu.read(address + 1, emu.memType.snesWorkRam, false) or 0
    return lo | (hi << 8)
end

local function signed_word(address)
    local value = word(address)
    return value >= 0x8000 and value - 0x10000 or value
end

local function write_word(address, value)
    emu.write(address, value & 0xff, emu.memType.snesWorkRam)
    emu.write(address + 1, (value >> 8) & 0xff, emu.memType.snesWorkRam)
end

-- Controlled evidence mode for every `$13E7` randomized command family.
-- `$80:8930` is the `$07F6` LFSR. These seeds force each masked result once;
-- the runtime never reads this file and normal passive captures are unchanged.
if variant_probe then
    local probes = {}
    local seeds3 = {0x0002,0x8001,0x0001,0x8000}
    local seeds15 = {0x0008,0x8003,0x0001,0x8002,0x0002,0x8001,0x0003,0x8000,
                     0x0004,0x8007,0x0005,0x8006,0x0006,0x8005,0x0007,0x8004}
    local seeds7 = {0x0004,0x8003,0x0001,0x8002,0x0002,0x8001,0x0003,0x8000}
    local families = {
        {name="bounce", bit=0x0001, seeds=seeds3},
        {name="inner_rim", bit=0x0002, seeds=seeds15},
        {name="made_basket", bit=0x0004, seeds=seeds3},
        {name="outer_rim", bit=0x0008, seeds=seeds3},
        {name="catch", bit=0x0010, seeds=seeds3},
        {name="contact", bit=0x0020, seeds=seeds3},
        {name="shoe", bit=0x0040, seeds=seeds3},
        {name="collision_a", bit=0x0080, seeds=seeds7},
        {name="landing", bit=0x0100, seeds=seeds3},
        {name="collision_b", bit=0x0200, seeds=seeds7},
    }
    for _, family in ipairs(families) do
        for index = 0, #family.seeds - 1 do
            probes[#probes + 1] = {
                name=family.name, bit=family.bit, index=index,
                seed=family.seeds[index + 1]
            }
        end
    end
    local last_probe = -1
    emu.addMemoryCallback(function()
        if gameplay_frame < 20 then return end
        local index = math.floor((gameplay_frame - 20) / 24)
        if index < 0 or index >= #probes or index == last_probe then return end
        last_probe = index
        local probe = probes[index + 1]
        write_word(0x13e7, probe.bit)
        write_word(0x07f6, probe.seed)
        variant_log:write(string.format("%d family=%s index=%d seed=%04X\n",
            gameplay_frame, probe.name, probe.index, probe.seed))
    end, emu.callbackType.exec, 0x82FD65, 0x82FD65,
        emu.cpuType.snes, emu.memType.snesMemory)
end

local function dump_memory(name, memory_type, size)
    local chunks = {}
    for address = 0, size - 1 do
        chunks[#chunks + 1] = string.char(
            emu.read(address, memory_type, false) or 0)
    end
    local file = assert(io.open(out .. "/" .. name, "wb"))
    file:write(table.concat(chunks))
    file:close()
end

-- The first live on-court draw is the same stable gameplay-frame origin used
-- by mesen_tipoff_capture.lua and the ROM/port differential harness.
emu.addMemoryCallback(function()
    if gameplay_frame < 0 then gameplay_frame = 0 end
end, emu.callbackType.exec, 0x87A47A, 0x87A47A,
    emu.cpuType.snes, emu.memType.snesMemory)

-- `$80:9DF3` is the shared native indexed-sound dispatcher. A on entry is
-- the command ID; recording the surrounding raw state lets an offline pass
-- identify the gameplay producer without inferring from audio alone.
emu.addMemoryCallback(function()
    local state = emu.getState()
    commands:write(string.format("%d %d %02X %04X %04X %d %04X/%04X %04X/%04X %04X %04X/%04X\n",
        global_frame, gameplay_frame, (state["cpu.a"] or 0) & 0xff,
        state["cpu.x"] or 0, state["cpu.y"] or 0, signed_word(0x0946),
        word(0x0948), word(0x094a), word(0x097c), word(0x13e7),
        word(0x09b6), word(0x4711), word(0x4791)))
end, emu.callbackType.exec, 0x809DF3, 0x809DF3,
    emu.cpuType.snes, emu.memType.snesMemory)

local function on_apu_write(address, value)
    if gameplay_frame < 0 then return end
    local state = emu.getState()
    ports:write(string.format("%d %d %d %02X %02X:%04X\n",
        global_frame, gameplay_frame, address & 3, value or 0,
        state["cpu.k"] or 0, state["cpu.pc"] or 0))
end

-- $2140-$2143 are mirrored in banks $00-$3F and $80-$BF. Observe each
-- physical CPU-visible alias because the game's DBR changes by caller.
for bank = 0, 0xbf do
    if bank <= 0x3f or bank >= 0x80 then
        local base = bank * 0x10000
        emu.addMemoryCallback(on_apu_write, emu.callbackType.write,
            base + 0x2140, base + 0x2143,
            emu.cpuType.snes, emu.memType.snesMemory)
    end
end

emu.addEventCallback(function()
    global_frame = global_frame + 1
    if gameplay_frame < 0 then return end

    if not gameplay_spc_dumped then
        gameplay_spc_dumped = true
        dump_memory("gameplay_spc_ram.bin", emu.memType.spcRam, 0x10000)
        dump_memory("gameplay_spc_dsp.bin", emu.memType.spcDspRegisters, 0x80)
    end

    local changed = false
    local parts = {}
    for voice = 0, 7 do
        local base = voice * 0x10
        local left = emu.read(base, emu.memType.spcDspRegisters, false) or 0
        local right = emu.read(base + 1, emu.memType.spcDspRegisters, false) or 0
        local pitch = (emu.read(base + 2, emu.memType.spcDspRegisters, false) or 0) |
                      ((emu.read(base + 3, emu.memType.spcDspRegisters, false) or 0) << 8)
        local srcn = emu.read(base + 4, emu.memType.spcDspRegisters, false) or 0
        local envx = emu.read(base + 8, emu.memType.spcDspRegisters, false) or 0
        local state = string.format("%02X/%04X/%02X/%02X/%02X", srcn, pitch,
                                    envx, left, right)
        if previous_voice[voice] ~= state then changed = true end
        previous_voice[voice] = state
        parts[#parts + 1] = string.format("v%d=%s", voice, state)
    end

    if changed then
        voices:write(string.format("%d %d %d %04X/%04X %04X/%04X %04X %04X/%04X %s\n",
            global_frame, gameplay_frame, signed_word(0x0946),
            word(0x0948), word(0x094a), word(0x097c), word(0x13e7),
            word(0x09b6), word(0x4711), word(0x4791), table.concat(parts, " ")))
    end
    gameplay_frame = gameplay_frame + 1
end, emu.eventType.endFrame)

-- The driver owns navigation, CPU-vs-CPU setup, capture termination, and the
-- canonical gameplay-frame origin. Its emu.stop closes the process; flush on
-- every line so a completed testrunner capture never loses the final events.
ports:setvbuf("no")
voices:setvbuf("no")
commands:setvbuf("no")
if variant_log then variant_log:setvbuf("no") end
dofile(driver)
