-- Capture the ROM-derived state needed to reconstruct $80:E01E without a
-- recorded WAV or rendered-video asset.  The capture begins when the title's
-- visible fade loop ($80:E1B1) is first executed, after $80:E01E has loaded
-- the gym, title tile data, palettes, and SPC sound bank.
--
-- Output is intentionally raw hardware state: SPC RAM/DSP/registers, PPU
-- VRAM/CGRAM/OAM, WRAM, CPU->APU port writes, and writes to the title music cue
-- mailbox $064A.  tools/extract_assets.py validates and packs these files; the
-- C port, not Mesen, renders the tiles and synthesizes the BRR samples.
local out = "C:/Users/joshs/Projects/nba-live-95-c-port/.analysis/title_capture"

local log = assert(io.open(out .. "/capture_log.txt", "wb"))
local apu = assert(io.open(out .. "/apu_ports.txt", "wb"))
local cues = assert(io.open(out .. "/cues.txt", "wb"))
local ppu = assert(io.open(out .. "/ppu_frames.txt", "wb"))
local vram_writes = assert(io.open(out .. "/vram_writes.txt", "wb"))
local cgram_writes = assert(io.open(out .. "/cgram_writes.txt", "wb"))
local oam_writes = assert(io.open(out .. "/oam_writes.txt", "wb"))
log:write("# waiting for $80:E1B1\n")
apu:write("# title_frame port value\n")
cues:write("# title_frame value\n")
ppu:write("# frame brightness main bg1_h bg1_v bg2_h bg2_v bg3_h bg3_v credit_x credit_y attract_index delay\n")
vram_writes:write("# title_frame address value\n")
cgram_writes:write("# title_frame address value\n")
oam_writes:write("# title_frame address value\n")
log:flush(); apu:flush(); cues:flush(); ppu:flush()

local global_frame = 0
local title_frame = -1
local title_seen = false
local dumped_initial = false
local title_frame_limit = 2160
local previous_vram = {}
local previous_cgram = {}

