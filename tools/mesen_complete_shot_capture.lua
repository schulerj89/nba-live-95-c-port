-- Full launch observer with optional controlled WRAM inputs. Never changes
-- ROM, PC, flags or stack. Timer writes are logged because NMI can interrupt
-- the call after $9DEA initializes $0930; that decrement is not a launch op.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local file=assert(io.open(out..'/complete_shot.vectors.jsonl','wb'))
local controlled=os.getenv('NBA95_LAUNCH_CONTROL')=='1'
local cases={}
local function add(name,values) cases[#cases+1]={name=name,values=values or {}} end
add('baseline')
for _,d in ipairs({0,7,8,39,40,79,80,103,104,127,128,159,160,300,600}) do add('distance_'..d,{A8c=d}) end
for _,d in ipairs({0,1,2}) do add('difficulty_'..d,{G17af=d,A8a=10}) end
add('hot_same',{G9c0=0,A6e=0});add('hot_other',{G9c0=5,A6e=0})
add('stamina_empty',{stamina=0});add('moving',{A4c=0x100});add('modifier',{Ab2=3})
add('late_period',{G928=100,G926=1,A8a=0})
add('opposite_basket',{basket=-280,ballx=-100,originx=-100})
add('fractional',{G3eed=0x1234,G3ef1=0x5678,G3ef5=0xabcd})
for _,v in ipairs({0,48,144,288,432,576}) do add('human_timing_'..v,{A16=0,A12=v,G17bf=0}) end
add('manual',{A16=0,G17c3=1});add('assist_clear',{assistclock=300,G928=100})
for _,seed in ipairs({1,13,42,987,65535}) do
    add('cpu_free_'..seed,{G978=1,G7f6=seed})
    add('cpu_free_alt_'..seed,{G978=1,G7f6=seed,Aa8=1,A04=-100})
end
for _,aim in ipairs({0,14,15,27,39,40,55,56,69,70,95,96,111}) do
    add('human_free_aim_'..aim,{G978=1,A16=0,G982=aim,G980=27})
end
for _,power in ipairs({0,14,15,39,40,55,56,70,95,96,111}) do
    add('human_free_power_'..power,{G978=1,A16=0,G982=27,G980=power})
end
add('manual_free',{G978=1,A16=0,G17c3=1,G3eed=0x1234,G3ef1=0x5678,G3ef5=0xabcd})

local ranges={{0,0xff},{0x7f6,0xa0f},{0x1400,0x18ff},{0x3400,0x49ff}}
local function word(a) return emu.read(a,emu.memType.snesWorkRam)|emu.read(a+1,emu.memType.snesWorkRam)<<8 end
local function put(a,v) emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam) end
local function snapshot()
    local chunks={}
    for _,r in ipairs(ranges) do
        local bytes={}
        for a=r[1],r[2] do bytes[#bytes+1]=string.format('%02x',emu.read(a,emu.memType.snesWorkRam)) end
        chunks[#chunks+1]=string.format('"%04x":"%s"',r[1],table.concat(bytes))
    end
    return '{"mem":{'..table.concat(chunks,',')..'}}'
end
local function apply(values)
    local actor,context=word(0x96),word(0x9e)
    for key,value in pairs(values) do
        if key=='stamina' then put(word(0x3435+word(0xc2)*2)+0x18,value)
        elseif key=='basket' then put(context+0xa,value)
        elseif key=='ballx' then put(0x3eef,value)
        elseif key=='originx' then put(0x900,value)
        elseif key=='assistclock' then put(context+0x47,value)
        elseif key:sub(1,1)=='A' then put(actor+tonumber(key:sub(2),16),value)
        else put(tonumber(key:sub(2),16),value) end
    end
end
local current,entry,saved,entrypc,timeout,events=nil,nil,nil,nil,nil,{}
local count,frames=0,0
emu.addMemoryCallback(function(address,value)
    if entry and timeout then
        local cpu=emu.getState()
        events[#events+1]=string.format('[%d,%d,%d,%d]',address&0xffff,value,cpu['cpu.k'] or 0,cpu['cpu.pc'] or 0)
    end
end,emu.callbackType.write,0x930,0x931,emu.cpuType.snes,emu.memType.snesWorkRam)
local function on_entry(address)
    assert(not entry,'nested launch')
    if controlled then
        current=cases[count+1]
        saved={};for _,r in ipairs(ranges) do for a=r[1],r[2] do saved[a]=emu.read(a,emu.memType.snesWorkRam) end end
        apply({A16=0xffff,G9c0=0xffff,G17af=1,G17c3=0,G978=0,A8c=100,A8a=80,
               A4c=0,Ab2=0,stamina=0x7fff,G17bf=1,G900=100,G902=0,G982=27,G980=27})
        apply(current.values)
    end
    entrypc=address;entry=snapshot();timeout=nil;events={}
end
for _,pc in ipairs({0x869d6e,0x869da6}) do
    emu.addMemoryCallback(on_entry,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
emu.addMemoryCallback(function() if entry then timeout=word(0x930) end end,
    emu.callbackType.exec,0x869ded,0x869ded,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
    if not entry then return end
    count=count+1
    file:write(string.format('{"call":%d,"entry_pc":"%06x","exit_pc":"86a476","provenance":"%s","launch_timeout":%d,"timer_writes":[%s],"entry":%s,"exit":%s}\n',
        count,entrypc,controlled and ('controlled-ROM:'..current.name) or
            (os.getenv('NBA95_LAUNCH_PROVENANCE') or 'natural-ROM'),timeout,table.concat(events,','),entry,snapshot()))
    file:flush();entry=nil
    if controlled then
        for a,v in pairs(saved) do emu.write(a,v,emu.memType.snesWorkRam) end
        if count==#cases then
            file:close();local done=assert(io.open(out..'/complete_shot.done','wb'));done:write('cases='..count);done:close();emu.stop(0)
        else put(0x92c,0) end
    end
end,emu.callbackType.exec,0x86a476,0x86a476,emu.cpuType.snes,emu.memType.snesMemory)
emu.addEventCallback(function()
    frames=frames+1
    if frames%600==0 then
        local f=assert(io.open(out..'/launch_progress.txt','wb'));f:write('frames='..frames..' calls='..count);f:close()
    end
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_SHOT_CAPTURE_DRIVER') or os.getenv('NBA95_VECTOR_DRIVER')))
