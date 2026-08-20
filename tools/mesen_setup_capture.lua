-- Press Start once on the title screen, settle on Game Setup, then capture the
-- screen plus full PPU/APU state and a per-frame BG scroll log.
-- This Mesen build: emu.setInput(inputTable, port); memTypes are snesVideoRam /
-- snesCgRam / snesWorkRam / spcRam / snesSpriteRam.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/setup_capture"
local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local scrolllog = assert(io.open(out .. "/scroll_log.txt", "wb"))

local frame = 0
local PRESS_AT = 1500
local PRESS_LEN = 8
local SHOT_FROM = 1560
local SHOT_TO = 2100
local STATE_AT = 1900
local LAST_FRAME = 2120

local function dumpMem(name, memType, size)
    if memType == nil then
        log:write("SKIP " .. name .. " (nil memType)\n"); log:flush(); return
    end
    local t = {}
    for i = 0, size - 1 do
        t[#t + 1] = string.char(emu.read(i, memType, false) or 0)
    end
    local g = assert(io.open(out .. "/" .. name, "wb"))
    g:write(table.concat(t))
    g:close()
    log:write(string.format("dumped %s (%d bytes, memType=%s)\n", name, size, tostring(memType)))
    log:flush()
end

local function dumpTable(f, t, prefix, depth)
    if depth > 3 or type(t) ~= "table" then return end
    for k, v in pairs(t) do
        if type(v) == "table" then
            dumpTable(f, v, prefix .. tostring(k) .. ".", depth + 1)
        else
            f:write(prefix .. tostring(k) .. "=" .. tostring(v) .. "\n")
        end
    end
end

emu.addEventCallback(function()
    if frame >= PRESS_AT and frame < PRESS_AT + PRESS_LEN then
        emu.setInput({ start = true }, 0)
    else
        emu.setInput({ }, 0)
    end
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    frame = frame + 1

    if frame >= SHOT_FROM - 60 and frame <= SHOT_TO then
        local ok, st = pcall(emu.getState)
        if ok and type(st) == "table" and st.ppu and st.ppu.layers then
            local L = st.ppu.layers
            scrolllog:write(string.format("%d bg1=%s,%s bg2=%s,%s bg3=%s,%s bright=%s\n",
                frame,
                tostring(L[0] and L[0].hscroll), tostring(L[0] and L[0].vscroll),
                tostring(L[1] and L[1].hscroll), tostring(L[1] and L[1].vscroll),
                tostring(L[2] and L[2].hscroll), tostring(L[2] and L[2].vscroll),
                tostring(st.ppu.screenBrightness)))
        end
    end

    if frame >= SHOT_FROM and frame <= SHOT_TO and frame % 4 == 0 then
        local g = assert(io.open(string.format("%s/cap_%04d.png", out, frame), "wb"))
        g:write(emu.takeScreenshot())
        g:close()
    end

    if frame == STATE_AT then
        local okst, st = pcall(emu.getState)
        if not okst or type(st) ~= "table" then st = {} end
        local f = assert(io.open(out .. "/ppu_state.txt", "wb"))
        dumpTable(f, st, "", 0)
        f:close()
        log:write("wrote ppu_state.txt\n"); log:flush()

        dumpMem("vram.bin", emu.memType.snesVideoRam, 0x10000)
        dumpMem("cgram.bin", emu.memType.snesCgRam, 0x200)
        dumpMem("oam.bin", emu.memType.snesSpriteRam, 0x220)
        dumpMem("wram.bin", emu.memType.snesWorkRam, 0x10000)
        dumpMem("spcram.bin", emu.memType.spcRam, 0x10000)

        local g = assert(io.open(out .. "/state_frame.png", "wb"))
        g:write(emu.takeScreenshot()); g:close()
    end

    if frame >= LAST_FRAME then
        log:write("capture done\n")
        log:close(); scrolllog:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
