-- First entry only. Original CPU executes normally; controller input is the
-- only mutation. No ROM/register/WRAM/SRAM/PPU writes or native scene jumps.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local target=assert(tonumber(os.getenv('NBA95_RETAIL_MODE')))
assert(target>=0 and target<=3)
local function json(v)
    if type(v)=='string' then return '"'..v:gsub('\\','\\\\'):gsub('"','\\"')..'"' end
    if type(v)~='table' then return tostring(v) end
    local values={}
    if #v>0 then for _,x in ipairs(v) do values[#values+1]=json(x) end
        return '['..table.concat(values,',')..']' end
    local keys={};for k in pairs(v) do keys[#keys+1]=k end;table.sort(keys)
    for _,k in ipairs(keys) do values[#values+1]=json(k)..':'..json(v[k]) end
    return '{'..table.concat(values,',')..'}'
end
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder());home:close()
local log=assert(io.open(out..'/route-events.jsonl','wb'))
local inputlog=assert(io.open(out..'/input-frames.jsonl','wb'))
local frame,title,setup,entered=0,-1,-1,-1
local events,observed_mode=0,-1
local seen={}
local function word(a)
    return emu.read(a,emu.memType.snesWorkRam,false)|
        (emu.read(a+1,emu.memType.snesWorkRam,false)<<8)
end
local function words(a,n)
    local v={};for i=0,n-1 do v[#v+1]=word(a+i*2) end;return v
end
local function state(kind,pc)
    local cpu=emu.getState();local save={}
    for a=0x48,0x56 do save[#save+1]=emu.read(0x700000+a,emu.memType.snesMemory,false) end
    return {kind=kind,pc=pc,frame=frame,title_frame=title,setup_frame=setup,
        main=words(0x17ab,4),working=words(0x16fb,4),rules=words(0x17d1,13),
        options=words(0x17b5,7),selection=words(0x166d,5),team16b1=word(0x16b1),
        team16b3=word(0x16b3),field17a7=word(0x17a7),cancel1753=word(0x1753),
        field0a5c=word(0xa5c),field07f8=word(0x7f8),field0bcb=word(0xbcb),
        field8e40=word(0x8e40),field8f48=word(0x8f48),sram48_56=save,
        marker=emu.read(0x700004,emu.memType.snesMemory,false),
        a=cpu['cpu.a'],x=cpu['cpu.x'],y=cpu['cpu.y'],p=cpu['cpu.ps']}
end
local function dump(name)
    local bytes={};for a=0,0x1ffff do bytes[#bytes+1]=string.char(emu.read(a,emu.memType.snesWorkRam,false)) end
    local f=assert(io.open(out..'/'..name..'.wram.bin','wb'));f:write(table.concat(bytes));f:close()
end
local function hook(pc,callback)
    emu.addMemoryCallback(callback,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
hook(0x80e1b1,function() if title<0 then title=0 end end)
hook(0x80a2bf,function() if title>=850 and setup<0 then setup=0 end end)
local entries={[0x82809a]=true,[0x81c54f]=true,[0x84a2f3]=true,[0x82d085]=true}
for _,pc in ipairs({0x81c19a,0x81c1a9,0x81c24b,0x81c0ba,0x81c199,
    0x80dba8,0x82809a,0x81c54f,0x848103,0x84a2f3,0x84a328,0x84ab5f,
    0x84a9dd,0x81a489,0x84a7a9,0x828553,0x828563,0x81bf99,0x82d085}) do
    hook(pc,function()
        log:write(json(state('entry',pc))..'\n');log:flush()
        if setup>=400+target*60 and entries[pc] then
            assert(word(0x17ab)==target,'Mode mismatch at original entry')
            events=events+1;observed_mode=word(0x17ab)
            if entered<0 then entered=frame end
        end
        if not seen[pc] then seen[pc]=true;dump(string.format('entry-%06x',pc)) end
    end)
end
local function pulse(n,at) return n>=at and n<at+3 end
emu.addEventCallback(function()
    local input={}
    if setup<0 then input.start=pulse(title,850)
    else
        for i=0,target-1 do if pulse(setup,400+i*60) then input.right=true end end
        input.start=pulse(setup,400+target*60)
    end
    inputlog:write(json({frame=frame,setup_frame=setup,start=input.start or false,right=input.right or false})..'\n')
    emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
    frame=frame+1
    if title>=0 and setup<0 then title=title+1 end
    if setup>=0 then setup=setup+1 end
    if frame>=6500 then
        local f=assert(io.open(out..'/capture_failed.txt','wb'))
        f:write('No completed mode-entry route before frame limit');f:close()
        log:close();inputlog:close();emu.stop(1);return
    end
    if entered>=0 and frame-entered>=300 then
        log:write(json(state('entry_plus_300_frames',0))..'\n');log:close();inputlog:close()
        dump('final')
        local complete={requested_mode=target,observed_mode=observed_mode,
            route_events=events,entry_frame=entered,final_frame=frame}
        local f=assert(io.open(out..'/capture_complete.json','wb'));f:write(json(complete));f:close()
        emu.stop(0)
    end
end,emu.eventType.endFrame)
