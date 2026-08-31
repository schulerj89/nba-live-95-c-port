-- Controlled hardware raster experiment. The original ROM/CPU execute normally.
-- At startFrame inject only INIDISP, TM/TS, CGWSEL/CGADSUB, SETINI and CGRAM[0].
-- No expected RGB conversion is implemented here. All samples come from the PPU.
-- Requires --snes.disableFrameSkipping=true; getScreenBuffer is synchronous.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local output=assert(io.open(out..'/brightness.jsonl','wb'))
local rejected=assert(io.open(out..'/rejected-attempts.jsonl','wb'))
local writes=assert(io.open(out..'/native-hardware-writes.jsonl','wb'))
local frame,index,active=0,0,nil
local injecting=false
local function observe(kind,address,value,desired)
    if not active or injecting then return end
    local state=emu.getState()
    local conflict=value~=desired
    if conflict then active.conflicts=active.conflicts+1 end
    writes:write(string.format(
        '{"case":%d,"frame":%d,"kind":"%s","address":%d,"value":%d,"injected_value":%d,"conflict":%s,"scanline":%d,"observed_pc":%d}\n',
        active.index,frame,kind,address,value,desired,conflict and 'true' or 'false',
        state['ppu.scanline'] or -1,state['cpu.pc'] or 0))
    writes:flush()
end
for bank=0,0xbf do
    if bank<=0x3f or bank>=0x80 then
        emu.addMemoryCallback(function(address,value)
            local reg=address&0xffff
            if active and (reg==0x2100 or reg==0x212c or reg==0x212d or
                           reg==0x2130 or reg==0x2131 or reg==0x2133) then
                observe('register',address,value,reg==0x2100 and active.brightness or 0)
            elseif active and not injecting and (reg==0x2121 or reg==0x2122) then
                -- CGDATA writes are staged and may target the PPU's current
                -- CGRAM fetch during active scanout. Record the bus writes and
                -- actual backdrop before every command, avoiding an inferred
                -- address/latch model. Overwrite-then-restore is thus visible.
                local actual=emu.read(0,emu.memType.snesCgRam)|
                             (emu.read(1,emu.memType.snesCgRam)<<8)
                local conflict=actual~=active.color
                if conflict then active.conflicts=active.conflicts+1 end
                local state=emu.getState()
                writes:write(string.format(
                    '{"case":%d,"frame":%d,"kind":"cgram_command","address":%d,"value":%d,"observed_cgram_word":%d,"injected_value":%d,"conflict":%s,"scanline":%d,"observed_pc":%d}\n',
                    active.index,frame,address,value,actual,active.color,
                    conflict and 'true' or 'false',state['ppu.scanline'] or -1,state['cpu.pc'] or 0))
                writes:flush()
            end
        end,emu.callbackType.write,bank*0x10000+0x2100,bank*0x10000+0x2133,
            emu.cpuType.snes,emu.memType.snesMemory)
    end
end
emu.addMemoryCallback(function(address,value)
    if active then
        observe('cgram',address,value,(active.color>>(address*8))&255)
    end
end,emu.callbackType.write,0,1,emu.cpuType.snes,emu.memType.snesCgRam)
local function put(address,value)
    emu.write(address,value,emu.memType.snesMemory)
end
emu.addEventCallback(function()
    if frame<120 or index>=1536 then return end
    local brightness=index%16
    local level=math.floor(index/16)%32
    local channel=math.floor(index/512)
    local color=level<<(channel*5)
    active={index=index+1,brightness=brightness,level=level,channel=channel,
            color=color,conflicts=0}
    injecting=true
    -- Visible backdrop without color math, windows or interlace/overscan.
    put(0x2100,brightness);put(0x212c,0);put(0x212d,0)
    put(0x2130,0);put(0x2131,0);put(0x2133,0)
    emu.write(0,color&255,emu.memType.snesCgRam)
    emu.write(1,(color>>8)&255,emu.memType.snesCgRam)
    injecting=false
end,emu.eventType.startFrame)
emu.addEventCallback(function()
    if active then
        local pixels=emu.getScreenBuffer()
        assert(#pixels==256*239,'unexpected PPU output geometry')
        local samples={}
        -- Full active viewport coordinate convention: output y7..230 is 224p.
        for _,point in ipairs({{0,7},{255,7},{128,119},{0,230},{255,230}}) do
            samples[#samples+1]=pixels[point[2]*256+point[1]+1]&0xffffff
        end
        local state=emu.getState()
        local cgram=emu.read(0,emu.memType.snesCgRam)|
                    (emu.read(1,emu.memType.snesCgRam)<<8)
        -- Original code may change PPU state during a scene handoff. Such an
        -- attempt is not the requested hardware experiment: retain it, retry
        -- the same input, and never select/reject based on an expected RGB.
        local reason=(state['ppu.forcedBlank'] and 1 or 0)|
            (state['ppu.screenBrightness']~=active.brightness and 2 or 0)|
            (state['ppu.mainScreenLayers']~=0 and 4 or 0)|
            (state['ppu.subScreenLayers']~=0 and 8 or 0)|
            (cgram~=active.color and 16 or 0)|
            (active.conflicts>0 and 64 or 0)
        for _,sample in ipairs(samples)do if sample~=samples[1]then reason=reason|32 end end
        local valid=reason==0
        local target=valid and output or rejected
        target:write(string.format(
            '{"case":%d,"frame":%d,"channel":%d,"level":%d,"color":%d,"brightness":%d,"samples":[%s],"forced_blank":%s,"ppu_brightness":%d,"main_layers":%d,"sub_layers":%d,"cgram_word":%d,"conflicting_writes":%d,"rejection_reason_bits":%d}\n',
            active.index,frame,active.channel,active.level,active.color,
            active.brightness,table.concat(samples,','),
            state['ppu.forcedBlank'] and 'true' or 'false',
            state['ppu.screenBrightness'],state['ppu.mainScreenLayers'],state['ppu.subScreenLayers'],cgram,active.conflicts,reason))
        target:flush();if valid then index=index+1 end;active=nil
        if index==1536 then
            output:close();rejected:close();writes:close()
            local f=assert(io.open(out..'/capture_complete.txt','wb'))
            f:write('brightness_cases=1536\n');f:close();emu.stop(0)
        end
    end
    frame=frame+1
end,emu.eventType.endFrame)
