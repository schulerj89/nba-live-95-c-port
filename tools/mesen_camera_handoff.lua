-- Native camera oracle. Only controlled WRAM inputs; no ROM/PC/flags patch.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local controlled=os.getenv('NBA95_CAMERA_CONTROL')=='1'
local init_actor=os.getenv('NBA95_CAMERA_INIT_ACTOR')=='1'
local file=assert(io.open(out..'/camera_handoff.jsonl','wb'))
local frames=assert(io.open(out..'/camera_frames.jsonl','wb'))
local writes=assert(io.open(out..'/state_writes.jsonl','wb'))
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function arr(t)local s={};for _,v in ipairs(t)do s[#s+1]=tostring(v)end;return '['..table.concat(s,',')..']'end
local function read(t)local v={};for _,a in ipairs(t)do v[#v+1]=w(a)end;return v end
local inputs={0x85c,0x860,0x85e,0x862,0x890,0x892,0x4a54,0x4a56,0x4a58,0x4a5a,0x4a5c,0x3ef7,0x93a,0x46f5,0x4775,0x8bc,0x8cc,0x936}
local outputs={0x85c,0x860,0x85e,0x862,0x88c,0x88e,0x890,0x892,0x4a54}
local active={};local count=0;local ready=false;local frame=0;local cases={}
emu.addMemoryCallback(function(address,value)
    if frame<=230 then local s=emu.getState();writes:write('{"frame":'..frame..',"pc":'..((s['cpu.k']<<16)|s['cpu.pc'])..',"address":'..address..',"value":'..value..'}\n');writes:flush()end
end,emu.callbackType.write,0x7e0936,0x7e0937,emu.cpuType.snes,emu.memType.snesMemory)
local function add(t)cases[#cases+1]=t end
for _,initial in ipairs({0,0xffff})do for _,side in ipairs({0xffff,0,5})do for _,reverse in ipairs({false,true})do
    for _,xy in ipairs({{394,224},{-394,-224},{393,223},{-393,-223},{0,0}})do
        add({[0x4a54]=initial,[0x93a]=side,[0x46f5]=reverse and 336 or -336,[0x4775]=reverse and -336 or 336,
            [0x4a58]=xy[1],[0x4a5c]=xy[2],[0x4a56]=0x81ff,[0x4a5a]=0x8101,[0x8bc]=0,[0x8cc]=0,[0x936]=2,
            [0x85c]=-140,[0x860]=-100,[0x85e]=-145,[0x862]=-110,[0x890]=13,[0x892]=17})
    end
end end end
for _,bc in ipairs({0,1,0x8000})do for _,cc in ipairs({0,1,2})do for _,state in ipairs({1,2,0x82})do for _,z in ipairs({0,55,56,57,180})do
    add({[0x4a54]=0xffff,[0x8bc]=bc,[0x8cc]=cc,[0x936]=state,[0x3ef7]=z,[0x93a]=0xffff,
        [0x4a58]=40,[0x4a5c]=20,[0x4a56]=0xffff,[0x4a5a]=1})
end end end end
local function begin(kind,test)
    assert(not active[kind]);local p={kind=kind,pcs={},frame=frame,test=test}
    if test then p.saved={};for a=0,0xffff do p.saved[a]=emu.read(a,emu.memType.snesWorkRam)end;for a,v in pairs(test)do put(a,v)end end
    active[kind]=p;return p
end
local function finish(kind,expected)
    local p=active[kind];if not p then return end
    local pcs={};for pc in pairs(p.pcs)do pcs[#pcs+1]=pc end;table.sort(pcs)
    if kind=='cadence' then p.input[#p.input+1]=#p.samples;for _,v in ipairs(p.samples)do p.input[#p.input+1]=v end end
    if not p.tainted then file:write('{"kind":"'..kind..'","controlled":'..tostring(p.test~=nil)..',"frame":'..p.frame..',"input":'..arr(p.input)..',"expected":'..arr(expected)..',"executed":'..arr(pcs)..'}\n');file:flush()end
    if p.saved then for a,v in pairs(p.saved)do emu.write(a,v,emu.memType.snesWorkRam)end end
    active[kind]=nil
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function pcs(kind,lo,hi)emu.addMemoryCallback(function(pc)if active[kind]then active[kind].pcs[pc]=true end end,emu.callbackType.exec,lo,hi,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x858b98,function()
    local test=init_actor and {[0x940]=0x34eb,[0x34ed]=0x1234,[0x34ef]=80,[0x34f1]=0x9876,[0x34f3]=40} or nil
    local p=begin('init',test);local ptr=w(0x940);if ptr==0 then ptr=0x3eeb end
    p.input=read(inputs);p.input[#p.input+1]=w(0x940)
    for a=ptr+2,ptr+8,2 do p.input[#p.input+1]=w(a)end
end)
pcs('init',0x858b98,0x858bbe)
hook(0x858bbf,function()local v=read(outputs);for _,x in ipairs(read({0x4a56,0x4a58,0x4a5a,0x4a5c}))do v[#v+1]=x end;finish('init',v)end)
hook(0x859192,function()
    count=count+1;if count>1800 then return end
    local test=controlled and ready and frame>=400 and cases[count-200] or nil
    local p=begin('core',test);p.input=read(inputs)
end)
pcs('core',0x859192,0x8593f4)
for _,pc in ipairs({0x859351,0x8593f4})do hook(pc,function()if active.core then active.core.pcs[pc]=true end;finish('core',read(outputs))end)end
hook(0x87a9d0,function()
    local test=nil
    if controlled and ready and frame>=500 and frame<540 then test={[0x93e]=(frame%12)-2}end
    local p=begin('resolve',test);p.input=read({0x93e,0x940})
    if test and active.cadence then active.cadence.tainted=true end
end)
pcs('resolve',0x87a9d0,0x87a9e2)
for _,pc in ipairs({0x87a9de,0x87a9e2})do hook(pc,function()if active.resolve then active.resolve.pcs[pc]=true end;finish('resolve',{w(0x940)})end)end
hook(0x8795ac,function()
    local p=begin('cadence');p.input=read({0x93e,0x940,0x564});p.samples={}
end)
hook(0x8795b0,function()local p=active.cadence;if p and p.samples[#p.samples]~=w(0x564)then p.samples[#p.samples+1]=w(0x564)end end)
pcs('cadence',0x8795ac,0x8795ba)
hook(0x8795bb,function()
    finish('cadence',read({0x940,0x564}))
    local p=begin('copy');local ptr=w(0x940);if ptr==0 then ptr=0x3eeb end
    p.input={w(0x940)};for a=ptr+2,ptr+8,2 do p.input[#p.input+1]=w(a)end
end)
pcs('copy',0x8795bb,0x8795de)
hook(0x858e1c,function()finish('copy',read({0x4a56,0x4a58,0x4a5a,0x4a5c}))end)
hook(0x87a47a,function()ready=true end)
emu.addEventCallback(function()
    if not ready then return end;frame=frame+1
    if frame<=700 then
        frames:write('{"frame":'..frame..',"raw":'..arr(read({0x85c,0x860,0x85e,0x862,0x4a54,0x4a56,0x4a58,0x4a5a,0x4a5c,0x93e,0x940,0x93a,0x936,0x8bc,0x8cc,0x564,0x3eed,0x3eef,0x3ef1,0x3ef3,0x3ef7}))..'}\n');frames:flush()
    end
    if not controlled and not init_actor and frame>=120 and frame<=360 then
        local image=assert(io.open(out..'/native_'..string.format('%04d',frame)..'.png','wb'));image:write(emu.takeScreenshot());image:close()
    end
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
