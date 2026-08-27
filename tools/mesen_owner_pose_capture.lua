-- Independent ROM oracle. Controlled cases change WRAM inputs at genuine
-- routine entries, never PC, flags, stack, ROM or rendered assets.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local controlled=os.getenv('NBA95_POSE_CONTROL')=='1'
local file=assert(io.open(out..'/owner_pose.vectors.jsonl','wb'))
local frames,count=0,0
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function snapshot(base)
    local parts={}
    for _,r in ipairs({{0,255},{0x7f6,0x7f7},{0x968,0x969},{0x9f6,0x9f7},{base,base+0xbf}})do
        local bytes={};for a=r[1],r[2]do bytes[#bytes+1]=string.format('%02x',emu.read(a,emu.memType.snesWorkRam))end
        parts[#parts+1]=string.format('"%04x":"%s"',r[1],table.concat(bytes))
    end
    local cpu={};for k,v in pairs(emu.getState())do if k:sub(1,4)=='cpu.' then cpu[#cpu+1]=string.format('"%s":%s',k:sub(5),tostring(v))end end
    return '{"cpu":{'..table.concat(cpu,',')..'},"mem":{'..table.concat(parts,',')..'}}'
end
local cases={animation={},pose={}}
local function anim(name,state,phase,target,acc,timer,rng,direction,delta,alternate,lowerphase)
    cases.animation[#cases.animation+1]={name=name,actor={
        [0x18]=65535,[0x1a]=65535,[0x30]=state,[0x32]=state,[0x38]=state,
        [0x3a]=phase,[0x3c]=lowerphase or 0,[0x42]=acc,[0x44]=timer,[0x46]=0,[0x48]=0,
        [0x4e]=direction,[0x52]=direction,[0x6c]=direction%2,[0xa8]=alternate or 0,[0xb0]=target},
        globals={[0x7f6]=rng,[0xc6]=delta or 2}}
end
for d=0,7 do for _,state in ipairs({13,18})do
    anim('direction-'..state..'-'..d,state,d%2,d%2,0x400,0x600,0x9146+d,d,2,d%2,state==13 and 1 or 0)
end end
for i,v in ipairs({{0,0x600},{0x3ff,0x600},{0x400,0x600},{0x401,0x600},{0xffff,0x600},{0,0},{0,0x3ff},{0,0x400}})do
    for _,state in ipairs({13,18})do anim('timer-'..state..'-'..i,state,0,3,v[1],v[2],0x9146,4)end
end
for phase=0,7 do
    anim('ascending-'..phase,18,phase,(phase+1)%8,0x400,0x600,0x9146,4)
    anim('descending-'..phase,18,phase,0x8000+((phase+7)%8),0x400,0x600,0x9146,4)
    for _,seed in ipairs({0x9146,0x8001,0x1234,0xabcd})do
        anim('target-'..phase..'-'..seed,18,phase,phase,0x600,0x600,seed,4)
    end
end
for _,target in ipairs({8,0xffff,0x8008,0x8000})do anim('target-reset-'..target,18,0,target,0x400,0x600,0x9146,4)end
for _,state in ipairs({13,18})do
    anim('delta-zero-'..state,state,0,1,0x600,0x600,0x8001,4,0)
    anim('delta-xba-'..state,state,0,1,0xff00,0x600,0x1234,4,0x201)
end
for _,controller in ipairs({0xffff,0,1})do
    for _,distance in ipairs({0,16,17,18,0x8010,0x8011,0xffff})do
        for _,ball in ipairs({0,2})do
            cases.pose[#cases.pose+1]={name='pose-'..controller..'-'..distance..'-'..ball,
                actor={[0x16]=controller,[0x8a]=distance,[0x38]=12,[0x50]=2,[0x4e]=ball==0 and 6 or 2},
                globals={[0x9f6]=ball,[0x968]=distance%2}}
        end
    end
end
local specs={{name='animation',entry=0x87ab38,exits={0x87ac98,0x87ad5a}},
             {name='pose',entry=0x86e4f5,exits={0x86e518,0x86e51f,0x86e534,0x86e544}}}
local forced_dead
-- Exercise the latched fork through its actual caller's WRAM gate. The
-- captured entry below remains the real E4F5 execution, not a PC jump.
emu.addMemoryCallback(function()
    if controlled and (specs[2].calls or 0)<#cases.pose then
        forced_dead=word(0x968);put(0x968,1)
    end
end,emu.callbackType.exec,0x86e4ed,0x86e4ed,emu.cpuType.snes,emu.memType.snesMemory)
for _,spec in ipairs(specs)do
    spec.calls=0
    emu.addMemoryCallback(function()
        local base=word(0x96)
        if base<0x34eb or base>0x3deb then return end
        spec.calls=spec.calls+1
        if spec.name=='animation' and spec.calls>6000 and word(base+0x30)~=13 and word(base+0x30)~=18 then return end
        local test=controlled and cases[spec.name][spec.calls] or nil
        local saved={}
        if test then
            for offset in pairs(test.actor)do saved[base+offset]=word(base+offset)end
            -- Restore all owned outputs as well, keeping controlled cases
            -- separate from the subsequent natural gameplay trajectory.
            for a=base,base+0xbe,2 do saved[a]=word(a)end
            saved[0x7f6]=word(0x7f6)
            for a in pairs(test.globals)do saved[a]=word(a)end
            for offset,v in pairs(test.actor)do put(base+offset,v)end
            for a,v in pairs(test.globals)do put(a,v)end
        end
        spec.pending={base=base,before=snapshot(base),saved=saved,test=test,frame=frames,executed={}}
    end,emu.callbackType.exec,spec.entry,spec.entry,emu.cpuType.snes,emu.memType.snesMemory)
    for _,pc in ipairs(spec.exits)do
        emu.addMemoryCallback(function()
            local p=spec.pending;if not p then return end
            count=count+1
            -- Keep all controlled cases and bounded natural witnesses.
            if p.test or spec.calls<=6000 or (spec.name=='animation' and (word(p.base+0x30)==13 or word(p.base+0x30)==18)) then
                if spec.name=='pose' then p.executed[spec.entry]=true;p.executed[pc]=true end
                local executed={};for address in pairs(p.executed)do executed[#executed+1]=string.format('"%06x"',address)end;table.sort(executed)
                file:write(string.format('{"call":%d,"kind":"%s","provenance":"%s","entry_frame":%d,"exit_frame":%d,"entry_pc":"%06x","exit_pc":"%06x","entry":%s,"exit":%s,"executed":[%s]}\n',
                    count,spec.name,p.test and ('controlled-ROM:'..p.test.name) or 'natural-ROM',p.frame,frames,spec.entry,pc,p.before,snapshot(p.base),table.concat(executed,',')));file:flush()
            end
            for a,v in pairs(p.saved)do put(a,v)end
            if spec.name=='pose' and forced_dead~=nil then put(0x968,forced_dead);forced_dead=nil end
            spec.pending=nil
        end,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
    end
end
emu.addMemoryCallback(function(address)
    if specs[1].pending then specs[1].pending.executed[address]=true end
end,emu.callbackType.exec,0x87adbe,0x87aebc,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function(address)
    if specs[2].pending then specs[2].pending.executed[address]=true end
end,emu.callbackType.exec,0x86e4f5,0x86e544,emu.cpuType.snes,emu.memType.snesMemory)
emu.addEventCallback(function()
    frames=frames+1
    if frames%600==0 then
        local f=assert(io.open(out..'/pose_progress.txt','wb'))
        for _,s in ipairs(specs)do f:write(s.name..'='..s.calls..' required='..#cases[s.name]..'\n')end
        f:close()
    end
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
