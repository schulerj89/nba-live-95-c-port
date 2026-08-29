-- Observe EC32's parent decisions at native call/return boundaries. Child
-- animation/reach effects are explicitly outside this decision projection.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local file=assert(io.open(out..'/jump-reach.jsonl','wb'))
local active=nil;local frame=0;local ready=false;local serial=0;local completed=0
local raw46_writer=0
emu.addMemoryCallback(function()
    local s=emu.getState();raw46_writer=s['cpu.k']*65536+s['cpu.pc']
end,emu.callbackType.write,0x46,0x47,emu.cpuType.snes,emu.memType.snesWorkRam)
local cases={};local controlled=os.getenv('NBA95_DIFF_JUMP_CASES')
if controlled then
    local f=assert(io.open(controlled,'rb'))
    for line in f:lines()do local v={};for x in line:gmatch('%d+')do v[#v+1]=tonumber(x)end;assert(#v==28);cases[#cases+1]=v end
    f:close()
end
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function arr(t)local s={};for _,v in ipairs(t)do s[#s+1]=tostring(v)end;return '['..table.concat(s,',')..']'end
local function outputs(a)return {w(a+0xe),w(a+0x10),w(a+0x12),w(0x7f6)}end
local function channels(a)
    local v={};for _,o in ipairs({0x18,0x1a,0x30,0x32,0x38,0x3a,0x3c,0x42,0x44,0x46,0x48,0x1c,0x1e,0x20,0x22,0x24,0x26,0xb0})do v[#v+1]=w(a+o)end
    return v
end
local function inputs()
    local a=w(0x96);local b=w(0x910);local pair=emu.readWord(0x879c7b+w(a+0x74),emu.memType.snesMemory)
    local roster=w(0xe0)|(emu.read(0xe2,emu.memType.snesWorkRam)<<16)
    local v={}
    for _,p in ipairs({a+4,a+8,a+0xc,a+0x32,a+0x8e,a+0x86,a+0x4c,
        b+4,b+8,b+0xc,b+0x12,b+0x88,pair+0x86,0x3eef,0x3ef7,0x3efd,
        0x948,0x93e,0x946,0x936,0x962,0x46,a+0xe,a+0x10,a+0x12,0x7f6})do v[#v+1]=w(p)end
    v[#v+1]=emu.read(roster+0x3c,emu.memType.snesMemory)
    v[#v+1]=emu.read(roster+0x3d,emu.memType.snesMemory)
    return v,roster
end
hook(0x87a47a,function()ready=true end)
emu.addEventCallback(function()if ready then frame=frame+1 end end,emu.eventType.endFrame)
hook(0x86ec32,function()
    if not ready then return end
    assert(not active,'nested jump/reach entry')
    if controlled and serial>=#cases then return end
    serial=serial+1;local saved=nil
    if controlled then
        saved={}
        for _,r in ipairs({{0,255},{0x700,0xa10},{0x13e0,0x187f},{0x3400,0x5000},{0x17000,0x17040}})do
            for a=r[1],r[2]do saved[a]=emu.read(a,emu.memType.snesWorkRam)end
        end
        local v=cases[serial];local a=w(0x96)
        local pair=emu.readWord(0x879c7b+w(a+0x74),emu.memType.snesMemory)
        local b=v[17]~=0 and pair or 0x3eeb
        put(0x910,b);put(0xe0,0x7000);put(0xe2,0x7f)
        for i,p in ipairs({a+4,a+8,a+0xc,a+0x32,a+0x8e,a+0x86,a+0x4c,
            b+4,b+8,b+0xc,b+0x12,b+0x88,pair+0x86,0x3eef,0x3ef7,0x3efd,
            0x948,0x93e,0x946,0x936,0x962,0x46,a+0xe,a+0x10,a+0x12,0x7f6})do put(p,v[i])end
        emu.write(0x1703c,v[27],emu.memType.snesWorkRam);emu.write(0x1703d,v[28],emu.memType.snesWorkRam)
    end
    local s=emu.getState();local v,roster=inputs()
    local sp=s['cpu.sp'];local caller=(w(sp+1)+1)&65535
    caller=caller|(emu.read(sp+3,emu.memType.snesWorkRam)<<16)
    active={input=v,roster=roster,actor=w(0x96),frame=frame,pcs={},calls={},cpu={s['cpu.d'],s['cpu.dbr'],s['cpu.ps']},serial=serial,saved=saved,caller=caller,writer=raw46_writer}
    active.channels=channels(active.actor);active.options={w(active.actor+0x72),w(active.actor+0xa8)}
end)
emu.addMemoryCallback(function(pc)if active then active.pcs[pc]=true end end,
    emu.callbackType.exec,0x86ec32,0x86ee75,emu.cpuType.snes,emu.memType.snesMemory)
-- Capture at the parent's JSL instruction, not inside nested animation calls.
for _,row in ipairs({{0x86ec9c,0x86eaa8},{0x86ece0,0x87b47a},{0x86ece9,0x87b4db},
    {0x86ecf4,0x87b3bd},{0x86ee33,0x87b47a},{0x86ee3c,0x87b4db},{0x86ee71,0x86bd1f}})do
    hook(row[1],function()if active then
        if not active.output then active.output=outputs(active.actor)end
        active.calls[#active.calls+1]=arr({row[2],row[2]>=0x870000 and w(0) or 0})
    end end)
end
for _,pc in ipairs({0x86eca0,0x86ecf8,0x86ee40,0x86ee75})do hook(pc,function()
    if not active then return end
    local v=active;active=nil;v.pcs[pc]=true;completed=completed+1
    local pcs={};for p in pairs(v.pcs)do pcs[#pcs+1]=p end;table.sort(pcs)
    file:write(string.format('{"case":%d,"controlled":%s,"frame":%d,"actor":%d,"roster":%d,"abi":%s,"input":%s,"output":%s,"calls":[%s],"exit":%d,"pcs":%s,"return_pc":%d,"raw46_writer_observed_pc":%d,"channels_in":%s,"channels_out":%s,"animation_options":%s}\n',
        v.serial,tostring(v.saved~=nil),v.frame,v.actor,v.roster,arr(v.cpu),arr(v.input),arr(v.output or outputs(v.actor)),table.concat(v.calls,','),pc,arr(pcs),v.caller,v.writer,
        arr(v.channels),arr(channels(v.actor)),arr(v.options)));file:flush()
    if v.saved then for a,value in pairs(v.saved)do emu.write(a,value,emu.memType.snesWorkRam)end end
    local progress=assert(io.open(out..'/jump-reach-progress.json','wb'))
    progress:write(string.format('{"started":%d,"completed":%d,"expected_controlled":%d}',serial,completed,#cases));progress:close()
end)end
hook(0x878f95,function()
    if not ready then return end
    local f=assert(io.open(out..'/jump-reach-progress.json','wb'))
    f:write(string.format('{"started":%d,"completed":%d,"expected_controlled":%d}',serial,completed,#cases));f:close()
end)
dofile(assert(os.getenv('NBA95_DIFF_DRIVER')))
