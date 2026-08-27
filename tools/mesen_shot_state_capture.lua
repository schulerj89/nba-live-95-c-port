-- Observe genuine shot-state calls. Optional controlled WRAM inputs are
-- labeled; never changes ROM, PC, CPU flags, or stack.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local controlled=os.getenv('NBA95_SHOT_STATE_CONTROL')=='1'
local menu_test=os.getenv('NBA95_SHOT_STATE_MENU')=='1'
local file=assert(io.open(out..'/shot_state.vectors.jsonl','wb'))
local frames,count=0,0
local ranges={{0,0xff},{0x7f6,0xa0f},{0x13f0,0x18ff},{0x3400,0x49ff}}
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function snapshot()
    local parts={};for _,r in ipairs(ranges) do local bytes={}
        for a=r[1],r[2] do bytes[#bytes+1]=string.format('%02x',emu.read(a,emu.memType.snesWorkRam)) end
        parts[#parts+1]=string.format('"%04x":"%s"',r[1],table.concat(bytes))
    end
    local cpu={};for k,v in pairs(emu.getState()) do if k:sub(1,4)=='cpu.' then cpu[#cpu+1]=string.format('"%s":%s',k:sub(5),tostring(v)) end end
    return '{"cpu":{'..table.concat(cpu,',')..'},"mem":{'..table.concat(parts,',')..'}}'
end
local cases={make={},fatigue={},clock={},fixed_grant={}}
local function add(kind,name,values)cases[kind][#cases[kind]+1]={name=name,values=values}end
add('fixed_grant','timeout_stamina_boundaries',{})
for _,shooter in ipairs({0,4,5,9}) do
    add('make','shooter_'..shooter,{[0x9c8]=shooter,[0x17c1]=0})
end
for _,clock in ipairs({0,7199,7200,7201,65535}) do
    for _,delta in ipairs({-3,-2,-1,0,1,2,3}) do
        add('make','clock_'..clock..'_score_'..delta,{[0x9c8]=0,[0x17c1]=1,[0x928]=clock,[0x4711]=100+delta,[0x4791]=100})
    end
end
for _,quarter in ipairs({0,1,2,3}) do for _,enabled in ipairs({0,1}) do
    add('fatigue','quarter_'..quarter..'_enabled_'..enabled,{[0x936]=2,[0x9c2]=60,[0x17b1]=quarter,[0x17e7]=enabled})
end end
for _,timer in ipairs({0,59,60,61,1000,65535}) do
    add('fatigue','timer_'..timer,{[0x936]=2,[0x9c2]=timer,[0x17e7]=1})
end
for _,live in ipairs({0x7f,0x80,0x82,0xffff}) do add('fatigue','live_'..live,{[0x936]=live,[0x9c2]=60}) end
for _,live in ipairs({0,2,0x7f,0x80,0x81,0x82,0xffff}) do
    for _,clock in ipairs({0,3599,3600,7199,7200}) do
        add('clock','live_'..live..'_clock_'..clock,{[0x936]=live,[0x928]=clock,[0xa04]=1,[0x926]=0,[0x9c2]=59})
    end
end
for _,values in ipairs({{[0x9c2]=60},{[0x9c2]=1000},{[0x9c2]=0x8000},
    {[0x17e1]=0},{[0x92c]=1440},{[0x92c]=0},{[0x930]=0},
    {[0x936]=0x82,[0xa04]=0},{[0x936]=0x82,[0x926]=3,[0x928]=7199},
    {[0x936]=0x82,[0x926]=3,[0x928]=7200}}) do add('clock','boundary_'..#cases.clock,values) end
local specs={
    {name='make',entry=0x85a081,exit=0x85a0eb},
    {name='fatigue',entry=0x8798ea,exit=0x879969},
    {name='recovery',entry=0x87996a,exit=0x8799c2},
    {name='grant',entry=0x87985d,exit=0x87987d},
    {name='fixed_grant',entry=0x868468,exit=0x868496},
    {name='init',entry=0x86da49,exit=0x86da61},
    {name='reset',entry=0x86dd80,exit=0x86dd89},
    {name='clock',entry=0x85edc6,exit=0x85ee3e},
    {name='timer_init',entry=0x878df3,exit=0x878df9},
}
local seen={};local completed={}
emu.addMemoryCallback(function(address,value)
    local cpu=emu.getState()
    for _,spec in ipairs(specs) do if spec.pending then
        local events=spec.pending.timer_writes
        events[#events+1]=string.format('[%d,%d,%d,%d]',address&0xffff,value,cpu['cpu.k'] or 0,cpu['cpu.pc'] or 0)
    end end
end,emu.callbackType.write,0x9c2,0x9c3,emu.cpuType.snes,emu.memType.snesWorkRam)
for _,spec in ipairs(specs) do
    emu.addMemoryCallback(function()
        if spec.pending then error('nested same writer')end
        local index=(completed[spec.name] or 0)+1
        if spec.name=='make' then index=index+(tonumber(os.getenv('NBA95_SHOT_MAKE_OFFSET')) or 0)end
        local test=(controlled or (menu_test and spec.name=='fixed_grant')) and cases[spec.name] and cases[spec.name][index]
        if controlled and cases[spec.name] and not test then return end
        if not test then
            local key=spec.name
            if spec.name=='clock' then key=key..':'..word(0x936)..':'..(word(0x9c2)>=60 and 1 or 0)..':'..(word(0x928)==0 and 1 or 0) end
            if spec.name=='fatigue' then key=key..':'..word(0x936)..':'..(word(0x9c2)>=60 and 1 or 0) end
            if (seen[key] or 0)>=(spec.name=='make' and 60 or 4) then return end
            seen[key]=(seen[key] or 0)+1
        end
        local saved=nil
        if test then
            saved={};for _,r in ipairs(ranges) do for a=r[1],r[2] do saved[a]=emu.read(a,emu.memType.snesWorkRam)end end
            if spec.name=='make' then
                for i=0,9 do put(0x34eb+i*256+0xb2,65530+i);put(0x34eb+i*256+0xb4,90+i)end
            elseif spec.name=='fatigue' then
                for i=0,23 do put(0x40eb+i*64+0x18,({0,1,50,200,32760,65535})[(i%6)+1]);put(0x40eb+i*64+0x1a,65530+i)end
                for i=0,9 do put(0x34eb+i*256+0x72,i%2)end
            elseif spec.name=='clock' then
                for a,v in pairs({[0x936]=2,[0x928]=10000,[0x9c2]=59,[0x17e1]=1,[0x92c]=100,[0x930]=100,[0x13f7]=65535,[0x13f9]=65535})do put(a,v)end
            elseif spec.name=='fixed_grant' then
                for i=0,23 do put(0x40eb+i*64+0x18,({0,1,4095,4096,28670,28671,32767,65535})[(i%8)+1])end
            end
            for a,v in pairs(test.values)do put(a,v)end
        end
        spec.pending={before=snapshot(),saved=saved,label=test and ('controlled-ROM:'..test.name) or 'natural-ROM',frame=frames,timer_writes={}}
    end,emu.callbackType.exec,spec.entry,spec.entry,emu.cpuType.snes,emu.memType.snesMemory)
    emu.addMemoryCallback(function()
        local p=spec.pending;if not p then return end
        count=count+1;completed[spec.name]=(completed[spec.name] or 0)+1
        file:write(string.format('{"call":%d,"kind":"%s","provenance":"%s","entry_frame":%d,"exit_frame":%d,"entry_pc":"%06x","exit_pc":"%06x","entry":%s,"exit":%s,"timer_writes":[%s]}\n',count,spec.name,p.label,p.frame,frames,spec.entry,spec.exit,p.before,snapshot(),table.concat(p.timer_writes,',')));file:flush()
        if p.saved then for a,v in pairs(p.saved)do emu.write(a,v,emu.memType.snesWorkRam)end end
        spec.pending=nil
        if menu_test and spec.name=='fixed_grant' then
            local done=assert(io.open(out..'/capture_complete.txt','wb'));done:write('controlled timeout-menu grant captured\n');done:close()
            emu.stop(0)
        end
    end,emu.callbackType.exec,spec.exit,spec.exit,emu.cpuType.snes,emu.memType.snesMemory)
end
emu.addEventCallback(function()
    frames=frames+1
    if frames%600==0 then
        local progress=assert(io.open(out..'/shot_state_progress.txt','wb'))
        progress:write('frames='..frames..' calls='..count..'\n')
        for name,n in pairs(completed)do progress:write(name..'='..n..'\n')end
        progress:close()
    end
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
if menu_test then
    -- Enter the real pause menu with controller input. Select its timeout
    -- entry as controlled menu input; never alter PC/ROM/stack/CPU flags.
    emu.addEventCallback(function()
        if frames>=5000 then
            emu.setInput({start=frames<5003,a=frames>=5200 and frames%60<3},0)
        end
    end,emu.eventType.inputPolled)
    emu.addMemoryCallback(function()put(0x4981,0)end,
        emu.callbackType.exec,0x868369,0x868369,emu.cpuType.snes,emu.memType.snesMemory)
end
