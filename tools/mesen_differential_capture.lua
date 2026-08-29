-- cpu-sweep-v1: actual execution checkpoints; no endFrame reconstruction.
-- Configures teams before commit, then observes without repairing state.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local function require_capture(condition,message)
    if condition then return end
    local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
    emu.stop(1);error(message)
end
local sweeps=assert(tonumber(os.getenv('NBA95_DIFF_SWEEPS')))
local controller_mode=os.getenv('NBA95_DIFF_CONTROLLERS') or 'cpu-vs-human'
assert(controller_mode=='cpu-vs-human' or controller_mode=='cpu-vs-cpu','unknown controller mode')
local fieldfile=assert(io.open(out..'/addresses.txt','rb'))
local addresses={};for s in fieldfile:lines()do addresses[#addresses+1]=assert(tonumber(s,16))end;fieldfile:close()
local file=assert(io.open(out..'/rom.jsonl','wb'))
local ready=false;local sequence=0;local frame=0;local complete=0;local pending=false
local writers={};local teams_confirmed=false;local controllers_configured=false
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function write(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,v>>8,emu.memType.snesWorkRam)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function checkpoint(phase)
    local values={};local sites={}
    for _,a in ipairs(addresses)do
        values[#values+1]=string.format('"%04x":%d',a,word(a))
        if writers[a]then sites[#sites+1]=string.format('"%04x":%d',a,writers[a])end
    end
    file:write(string.format('{"sequence":%d,"checkpoint":"%s","outer_frame":%d,"inputs":[0,0,0,0,0],"state":{%s},"writers":{%s}}\n',
        sequence,phase,frame,table.concat(values,','),table.concat(sites,',')))
    sequence=sequence+1;file:flush()
end
local function snapshot()
    -- Retain unprojected context for the next bootstrap increment. This is
    -- evidence only; the port never loads this file as a gameplay asset.
    local bytes={}
    for a=0,0x1ffff do bytes[#bytes+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
    local f=assert(io.open(out..'/baseline.wram','wb'));f:write(table.concat(bytes));f:close()
end
-- The configured launch is disclosed in run.json. Only pre-game selection
-- is written here; RNG, positions, timers and gameplay state are never synced.
hook(0x828553,function()
    if not ready then write(0x16fb,3);write(0x16fd,18);teams_confirmed=true end
end)
-- Configure at the actual selection consumer, not the earlier roster loader
-- or next endFrame. Presentation/setup can overwrite earlier selections.
-- Never repair actor state, and stop all launch writes at the baseline.
hook(0x86e285,function()
    if not ready and controller_mode=='cpu-vs-cpu' then
        -- $86:E285/E2D2 reads Player Setup selections at $166D+pad*2.
        -- Value1 is neutral; E29C/E2D9 skip assigning a human actor.
        -- Clearing only $08D4/etc does not prevent this later assignment.
        for a=0x166d,0x1675,2 do write(a,1)end
        controllers_configured=true
    end
end)
for _,a in ipairs(addresses)do
    local target=a
    emu.addMemoryCallback(function()
        local s=emu.getState();writers[target]=(s['cpu.k'] or 0)*65536+(s['cpu.pc'] or 0)
    end,emu.callbackType.write,target,target+1,emu.cpuType.snes,emu.memType.snesWorkRam)
end
hook(0x87a47a,function()
    if not ready then
        require_capture(teams_confirmed,'team-selection commit not observed')
        require_capture(word(0x16fb)==3 and word(0x16fd)==18,'selected teams changed before baseline')
        local humans=0
        for slot=0,9 do
            local controller=word(0x34eb+slot*256+0x16)
            if controller~=0xffff then
                require_capture(controller==0,'unexpected controller assignment at baseline')
                humans=humans+1
            end
        end
        if controller_mode=='cpu-vs-cpu' then
            require_capture(controllers_configured and humans==0,'CPU-only launch not established')
        else require_capture(humans==1,'expected one human controller at baseline')end
        ready=true;checkpoint('baseline');snapshot()
    end
end)
hook(0x878efb,function()
    if ready then require_capture(not pending,'nested actor sweep');pending=true;checkpoint('actors.begin')end
end)
hook(0x878f95,function()
    if ready and pending then
        checkpoint('actors.end');pending=false;complete=complete+1
        if complete==sweeps then
            file:close();local f=assert(io.open(out..'/differential_complete.txt','wb'))
            f:write(string.format('sweeps=%d\ncheckpoints=%d\n',complete,sequence));f:close();emu.stop(0)
        end
    end
end)
-- Minimal menu driver from the established capture route. Unlike the generic
-- routine-vector driver, this writes no gameplay state on endFrame.
local title_frame,setup_frame=-1,-1
hook(0x80e1b1,function()if title_frame<0 then title_frame=0 end end)
hook(0x80a2bf,function()if title_frame>=850 and setup_frame<0 then setup_frame=0 end end)
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
    local input={}
    if not ready then
        if setup_frame<0 then input.start=pulse(title_frame,850)
        elseif pulse(setup_frame,400) then input.start=true
        elseif setup_frame>=650 and (setup_frame-650)%200<3 then input.start=true end
    end
    emu.setInput(input,0)
    for pad=1,4 do emu.setInput({},pad)end
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
    if ready then frame=frame+1;require_capture(frame<4000,'sweep capture timeout');return end
    if title_frame>=0 and setup_frame<0 then title_frame=title_frame+1 end
    if setup_frame>=0 then
        if setup_frame>=300 and setup_frame<400 then write(0x16fb,0)end -- Exhibition
        setup_frame=setup_frame+1;require_capture(setup_frame<8000,'menu launch timeout')
    end
end,emu.eventType.endFrame)
