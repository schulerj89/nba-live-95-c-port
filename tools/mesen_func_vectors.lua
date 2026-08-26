-- Generic per-function I/O vector capture.
-- Breakpoints one 65816 routine at its entry and return instruction(s) and
-- records, for every real in-game call, the CPU registers plus configured
-- WRAM ranges at entry and at exit. The resulting JSONL vectors are ground
-- truth for verifying the corresponding C port function: replay each entry
-- state through the C function and diff its outputs against the exit state.
--
-- Requires Debug.ScriptWindow.AllowIoOsAccess = true in Mesen2 settings.
--
-- Environment:
--   NBA95_CAPTURE_DIR   output directory (required)
--   NBA95_VEC_ENTRY     24-bit entry PC, hex, e.g. 86D035 (required)
--   NBA95_VEC_EXITS     comma-separated 24-bit PCs of the routine's RTS/RTL
--                       (or other return points), hex (required)
--   NBA95_VEC_READS     WRAM ranges captured at entry, e.g. 0900-097F,1615
--   NBA95_VEC_WRITES    WRAM ranges captured at exit, same syntax
--   NBA95_VEC_LABEL     output name stem (default: func_<entry>)
--   NBA95_VEC_MAX       stop recording after this many calls (default 200)
--   NBA95_VEC_DRIVE     1 = drive the verified Exhibition path into live
--                       gameplay first and record only once on-court play
--                       begins (same route as mesen_tipoff_capture.lua)
--   NBA95_CPU_VS_CPU    1 = with NBA95_VEC_DRIVE, clear human assignments so
--                       both teams play under CPU control
--   NBA95_VEC_SHARED_EXITS 1 = exit PCs are internal/shared boundaries;
--                       callbacks without a pending entry are counted
--                       separately instead of as orphaned returns
--
-- Output:
--   <label>.vectors.jsonl  one vector per completed call
--   <label>.meta.json      the configuration that produced the vectors
--   capture_complete.txt   completion sentinel for the launching script

local out = os.getenv("NBA95_CAPTURE_DIR")
assert(out and out ~= "", "NBA95_CAPTURE_DIR is not set")

local function parse_pc(text, name)
    local value = tonumber(text or "", 16)
    assert(value, name .. " must be a hex 24-bit address")
    return value
end

local entry_pc = parse_pc(os.getenv("NBA95_VEC_ENTRY"), "NBA95_VEC_ENTRY")

