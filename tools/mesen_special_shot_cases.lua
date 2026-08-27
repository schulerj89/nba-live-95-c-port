-- Controlled WRAM-input cases on genuine ROM selector/mode-17 invocations.
-- Never changes ROM, PC, CPU flags, or stack. Not a natural gameplay trace.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local file=assert(io.open(out..'/special_shot_cases.vectors.jsonl','wb'))
local cases={}
local function add(name,kind,values) cases[#cases+1]={name=name,kind=kind,values=values or {}} end
-- A:actor word; G:WRAM word. Selector starts after the separately verified
-- F5E4 call so the test owns an explicit lane predicate result.
for appearance=0,1 do for relative=0,7 do
    add('select_variant'..appearance..'_relative'..relative,0,
        {A6c=appearance,A88=relative*2})
end end
for _,v in ipairs({{'lane_clear','Gaa',0},{'moving','A4c',1},
    {'distance_95','A8c',95},{'distance_96','A8c',96},
    {'mask_2','A6c',2},{'mask_ffff','A6c',0xffff},
    {'negative_upper_lock','A46',0xffff},{'negative_lower_lock','A48',0xffff},
    {'boosted','A72',1},{'alternate_lower','Aa8',1},
    {'positive_facing','A4e',5}}) do add('select_'..v[1],0,{[v[2]]=v[3]}) end
add('step_cpu_hold',1)
add('step_jump_threshold',1,{G948=3})
add('step_jump_before_button',1,{G948=3,A16=0,buttons=0})
add('step_activity_wrap',1,{G948=0x7fff})
add('step_owner_lost_same',1,{lost=1})
add('step_owner_lost_other',1,{lost=1,G93a=5})
add('step_human_hold',1,{A16=0,buttons=0x80})
add('step_human_cancel',1,{A16=0,buttons=0})
add('step_cancel_zero_attachment',1,{A16=0,buttons=0,G9f6=0})
add('step_cancel_alternate_lower',1,{A16=0,buttons=0,Aa8=1})
add('step_grounded_cancel',1,{G948=0xffff,A0c=0})
add('step_airborne_hold',1,{G948=0xffff,A60=0,A3a=2})
add('step_release_before_turn_timer',1,{G948=0xffff,A60=0,A3a=3})
for facing=0,8 do
    add('step_release_facing_'..facing,1,{G948=0xffff,A60=8,A3a=3,A4e=facing,A66=0})
end
for _,flags in ipairs({0,1,2,3,0x8000,0x8001,0x8002,0x8003}) do
    add('step_attachment_flags_'..flags,1,{A28=flags})
end
add('step_timer_wrap',1,{G948=0xffff,A60=0xffff,A3a=3})

local ranges={{0,0xff},{0x7f6,0xa0f},{0x1400,0x18ff},{0x3400,0x49ff}}
local function word(a) return emu.read(a,emu.memType.snesWorkRam) |
    (emu.read(a+1,emu.memType.snesWorkRam)<<8) end
local function put(a,v)
    emu.write(a,v&255,emu.memType.snesWorkRam)
    emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)
end
local function snapshot()
    local chunks={}
    for _,r in ipairs(ranges) do
        local bytes={}
        for a=r[1],r[2] do bytes[#bytes+1]=string.format('%02x',emu.read(a,emu.memType.snesWorkRam)) end
        chunks[#chunks+1]=string.format('"%04x":"%s"',r[1],table.concat(bytes))
    end
    return '{"mem":{'..table.concat(chunks,',')..'}}'
end
local current,saved,entry=nil,nil,nil
local completed,captured=0,false
local frames,last_pc=0,0
emu.addEventCallback(function()
    frames=frames+1
    if frames%600==0 then
        local log=assert(io.open(out..'/progress.txt','wb'))
        log:write(string.format('frames=%d completed=%d case=%s last_pc=%04x entry=%s captured=%s\n',
            frames,completed,current and current.name or 'none',last_pc,tostring(entry~=nil),tostring(captured)))
        log:close()
    end
end,emu.eventType.endFrame)
local returns={[0xB744]=true,[0xB6D2]=true,[0xB9F9]=true,[0xB9FE]=true,
               [0xBA52]=true,[0xBA53]=true,[0xBAA1]=true}
local function apply(values)
    local base=word(0x96)
    for k,v in pairs(values) do
        if k=='lost' then put(0x93e,(word(0xc2)+1)%10)
        elseif k=='buttons' then put(word(0x90c)+8,v)
        elseif k:sub(1,1)=='A' then put(base+tonumber(k:sub(2),16),v)
        else put(tonumber(k:sub(2),16),v) end
    end
end
local function defaults(kind)
    apply({Gaa=1,A4c=0,A8c=95,A88=4,A4e=0,A6c=0,A66=7,A72=0,Aa8=0,
           A0e=123,A10=-77,A12=0,A4a=99,A60=0,A7e=4,A28=0,A64=7,
           A16=0xffff,A6e=0,G93a=0,G948=1,G920=9,G91c=7})
    if kind==1 then
        apply({A04=100,A08=-30,A0c=16,A3a=2,A5e=17,A66=6,
               G3eef=50,G3ef3=20,G3ef7=83,G922=7,G936=2,G9f6=1,G968=7,G3efd=-18})
        put(0x93e,word(0xc2))
    end
end
local function callback(address)
    local pc=address&0xffff
    last_pc=pc
    if pc==0xB625 and not current then
        current=cases[completed+1];saved={};entry=nil;captured=false
        for _,r in ipairs(ranges) do for a=r[1],r[2] do saved[a]=emu.read(a,emu.memType.snesWorkRam) end end
    end
    if not current then return end
    if pc==0xB629 and not entry then
        defaults(0)
        if current.kind==0 then apply(current.values);entry=snapshot() end
        -- For mode 17, allow the real selector to install the action and
        -- let the game's own next actor dispatch reach B979.
        return
    end
    if pc==0xB979 and current.kind==1 and not entry then
        defaults(1);apply(current.values);entry=snapshot();return
    end
    local terminal=current.kind==0 and (pc==0xB744 or pc==0xB6D2) or
        current.kind==1 and (pc==0xB9F9 or pc==0xB9FE or pc==0xBA52 or
                            pc==0xBA53 or pc==0xBAA1 or pc==0x9DA6)
    if entry and not captured and terminal then
        file:write(string.format('{"call":%d,"provenance":"controlled-ROM:%s","entry_pc":"86%04x","exit_pc":"86%04x","entry":%s,"exit":%s}\n',
            completed+1,current.name,current.kind==0 and 0xB629 or 0xB979,pc,entry,snapshot()))
        file:flush();captured=true
    end
    if captured and returns[pc] then
        for a,v in pairs(saved) do emu.write(a,v,emu.memType.snesWorkRam) end
        completed=completed+1;current=nil
        if completed==#cases then
            file:close()
            local done=assert(io.open(out..'/capture_complete.txt','wb'))
            done:write('controlled-ROM special-shot cases='..completed..'\n');done:close();emu.stop(0)
        else
            -- Controlled game inputs make another real CPU shot decision
            -- eligible promptly. This is not a CPU-PC shortcut.
            put(0x92c,0)
        end
    end
end
emu.addMemoryCallback(callback,emu.callbackType.exec,0x86B625,0x86BAA1,
    emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(callback,emu.callbackType.exec,0x869DA6,0x869DA6,
    emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
