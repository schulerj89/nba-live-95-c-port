-- ROM-only oracle. Inputs are changed at actual native entries and restored
-- at exit. No PC/flags/stack/ROM patches. Callee snapshots verify caller
-- ordering; they are not evidence of a translated callee's correctness.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local kind=os.getenv('NBA95_OWNER_KIND') or 'flow'
local controlled=os.getenv('NBA95_OWNER_CONTROL')=='1'
local file=assert(io.open(out..'/owner_'..kind..'.jsonl','wb'))
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function array(t)local s={};for _,v in ipairs(t)do s[#s+1]=tostring(v)end;return '['..table.concat(s,',')..']'end
local offsets={0x18,0x1a,0x30,0x32,0x38,0x3a,0x3c,0x42,0x44,0x46,0x48,0x1c,0x1e,0x20,0x22,0x24,0x26,0xb0}
local function channels(base)local t={};for _,o in ipairs(offsets)do t[#t+1]=word(base+o)end;return t end
local function actorbytes(base)local t={};for a=base,base+0xbf do t[#t+1]=emu.read(a,emu.memType.snesWorkRam)end;return t end
local base, pending, calls, complete, frames=0,nil,0,0,0
local function flow()
    local ptr=word(0xe0)|(emu.read(0xe2,emu.memType.snesWorkRam)<<16)
    return {word(0xc2),word(0x93e),word(0x9f8),word(0x9bc),word(0xa02),word(base+0x6e),word(0x9f4),
        word(0x996),word(0x954),word(0x9b8),word(0x994),word(0x968),word(0x9f6),
        word(base+0xe),word(base+0x10),word(base+0x4e),word(base+0x74),word(0x936),word(0x93a),
        word(base+0x60),word(0xc8),word(base+0x5e),word(base+0x64),word(base+0x7e),
        word(base+0x16),word(base+0x7a),emu.read((ptr+0x3f)&0xffffff,emu.memType.snesMemory)}
end
local function readstate()
    if kind=='flow' then return flow()end
    local t=channels(base)
    if kind=='unlatched' then
        for _,o in ipairs({0xe,0x10,0x50,0x4e,0x72,0xa8,0x2a,0x2c,0x52})do t[#t+1]=word(base+o)end
    else
        for _,v in ipairs({word(base+0x52),word(base+0x4a),word(0xc6),word(base+0xa8),word(base+0x6c),word(0x7f6),word(base+0x2a),word(base+0x2c)})do t[#t+1]=v end
    end
    return t
end
local cases={}
local function add(name,actor,globals)cases[#cases+1]={name=name,actor=actor or {},globals=globals or {}}end
if kind=='unlatched' then
    for _,state in ipairs({3,9,11})do for d=0,7 do for _,phase in ipairs({0,1,4,0xffff})do
        add('state'..state..'-dir'..d..'-phase'..phase,{[0x30]=state,[0x32]=state,[0x38]=3,[0x3a]=1,[0x3c]=phase,
            [0x18]=0xffff,[0x1a]=0xffff,[0x46]=0,[0x48]=0,[0xe]=0x100,[0x10]=0,[0x50]=d,[0x4e]=7,[0x72]=0,[0xa8]=d%2})
    end end end
    for _,lock in ipairs({1,0xffff})do for _,state in ipairs({9,11})do
        -- A rejected install leaves descriptor bank DP49 untouched. Supply
        -- the animation asset bank explicitly when constructing lock cases;
        -- arbitrary scratch-bank contents are not a valid pack descriptor.
        add('lock'..lock..'-state'..state..'-bank84',{[0x30]=state,[0x32]=state,[0x38]=3,[0x46]=lock,[0x48]=lock,
            [0xe]=0x100,[0x10]=0,[0x50]=state==9 and 2 or 6,[0x72]=0,[0xa8]=0},{[0x49]=0x84})
    end end
elseif kind=='idle' then
    for _,acc in ipairs({0,0x3ff,0x400,0xffff})do for _,timer in ipairs({0,0x600,0xffff})do for _,seed in ipairs({0,0x9146,0x8001,0x1234})do
        add('idle-'..acc..'-'..timer..'-'..seed,{[0x30]=7,[0x32]=7,[0x38]=7,[0x3a]=0,[0x3c]=0,
            [0x18]=0xffff,[0x1a]=0xffff,[0x46]=0,[0x48]=0,[0x42]=acc,[0x44]=timer,[0x52]=4,[0x6c]=0,[0xa8]=0},
            {[0x7f6]=seed,[0xc6]=2})
    end end end
else
    -- Base case suppresses unrelated child decisions; individual cases turn
    -- each gate on. CPU callbacks use an out-of-court X to take B678's real
    -- early return, then exercise F42C/F431/F435 normally.
    for _,v in ipairs({0,1,2,3,0x7fff,0x8000,0xffff})do add('timer-'..v,{[0x60]=v})end
    for _,v in ipairs({0,1,5,6,7,0x8005,0x8006,0xffff})do add('play-'..v,{}, {[0x996]=v})end
    add('negative-owner',{}, {[0x93e]=0xffff});add('lost-owner',{}, {[0x93e]=9})
    add('shooting-deferred',{}, {[0x9bc]=1,[0xa02]=1});add('shooting-only',{}, {[0x9bc]=1})
    add('transfer',{}, {[0x996]=1,[0x9b8]=1});add('inbound-owner',{}, {[0x996]=1,[0x954]=0})
    add('inbound-continuation',{}, {[0x936]=0x82})
    for _,attached in ipairs({0,1,2,3,0xffff})do for _,vx in ipairs({0,0x100,0xff00})do
        add('stop-'..attached..'-'..vx,{[0xe]=vx,[0x10]=0},{[0x968]=1,[0x9f6]=attached})
    end end
    for _,pair in ipairs({0,2,8,10,18})do add('pair-'..pair,{[0x74]=pair})end
    for _,recover in ipairs({0,1})do add('cpu-'..recover,{[0x16]=0xffff,[0x7a]=recover,[0x60]=0,[0x4]=500})end
end
local entry=kind=='flow' and 0x86f34f or kind=='idle' and 0x87ab38 or 0x86e545
local exits=kind=='flow' and {0x86f439,0x86f40a,0x86f43a} or kind=='idle' and {0x87ac98,0x87ad5a} or {0x86e578,0x86e58a,0x86e592}
local forced_gate
if kind=='unlatched' then emu.addMemoryCallback(function()
    if controlled and calls<#cases then
        local actor=word(0x96)
        local paired=word(0x9a);local context=word(0x9e)
        forced_gate={}
        local changes={[actor+0xc]=0,[actor+0x4]=word(context+0xa),[actor+0x4c]=0,
            [actor+0x8a]=0,[paired+0x4c]=0,[0x978]=0,[0x936]=2,[0x968]=0,[actor+0xae]=0}
        for a,v in pairs(changes)do forced_gate[a]=word(a);put(a,v)end
    end
end,emu.callbackType.exec,0x86e4a7,0x86e4a7,emu.cpuType.snes,emu.memType.snesMemory)end
emu.addMemoryCallback(function()
    base=word(0x96);if base<0x34eb or base>0x3deb then return end
    calls=calls+1
    if calls>2000 then pending=nil;return end
    local test=controlled and cases[calls] or nil
    local saved={}
    if test then
        for _,r in ipairs({{0,255},{0x700,0xaff},{0x34eb,0x3eea}})do
            for a=r[1],r[2]do saved[a]=emu.read(a,emu.memType.snesWorkRam)end
        end
        if kind=='flow' then
            for o,v in pairs({[0x16]=0,[0x60]=100,[0x74]=0xffff,[0x7a]=0,[0x5e]=11})do put(base+o,v)end
            for a,v in pairs({[0x93e]=word(0xc2),[0x9f8]=7,[0x9bc]=0,[0xa02]=0,[0x996]=6,[0x954]=0xffff,
                [0x9b8]=0,[0x994]=0,[0x968]=0,[0x9f6]=0,[0x936]=2,[0xc8]=2})do put(a,v)end
        end
        for o,v in pairs(test.actor)do put(base+o,v)end
        for a,v in pairs(test.globals)do put(a,v)end
    end
    pending={before=readstate(),actor_before=actorbytes(base),bank=word(0x49)&255,
        saved=saved,test=test,executed={[entry]=true},children={},frame=frames}
end,emu.callbackType.exec,entry,entry,emu.cpuType.snes,emu.memType.snesMemory)
local function boundary(pc)
    if not pending then return end
    local p=pending
    if p.child then
        if pc~=p.child.ret then return end
        p.child.after=readstate();p.children[#p.children+1]=p.child;p.child=nil
    end
    local id,ret
    if pc==0x86f3c7 then id=0;ret=0x86f3cb
    elseif pc==0x86f428 then id=1;ret=0x86f42c
    elseif pc==0x86f431 then id=2;ret=0x86f435
    elseif pc==0x86f435 then id=3;ret=0x86f439 end
    if id then p.child={id=id,ret=ret,before=readstate()}end
end
if kind=='flow' then for _,pc in ipairs({0x86f3c7,0x86f3cb,0x86f428,0x86f42c,0x86f431,0x86f435,0x86f439})do
    emu.addMemoryCallback(function()boundary(pc)end,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end end
for _,pc in ipairs(exits)do emu.addMemoryCallback(function()
    local p=pending;if not p then return end
    -- A CPU nonlocal return has not reached this caller exit. Do not pass it
    -- off as a complete caller witness; independent action tests own it.
    if p.child then return end
    p.executed[pc]=true
    local executed={};for a in pairs(p.executed)do executed[#executed+1]=a end;table.sort(executed)
    local children={};for _,c in ipairs(p.children)do children[#children+1]='{"id":'..c.id..',"input":'..array(c.before)..',"output":'..array(c.after)..'}'end
    file:write('{"call":'..calls..',"kind":"'..kind..'","provenance":"'..(p.test and 'controlled-ROM:'..p.test.name or 'natural-ROM')..
        '","frame":'..p.frame..',"exit_pc":'..pc..',"input":'..array(p.before)..',"expected":'..array(readstate())..
        ',"descriptor_bank":'..p.bank..',"actor_before":'..array(p.actor_before)..',"actor_after":'..array(actorbytes(base))..
        ',"children":['..table.concat(children,',')..'],"executed":'..array(executed)..'}\n');file:flush()
    for a,v in pairs(p.saved)do emu.write(a,v,emu.memType.snesWorkRam)end
    if forced_gate then for a,v in pairs(forced_gate)do put(a,v)end;forced_gate=nil end
    pending=nil;complete=complete+1
end,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local lo,hi=entry,kind=='flow' and 0x86f439 or kind=='idle' and 0x87adbd or 0x86e592
if kind=='idle' then lo=0x87ad86 end
emu.addMemoryCallback(function(a)if pending then pending.executed[a]=true end end,emu.callbackType.exec,lo,hi,emu.cpuType.snes,emu.memType.snesMemory)
emu.addEventCallback(function()
    frames=frames+1
    if frames%600==0 then local f=assert(io.open(out..'/owner_progress.txt','wb'));f:write('kind='..kind..' entries='..calls..' completed='..complete..' controlled_required='..#cases);f:close()end
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