local exit_pcs = {}
for item in (os.getenv("NBA95_VEC_EXITS") or ""):gmatch("[^,]+") do
    exit_pcs[#exit_pcs + 1] = parse_pc(item, "NBA95_VEC_EXITS entry")
end
assert(#exit_pcs > 0, "NBA95_VEC_EXITS is not set")

local function parse_ranges(text)
    local ranges = {}
    for item in (text or ""):gmatch("[^,]+") do
        local first, last = item:match("^%s*(%x+)%s*%-%s*(%x+)%s*$")
        if not first then first, last = item:match("^%s*(%x+)%s*$"), nil end
        assert(first, "bad WRAM range: " .. item)
        local lo = tonumber(first, 16)
        local hi = last and tonumber(last, 16) or lo
        assert(lo <= hi, "reversed WRAM range: " .. item)
        ranges[#ranges + 1] = { first = lo, last = hi }
    end
    return ranges
end

local read_ranges = parse_ranges(os.getenv("NBA95_VEC_READS"))
local write_ranges = parse_ranges(os.getenv("NBA95_VEC_WRITES"))
local label = os.getenv("NBA95_VEC_LABEL")
if not label or label == "" then
    label = string.format("func_%06x", entry_pc)
end
local max_calls = tonumber(os.getenv("NBA95_VEC_MAX")) or 200

-- Registers come from the flat dotted-string state table; keep every cpu.*
-- key rather than guessing individual names.
local function cpu_snapshot()
    local snapshot = {}
    for key, value in pairs(emu.getState()) do
        if key:sub(1, 4) == "cpu." then snapshot[key:sub(5)] = value end
    end
    return snapshot
end

local function mem_snapshot(ranges)
    local snapshot = {}
    for _, range in ipairs(ranges) do
        local chunks = {}
        for address = range.first, range.last do
            chunks[#chunks + 1] = string.format(
                "%02x", emu.read(address, emu.memType.snesWorkRam, false) or 0)
        end
        snapshot[string.format("%04x", range.first)] = table.concat(chunks)
    end
    return snapshot
end

local function json_scalar(value)
    local kind = type(value)
    if kind == "number" then
        if value % 1 == 0 then return string.format("%d", value) end
        return string.format("%.17g", value)
    elseif kind == "boolean" then
        return value and "true" or "false"
    end
    return string.format("%q", tostring(value))
end

local function json_object(map)
    local keys = {}
    for key in pairs(map) do keys[#keys + 1] = key end
    table.sort(keys)
    local parts = {}
    for _, key in ipairs(keys) do
        parts[#parts + 1] = string.format("%q:%s", key, json_scalar(map[key]))
    end
    return "{" .. table.concat(parts, ",") .. "}"
end

local vectors = assert(io.open(out .. "/" .. label .. ".vectors.jsonl", "wb"))
local meta = assert(io.open(out .. "/" .. label .. ".meta.json", "wb"))
meta:write(string.format(
    '{"entry":"%06x","exits":%s,"reads":%s,"writes":%s,"max_calls":%d}\n',
    entry_pc,
    (function()
        local parts = {}
        for _, pc in ipairs(exit_pcs) do
            parts[#parts + 1] = string.format('"%06x"', pc)
        end
        return "[" .. table.concat(parts, ",") .. "]"
    end)(),
    (function(ranges)
        local parts = {}
        for _, range in ipairs(ranges) do
            parts[#parts + 1] = string.format('"%04x-%04x"', range.first, range.last)
        end
        return "[" .. table.concat(parts, ",") .. "]"
    end)(read_ranges),
    (function(ranges)
        local parts = {}
        for _, range in ipairs(ranges) do
            parts[#parts + 1] = string.format('"%04x-%04x"', range.first, range.last)
        end
        return "[" .. table.concat(parts, ",") .. "]"
    end)(write_ranges),
    max_calls))
meta:close()

local frame, recorded, orphan_exits, shared_exit_callbacks, done =
    0, 0, 0, 0, false
local pending = {}
local shared_exits = os.getenv("NBA95_VEC_SHARED_EXITS") == "1"
local drive = os.getenv("NBA95_VEC_DRIVE") == "1"
local force_cpu_vs_cpu = os.getenv("NBA95_CPU_VS_CPU") == "1"
-- Without driving, record immediately; with driving, wait for on-court play.
local recording = not drive

local function finish()
    done = true
    vectors:close()
    local sentinel = assert(io.open(out .. "/capture_complete.txt", "wb"))
    sentinel:write(string.format(
        "label=%s vectors=%d orphan_exits=%d shared_exit_callbacks=%d\n",
        label, recorded, orphan_exits, shared_exit_callbacks))
    sentinel:close()
    emu.stop(0)
end

emu.addEventCallback(function() frame = frame + 1 end, emu.eventType.endFrame)

emu.addMemoryCallback(function()
    if done or not recording then return end
    -- LIFO so recursive or interrupt-nested calls pair with the right exit.
    pending[#pending + 1] = {
        frame = frame,
        cpu = cpu_snapshot(),
        mem = mem_snapshot(read_ranges),
    }
end, emu.callbackType.exec, entry_pc, entry_pc,
    emu.cpuType.snes, emu.memType.snesMemory)

for _, exit_pc in ipairs(exit_pcs) do
    emu.addMemoryCallback(function()
        if done or not recording then return end
        local entry = table.remove(pending)
        if not entry then
            if shared_exits then
                -- An internal boundary can be revisited later by the same
                -- invocation after its first classified exit was recorded.
                shared_exit_callbacks = shared_exit_callbacks + 1
            else
                -- Script attached mid-call; nothing to pair this exit with.
                orphan_exits = orphan_exits + 1
            end
            return
        end
        recorded = recorded + 1
        local mem_parts = {}
        for base, hex in pairs(entry.mem) do
            mem_parts[#mem_parts + 1] = string.format("%q:%q", base, hex)
        end
        table.sort(mem_parts)
        local exit_mem = mem_snapshot(write_ranges)
        local exit_parts = {}
        for base, hex in pairs(exit_mem) do
            exit_parts[#exit_parts + 1] = string.format("%q:%q", base, hex)
        end
        table.sort(exit_parts)
        vectors:write(string.format(
            '{"call":%d,"entry_frame":%d,"exit_frame":%d,"exit_pc":"%06x",' ..
            '"entry":{"cpu":%s,"mem":{%s}},"exit":{"cpu":%s,"mem":{%s}}}\n',
            recorded, entry.frame, frame, exit_pc,
            json_object(entry.cpu), table.concat(mem_parts, ","),
            json_object(cpu_snapshot()), table.concat(exit_parts, ",")))
        vectors:flush()
        if recorded >= max_calls then finish() end
    end, emu.callbackType.exec, exit_pc, exit_pc,
        emu.cpuType.snes, emu.memType.snesMemory)
end

if drive then
    -- Verified Exhibition route copied from mesen_tipoff_capture.lua: Start
    -- on the title, Start on Game Setup (forced to Exhibition), then spaced
    -- Start pulses through the presentation cards until the first on-court
    -- player draw at $87:A47A begins recording.
    local title_frame, setup_frame, gameplay_frame = -1, -1, -1
    local player_load_seen = false
    local PRESS_TITLE_AT, PRESS_SETUP_AT = 850, 400
    local MAX_GAMEPLAY_FRAMES = tonumber(os.getenv("NBA95_VEC_FRAMES")) or 1800

    local function write_word(address, value)
        emu.write(address, value & 0xff, emu.memType.snesWorkRam)
        emu.write(address + 1, (value >> 8) & 0xff, emu.memType.snesWorkRam)
    end

    local function clear_human_assignments()
        for address = 0x08d4, 0x08dc, 2 do write_word(address, 0xffff) end
        write_word(0x093a, 0xffff)
        write_word(0x093e, 0xffff)
        write_word(0x0940, 0x0000)
        write_word(0x095e, 0xffff)
        write_word(0x1615, 0xffff)
    end

    local function pulse(value, at)
        return value >= at and value < at + 3
    end

    emu.addMemoryCallback(function()
        if title_frame < 0 then title_frame = 0 end
    end, emu.callbackType.exec, 0x80E1B1, 0x80E1B1,
        emu.cpuType.snes, emu.memType.snesMemory)

    emu.addMemoryCallback(function()
        if title_frame >= PRESS_TITLE_AT and setup_frame < 0 then
            setup_frame = 0
        end
    end, emu.callbackType.exec, 0x80A2BF, 0x80A2BF,
        emu.cpuType.snes, emu.memType.snesMemory)

    emu.addMemoryCallback(function()
        player_load_seen = true
    end, emu.callbackType.exec, 0x86D7B8, 0x86D7B8,
        emu.cpuType.snes, emu.memType.snesMemory)

    emu.addMemoryCallback(function()
        if gameplay_frame < 0 then
            gameplay_frame = 0
            recording = true
        end
    end, emu.callbackType.exec, 0x87A47A, 0x87A47A,
        emu.cpuType.snes, emu.memType.snesMemory)

    emu.addEventCallback(function()
        local input = {}
        if setup_frame < 0 then
            if pulse(title_frame, PRESS_TITLE_AT) then input.start = true end
        elseif pulse(setup_frame, PRESS_SETUP_AT) then
            input.start = true
        elseif gameplay_frame < 0 and setup_frame >= 650 and
               ((setup_frame - 650) % 200) < 3 then
            input.start = true
        end
        emu.setInput(input, 0)
    end, emu.eventType.inputPolled)

    emu.addEventCallback(function()
        if done then return end
        if title_frame >= 0 and setup_frame < 0 then
            title_frame = title_frame + 1
        end
        if setup_frame >= 0 then
            -- Game Setup persists Mode; force the verified Exhibition working
            -- word before Start so an old save cannot route through Season.
            if setup_frame >= 300 and setup_frame < PRESS_SETUP_AT then
                emu.write(0x16fb, 0, emu.memType.snesWorkRam)
                emu.write(0x16fc, 0, emu.memType.snesWorkRam)
            end
            setup_frame = setup_frame + 1
        end
        if gameplay_frame < 0 then
            if force_cpu_vs_cpu and player_load_seen then
                clear_human_assignments()
            end
            assert(setup_frame < 8000,
                "Timed out before gameplay player initialization")
            return
        end
        gameplay_frame = gameplay_frame + 1
        -- Emit whatever was captured even if max_calls was never reached.
        if gameplay_frame >= MAX_GAMEPLAY_FRAMES then finish() end
    end, emu.eventType.endFrame)
end

emu.displayMessage("func_vectors", string.format(
    "recording %s at $%06x (max %d calls)", label, entry_pc, max_calls))
