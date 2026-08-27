-- Native tip-off call boundaries. ROM screenshots are evidence, never art.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local file=assert(io.open(out..'/tip_flow.jsonl','wb'))
local frames=assert(io.open(out..'/tip_frames.jsonl','wb'))
local frame=0;local ready=false;local active={}
local controlled=os.getenv('NBA95_TIP_CONTROL')=='1';local saved=nil;local case_index=0;local cases={}
local variant=tonumber(os.getenv('NBA95_TIP_VARIANT')) or -1
local launch_control=os.getenv('NBA95_TIP_LAUNCH_CONTROL')=='1';local launch_index=0
local ranges={{0,0xff},{0x700,0xa10},{0x13e0,0x14c0},{0x1800,0x187f},{0x34eb,0x3fff},{0x46eb,0x486b},{0x4900,0x4960}}
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
for _,dy in ipairs({-17,-16,0,15,16})do for _,z in ipairs({-1,0,55,60,71,72})do cases[#cases+1]={dy=dy,z=z}end end
for _,dx in ipairs({-17,-16,-8,0,7,8,15,16})do for _,z in ipairs({55,56,85,86})do cases[#cases+1]={dx=dx,z=z,hoop=0}end end
for _,receiver in ipairs({-1,0,5,8})do for _,lock in ipairs({0,1})do for _,z in ipairs({59,60,67,95,96})do cases[#cases+1]={receiver=receiver,lock=lock,z=z}end end end
for _,inhibit in ipairs({0,1})do for _,ft in ipairs({0,1})do cases[#cases+1]={inhibit=inhibit,ft=ft,z=50}end end
if launch_control then cases={};for i=1,300 do cases[i]={z=50}end end
local function restore()
    if not saved then return end
    for a,v in pairs(saved)do emu.write(a,v,emu.memType.snesWorkRam)end;saved=nil
end
local function control()
    if not controlled or saved or frame<120 or frame>=200 then return end
    local test=cases[case_index+1];if not test then return end;case_index=case_index+1
    saved={};for _,r in ipairs(ranges)do for a=r[1],r[2]do saved[a]=emu.read(a,emu.memType.snesWorkRam)end end
    local a=w(0x9a)
    put(0x93e,0xffff);put(0x946,test.receiver or 0xffff);put(0x93a,0xffff);put(0x948,0)
    put(0x978,test.ft or 0);put(0x492f,8);put(a+0x5a,test.inhibit or 0);put(a+0x46,test.lock or 0)
    put(a+4,test.dx or 0);put(a+8,test.dy or 0);put(a+0xc,0);put(a+0xaa,60)
    put(0x3eef,0);put(0x3ef3,0);put(0x3ef7,test.z);put(0x3fef,test.hoop or -336)
end
local function arr(t)local s={};for _,v in ipairs(t)do s[#s+1]=tostring(v)end;return '['..table.concat(s,',')..']'end
local function mem()
    local parts={};for _,r in ipairs(ranges)do local bytes={};for a=r[1],r[2]do bytes[#bytes+1]=string.format('%02x',emu.read(a,emu.memType.snesWorkRam))end
        parts[#parts+1]='"'..string.format('%04x',r[1])..'":"'..table.concat(bytes)..'"' end
    return '{'..table.concat(parts,',')..'}'
end
local function regs()local s=emu.getState();return arr({s['cpu.a'],s['cpu.x'],s['cpu.y'],s['cpu.ps'],s['cpu.d'],s['cpu.dbr'],s['cpu.sp']})end
local function begin(kind,pc)
    assert(not active[kind],kind..' nested entry');active[kind]={frame=frame,pc=pc,input=mem(),regs=regs(),pcs={},points={},controlled=saved~=nil}
end
local function finish(kind,pc)
    local v=active[kind];if not v then return end;v.pcs[pc]=true
    local pcs={};for p in pairs(v.pcs)do pcs[#pcs+1]=p end;table.sort(pcs)
    file:write('{"kind":"'..kind..'","controlled":'..tostring(v.controlled)..',"case":'..case_index..',"frame":'..v.frame..',"entry_pc":'..v.pc..',"exit_pc":'..pc..',"entry_regs":'..v.regs..',"exit_regs":'..regs()..',"entry":'..v.input..',"exit":'..mem()..',"points":'..arr(v.points)..',"executed":'..arr(pcs)..'}\n');file:flush();active[kind]=nil
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x87a47a,function()ready=true end)
hook(0x86ccfc,function()if ready and frame>=120 and frame<=260 and w(0x936)==0x81 then control();begin('contact',0x86ccfc)end end)
for _,p in ipairs({0x86cf9f,0x86cfa0,0x86d43e})do hook(p,function()finish('contact',p)end)end
hook(0x86d25a,function()if ready and frame<=300 then begin('acquisition',0x86d25a)end end)
hook(0x86baa2,function()if ready and frame<=300 then begin('catch_core',0x86baa2)end end)
hook(0x86bc99,function()finish('catch_core',0x86bc99)end)
hook(0x86d365,function()if ready and frame<=300 then begin('completion',0x86d365)end end)
hook(0x86d3c6,function()if ready and frame<=300 then begin('tip_bridge',0x86d3c6)end end)
hook(0x86d3c2,function()finish('tip_bridge',0x86d3c2)end)
hook(0x86d3c5,function()finish('completion',0x86d3c5);finish('acquisition',0x86d3c5);restore()end)
hook(0x86cf9f,restore)
hook(0x86d548,restore)
hook(0x86b04c,function()if ready and frame<=300 then
    if variant>=0 and not saved then
        saved={};for _,r in ipairs(ranges)do for a=r[1],r[2]do saved[a]=emu.read(a,emu.memType.snesWorkRam)end end
        put(0x7f6,variant%2==0 and 1 or 0x8000)
        put(w(0x96)+0x6e,variant<2 and 0 or 5)
        put(0x13e9,0x40)
    end
    begin('receiver',0x86b04c)
end end)
for _,p in ipairs({0x86b0d7,0x86b0e1})do hook(p,function()finish('receiver',p)end)end
hook(0x8699c4,function()if ready and frame<=300 then
    if launch_control and saved then
        local n=launch_index;launch_index=n+1
        local a=w(0x96);local r=w(0x8e)
        put(a+0xc0,({0xffff,0,1})[(n%3)+1]);put(a+0x62,(n%6)*6)
        put(a+0xc,n%2==0 and 0 or 16);put(a+0x30,({0x25,0x2b,0x2c})[(n%3)+1])
        put(a+0x5e,n%4==0 and 15 or 11);put(r+0x5e,n%5==0 and 14 or 10)
        put(0x936,n%7==0 and 2 or 0x81)
        put(r+4,({0,360,-360,400,-400})[(n%5)+1]);put(r+8,({0,190,-190,220,-220})[(math.floor(n/5)%5)+1])
        put(r+0xe,n%2==0 and 600 or -600);put(r+0x10,n%3==0 and 400 or -400)
        put(0x3eed,0x1234);put(0x3ef1,0x5678);put(0x3ef5,0x9abc)
    end
    begin('deflection',0x8699c4)
end end)
hook(0x869bb0,function()finish('deflection',0x869bb0)end)
hook(0x869846,function()if active.deflection then begin('restore_mode',0x869846)end end)
hook(0x86986c,function()finish('restore_mode',0x86986c)end)
for _,p in ipairs({0x86d550,0x86d598})do hook(p,function()
    local v=active.contact;if not v then return end
    local a=w(0x9a);for _,x in ipairs({w(a+4)+w(0),w(a+8)+w(2),w(a+0xc)+w(4)})do v.points[#v.points+1]=x&0xffff end
end)end
emu.addMemoryCallback(function(pc)for _,v in pairs(active)do v.pcs[pc]=true end end,emu.callbackType.exec,0x868000,0x86ffff,emu.cpuType.snes,emu.memType.snesMemory)
emu.addEventCallback(function()
    if not ready then return end;frame=frame+1
    if frame>=140 and frame<=260 and not controlled then
        local shot=assert(io.open(out..'/native_'..string.format('%04d',frame)..'.png','wb'))
        shot:write(emu.takeScreenshot());shot:close()
    end
    if frame<=360 then
        local v={};for _,a in ipairs({0x7f6,0x928,0x932,0x936,0x93a,0x93e,0x942,0x944,0x946,0x948,0x94a,0x3eed,0x3eef,0x3ef1,0x3ef3,0x3ef5,0x3ef7,0x3ef9,0x3efb,0x3efd})do v[#v+1]=w(a)end
        for slot=0,9 do for _,off in ipairs({4,8,0xc,0xe,0x10,0x12,0x30,0x32,0x34,0x36,0x3a,0x3c,0x46,0x48,0x4e,0x52,0x5a,0x5e,0x60,0x62,0xaa})do v[#v+1]=w(0x34eb+slot*0x100+off)end end
        frames:write('{"frame":'..frame..',"words":'..arr(v)..'}\n');frames:flush()
    end
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