local function dump_mem(name, mem_type, size)
    local chunks = {}
    for base = 0, size - 1, 4096 do
        local bytes = {}
        local limit = math.min(base + 4095, size - 1)
        for i = base, limit do
            bytes[#bytes + 1] = string.char(emu.read(i, mem_type, false) or 0)
        end
        chunks[#chunks + 1] = table.concat(bytes)
    end
    local f = assert(io.open(out .. "/" .. name, "wb"))
    f:write(table.concat(chunks)); f:close()
end

local function dump_state(prefix)
    dump_mem(prefix .. "_vram.bin", emu.memType.snesVideoRam, 0x10000)
    dump_mem(prefix .. "_cgram.bin", emu.memType.snesCgRam, 0x200)
    dump_mem(prefix .. "_oam.bin", emu.memType.snesSpriteRam, 0x220)
    dump_mem(prefix .. "_wram.bin", emu.memType.snesWorkRam, 0x20000)
    dump_mem(prefix .. "_spc_ram.bin", emu.memType.spcRam, 0x10000)
    dump_mem(prefix .. "_spc_dsp.bin", emu.memType.spcDspRegisters, 0x80)
    local ok, state = pcall(emu.getState)
    if ok and type(state) == "table" then
        local entries = {}
        for key, value in pairs(state) do
            if type(key) == "string" and type(value) ~= "table" and
               (key:sub(1, 4) == "spc." or key:sub(1, 4) == "ppu.") then
                entries[#entries + 1] = key .. "=" .. tostring(value)
            end
        end
        table.sort(entries)
        local f = assert(io.open(out .. "/" .. prefix .. "_state.txt", "wb"))
        f:write(table.concat(entries, "\n") .. "\n"); f:close()
    end
end

-- $80:E1B1 begins the four-frame pre-roll and 0..15 INIDISP fade. Capturing
-- here preserves that fade instead of starting from an already-visible gym.
emu.addMemoryCallback(function()
    if not title_seen then
        title_seen = true
        title_frame = 0
        log:write("title loop entered at global frame " .. global_frame .. "\n")
        log:flush()
    end
end, emu.callbackType.exec, 0x80E1B1, 0x80E1B1,
    emu.cpuType.snes, emu.memType.snesMemory)

-- $064A is consumed and cleared by $80:E381.  These values are the contract
-- between the original music sequencing and the N/B/A/LIVE/95/light stages.
emu.addMemoryCallback(function(_, value)
    if title_frame >= 0 and value ~= 0 then
        cues:write(string.format("%d %d\n", title_frame, value))
        cues:flush()
    end
end, emu.callbackType.write, 0x064A, 0x064A,
    emu.cpuType.snes, emu.memType.snesWorkRam)

local function on_apu_write(addr, value)
    if title_frame >= 0 then
        apu:write(string.format("%d %d %02X\n", title_frame, addr & 3, value))
    end
end
for bank = 0, 0xBF do
    if bank <= 0x3F or bank >= 0x80 then
        local base = bank * 0x10000 + 0x2140
        emu.addMemoryCallback(on_apu_write, emu.callbackType.write, base, base + 3,
            emu.cpuType.snes, emu.memType.snesMemory)
    end
end

emu.addEventCallback(function()
    global_frame = global_frame + 1
    if title_seen then
        if not dumped_initial then
            dump_state("initial")
            local f = assert(io.open(out .. "/initial.png", "wb"))
            f:write(emu.takeScreenshot()); f:close()
            dumped_initial = true
            for i = 0, 0xFFFF do
                previous_vram[i] = emu.read(i, emu.memType.snesVideoRam, false) or 0
            end
            for i = 0, 0x1FF do
                previous_cgram[i] = emu.read(i, emu.memType.snesCgRam, false) or 0
            end
            log:write("initial state dumped at title frame " .. title_frame .. "\n")
            log:flush()
        else
            -- Mesen's DMA writes do not raise direct VRAM/CGRAM memory
            -- callbacks. Compare hardware memory at the frame boundary so the
            -- packed trace records tilemap/palette bytes, never rendered pixels.
            for i = 0, 0xFFFF do
                local value = emu.read(i, emu.memType.snesVideoRam, false) or 0
                if value ~= previous_vram[i] then
                    vram_writes:write(string.format("%d %04X %02X\n", title_frame, i, value))
                    previous_vram[i] = value
                end
            end
            for i = 0, 0x1FF do
                local value = emu.read(i, emu.memType.snesCgRam, false) or 0
                if value ~= previous_cgram[i] then
                    cgram_writes:write(string.format("%d %04X %02X\n", title_frame, i, value))
                    previous_cgram[i] = value
                end
            end
        end

        local ok, state = pcall(emu.getState)
        if ok and type(state) == "table" then
            local function rd16(address)
                local lo = emu.read(address, emu.memType.snesWorkRam, false) or 0
                local hi = emu.read(address + 1, emu.memType.snesWorkRam, false) or 0
                return lo | (hi << 8)
            end
            ppu:write(string.format("%d %s %s %s %s %s %s %s %s %d %d %d %d\n", title_frame,
                tostring(state["ppu.screenBrightness"]),
                tostring(state["ppu.mainScreenLayers"]),
                tostring(state["ppu.layers[0].hscroll"]),
                tostring(state["ppu.layers[0].vscroll"]),
                tostring(state["ppu.layers[1].hscroll"]),
                tostring(state["ppu.layers[1].vscroll"]),
                tostring(state["ppu.layers[2].hscroll"]),
                tostring(state["ppu.layers[2].vscroll"]),
                rd16(0x0615), rd16(0x1872), rd16(0x186C), rd16(0x186E)))
        end

        -- Hardware snapshots at every fourth frame are a reverse-engineering
        -- oracle only; the extractor never packs the PNGs or a video stream.
        if title_frame % 120 == 0 then
            local f = assert(io.open(out .. string.format("/reference_%04d.png", title_frame), "wb"))
            f:write(emu.takeScreenshot()); f:close()
        end
        title_frame = title_frame + 1
    end

    if title_frame >= title_frame_limit then
        if dumped_initial then dump_state("final") end
        apu:write("# done\n"); cues:write("# done\n"); ppu:write("# done\n")
        vram_writes:write("# done\n"); cgram_writes:write("# done\n");
        oam_writes:write("# done\n")
        apu:close(); cues:close(); ppu:close()
        vram_writes:close(); cgram_writes:close(); oam_writes:close()
        log:write("# done title_frames=" .. math.max(title_frame, 0) .. "\n")
        log:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
