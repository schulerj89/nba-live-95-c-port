-- Native ROM oracle, not port-derived expected values. Controlled cases change
-- WRAM inputs at real routine entries, never PC, flags, stack or ROM bytes.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local kind=os.getenv('NBA95_CAMERA_KIND') or 'stream'
local controlled=os.getenv('NBA95_CAMERA_CONTROL')=='1'
local f=assert(io.open(out..'/camera_'..kind..'.jsonl','wb'))
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function arr(t)local s={};for _,v in ipairs(t)do s[#s+1]=tostring(v)end;return '['..table.concat(s,',')..']'end
local offsets=kind=='core' and {0x85c,0x860,0x85e,0x862,0x4a56,0x4a58,0x4a5a,0x4a5c,0x3ef7,0x93a,0x936}
    or kind=='wrapper' and {0x85c,0x860,0x926,0x46f5,0x4775,0x3fef,0x87c,0x87e,0x880,0x882}
    or {0x85c,0x860,0x85e,0x862,0x611,0x613,0x86c,0x86e,0x870,0x874,0x876,0x878,0x87a,0xec}
local outputs=kind=='core' and {0x85c,0x860,0x85e,0x862,0x88c,0x88e,0x890,0x892}
    or kind=='wrapper' and {0x3fef,0x87c,0x87e,0x880,0x882}
    or {0x86c,0x86e,0x870,0x874,0x876,0x878,0x87a}
local function read(addrs)local t={};for _,a in ipairs(addrs)do t[#t+1]=w(a)end;return t end
local function rowbuffer()local t={};for a=0x498e,0x4a52,2 do t[#t+1]=w(a)end;return t end
local cases={}
local function add(name,t)cases[#cases+1]={name=name,values=t}end
if kind=='wrapper' then
    for period=0,3 do for _,x in ipairs({-582,-500,-400,-311,-310,-309,-128,0,36,37,38,160,300,328})do
        add('period'..period..'-x'..x,{[0x85c]=x,[0x860]=-100,[0x926]=period,[0x46f5]=-336,[0x4775]=336,[0x87e]=123})
    end end
elseif kind=='stream' then
    for _,xy in ipairs({{0,0},{1,1},{31,0},{32,3},{63,4},{64,31},{100,30},{113,23}})do
        for _,delta in ipairs({{0,0},{1,0},{-1,0},{0,1},{0,-1},{3,3},{-3,-3},{1,-1},{-1,1}})do
            local cx,cy=xy[1],xy[2];local nx,ny=cx+delta[1],cy+delta[2]
            if nx>=0 and nx<=113 and ny>=0 and ny<=23 then
                local dest=((cx&31)|((cx&32)<<5)|((cy&31)<<5))
                add('tile'..cx..'-'..cy..'-delta'..delta[1]..'-'..delta[2],{
                    [0x85c]=nx*8-582,[0x860]=ny*8-242,[0x85e]=cx*8-582,[0x862]=cy*8-242,
                    [0x611]=cx*8,[0x613]=cy*8,[0x86c]=cx,[0x86e]=cy,[0x874]=dest,
                    [0x876]=0x8006+cx*104+cy*2,[0x878]=123,[0x87a]=456,[0xec]=0xa0,[0x561]=0})
            end
        end
    end
    add('vertical-circular-wrap',{[0x85c]=-134,[0x860]=-122,[0x85e]=-134,[0x862]=-130,
        [0x611]=0,[0x613]=248,[0x86c]=56,[0x86e]=14,[0x874]=0xbe0,
        [0x876]=0x8006+56*104+14*2,[0x878]=123,[0x87a]=456,[0xec]=0xa0,[0x561]=0})
else
    -- Restrict inputs to this goal's 212 slices: initialized camera, ordinary
    -- team orientation, no special-height flags, representable 8.8 fractions.
    for _,side in ipairs({0,5})do for _,x in ipairs({-394,-388,-359,-223,-110,0,110,230,256,387,394})do
        for _,y in ipairs({-224,0,224})do for _,prior in ipairs({0,8,24})do
            add('side'..side..'-x'..x..'-y'..y..'-p'..prior,{[0x4a54]=0xffff,[0x8bc]=0,[0x8cc]=0,
                [0x93a]=side,[0x46f5]=-336,[0x4775]=336,[0x936]=2,[0x4a58]=x,[0x4a5c]=y,
                [0x4a56]=0,[0x4a5a]=0,[0x85c]=-128,[0x860]=-124,[0x85e]=-128-prior,[0x862]=-124+prior})
        end end
    end end
    for _,z in ipairs({0,55,56,57,200})do add('height'..z,{[0x4a54]=0xffff,[0x8bc]=0,[0x8cc]=0,[0x93a]=5,
        [0x46f5]=-336,[0x4775]=336,[0x936]=1,[0x3ef7]=z,[0x4a58]=0,[0x4a5c]=0,[0x4a56]=0,[0x4a5a]=0})end
end
local entry=kind=='core' and 0x859192 or kind=='wrapper' and 0x858e1c or 0x858ee6
local exits=kind=='core' and {0x8593f4} or kind=='wrapper' and {0x858ee5} or {0x858f0c,0x858fe3,0x8590c3}
local lo,hi=entry,kind=='core' and 0x8593f4 or kind=='wrapper' and 0x858ee5 or 0x8590c3
local p,calls,completed,ready=nil,0,0,false
emu.addMemoryCallback(function()ready=true end,emu.callbackType.exec,0x87a47a,0x87a47a,emu.cpuType.snes,emu.memType.snesMemory)
local function applycase()
    if p.test then for a,v in pairs(p.test.values)do put(a,v)end end
    p.input=read(offsets)
    if kind=='stream' then p.buffer_before=rowbuffer()end
end
emu.addMemoryCallback(function()
    if not ready or calls>=1000 then return end
    assert(not p,'nested camera witness')
    calls=calls+1;p={pcs={},transfers={},test=controlled and cases[calls] or nil}
    if p.test then
        p.saved={};for a=0,0xffff do p.saved[a]=emu.read(a,emu.memType.snesWorkRam)end
    end
    if kind~='wrapper' then applycase()end
end,emu.callbackType.exec,entry,entry,emu.cpuType.snes,emu.memType.snesMemory)
if kind=='wrapper' then
    emu.addMemoryCallback(function()if p then applycase()end end,emu.callbackType.exec,0x858e28,0x858e28,emu.cpuType.snes,emu.memType.snesMemory)
    emu.addMemoryCallback(function()if p then p.output=read(outputs)end end,emu.callbackType.exec,0x858edd,0x858edd,emu.cpuType.snes,emu.memType.snesMemory)
end
emu.addMemoryCallback(function(pc)if p then p.pcs[pc]=true end end,emu.callbackType.exec,lo,hi,emu.cpuType.snes,emu.memType.snesMemory)
if kind=='stream' then emu.addMemoryCallback(function()
    if p then local s=emu.getState();p.transfers[#p.transfers+1]={w(0xc),w(0xe)&255,s['cpu.x'],s['cpu.y']}end
end,emu.callbackType.exec,0x808ba1,0x808ba1,emu.cpuType.snes,emu.memType.snesMemory)end
for _,exit in ipairs(exits)do emu.addMemoryCallback(function()
    if not p then return end
    p.pcs[exit]=true;local pcs={};for pc in pairs(p.pcs)do pcs[#pcs+1]=pc end;table.sort(pcs)
    local tx={};for _,t in ipairs(p.transfers)do tx[#tx+1]=arr(t)end
    f:write('{"kind":"'..kind..'","provenance":"'..(p.test and 'controlled-ROM:'..p.test.name or 'natural-ROM')..'",'..
        '"input":'..arr(p.input)..',"expected":'..arr(p.output or read(outputs))..',"executed":'..arr(pcs)..
        ',"transfers":['..table.concat(tx,',')..'],"buffer_before":'..arr(p.buffer_before or {})..
        ',"buffer_after":'..arr(kind=='stream' and rowbuffer() or {})..'}\n');f:flush()
    if p.saved then for a,v in pairs(p.saved)do emu.write(a,v,emu.memType.snesWorkRam)end end
    p=nil;completed=completed+1
    local status=assert(io.open(out..'/camera_progress.txt','wb'));status:write(kind..' completed='..completed..' controlled_required='..#cases);status:close()
end,emu.callbackType.exec,exit,exit,emu.cpuType.snes,emu.memType.snesMemory)end
local visual_frame=0
emu.addEventCallback(function()
    if controlled or not ready then return end
    visual_frame=visual_frame+1
    if visual_frame%300~=0 then return end
    local stem=out..'/native_'..visual_frame
    local shot=assert(io.open(stem..'.png','wb'));shot:write(emu.takeScreenshot());shot:close()
    for _,m in ipairs({{'vram',emu.memType.snesVideoRam,0x10000},{'cgram',emu.memType.snesCgRam,0x200}})do
        local bytes={};for a=0,m[3]-1 do bytes[#bytes+1]=string.char(emu.read(a,m[2]))end
        local file=assert(io.open(stem..'_'..m[1]..'.bin','wb'));file:write(table.concat(bytes));file:close()
    end
    local state=emu.getState();local file=assert(io.open(stem..'_state.txt','wb'))
    for k,v in pairs(state)do if k:find('ppu.',1,true)==1 then file:write(k..'='..tostring(v)..'\n')end end
    file:write('camera_x='..w(0x85c)..'\ncamera_y='..w(0x860)..'\nhome_team='..w(0x16fd)..
        '\ncoarse_x='..w(0x86c)..'\ncoarse_y='..w(0x86e)..'\ndestination='..w(0x874)..'\n');file:close()
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
