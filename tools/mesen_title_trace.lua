-- Differential exec trace of the title screen. Run once with PRESS_ENABLED
-- true and once false; addresses unique to the pressed run are the input
-- handling and snap-to-complete path.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/title_input"
local PRESS_ENABLED = true
local TAG = "press"

local log = assert(io.open(out .. "/exec_" .. TAG .. ".txt", "wb"))
log:write("# loaded\n")
log:flush()

local frame = 0
local PRESS_AT = 1450
local PRESS_LEN = 6
local TRACE_FROM = 1450
local TRACE_TO = 1462
local FADE_FROM = 1572
local FADE_TO = 1580
local LAST_FRAME = 1600

local tracing = false
local fading = false
local seen = {}
local fadeSeen = {}

emu.addEventCallback(function()
    if PRESS_ENABLED and frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({}, 0)
    end
end, emu.eventType.inputPolled)

emu.addMemoryCallback(function(addr)
    if tracing then seen[addr] = true end
    if fading then fadeSeen[addr] = true end
end, emu.callbackType.exec, 0x000000, 0xFFFFFF, emu.cpuType.snes, emu.memType.snesMemory)

local function dump(name, tbl)
    local a = {}
    for k in pairs(tbl) do a[#a + 1] = k end
    table.sort(a)
    log:write("## " .. name .. " (" .. #a .. " addresses)\n")
    local runStart = nil
    local prev = nil
    for _, v in ipairs(a) do
        if runStart == nil then
            runStart = v
        elseif v > prev + 8 then
            log:write(string.format("%06X-%06X\n", runStart, prev))
            runStart = v
        end
        prev = v
    end
    if runStart ~= nil then
        log:write(string.format("%06X-%06X\n", runStart, prev))
    end
    log:flush()
end

emu.addEventCallback(function()
    frame = frame + 1
    tracing = (frame >= TRACE_FROM and frame <= TRACE_TO)
    fading = (frame >= FADE_FROM and frame <= FADE_TO)
    if frame == TRACE_TO + 1 then dump("press_window", seen) end
    if frame == FADE_TO + 1 then dump("fade_window", fadeSeen) end
    if frame >= LAST_FRAME then
        log:write("# done\n")
        log:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
