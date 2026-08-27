-- Controlled ROM branch tests, NOT naturally occurring gameplay captures.
-- Runs original ROM instructions with explicit WRAM inputs; no ROM patches.
-- Entry/exit snapshots are consumed by verify_shot_branch_vectors.py.
local out = assert(os.getenv('NBA95_CAPTURE_DIR'))
local file = assert(io.open(out .. '/shot_branch_cases.vectors.jsonl','wb'))
local cases = {}
local function add(name, pc, exits, values)
    cases[#cases+1]={name=name,pc=pc,exits=exits,values=values or {}}
end
local vectors={{0,-100},{100,-100},{100,0},{100,100},{0,100},
               {-100,100},{-100,0},{-100,-100},{0,0}}
for i,v in ipairs(vectors) do
    add('sidestep_direction_'..(i-1),0xB7F7,{[0xB84C]=true},
        {[0x34EF]=100,[0x34F3]=-v[2],[0x46F5]=100+v[1]})
end
for i,v in ipairs(vectors) do
    add('release_direction_'..(i-1),0x9D7A,{[0x9D9B]=true},
        {[0x34EF]=100,[0x34F3]=-v[2],[0x46F5]=100+v[1]})
end
for _,v in ipairs({{'moving',0x3537,1},{'distance_below',0x3577,119},
    {'distance_equal',0x3577,120},{'distance_wrap',0x3577,0x8078},
    {'free_throw',0x978,1},{'x_below',0x34EF,55},{'x_equal',0x34EF,56},
    {'x_negative',0x34EF,-56},{'x_min',0x34EF,0x8000},{'rng_even',0x7F6,2}}) do
    add('sidestep_'..v[1],0xB7F7,{[0xB84C]=true},{[v[2]]=v[3]})
end
add('lost_same_team',0xB867,{[0xB86B]=true})
add('lost_other_team',0xB867,{[0xB86B]=true},{[0x93A]=5})
for _,value in ipairs({0,1,2,0xffff}) do
    add('cancel_attachment_'..value,0xB890,{[0xB8C8]=true},{[0x9F6]=value})
end
for _,v in ipairs({{'cpu',0xffff,0,0},{'human_hold',0,0,0x80},
    {'human_release',0,0,0},{'free_throw',0,1,0}}) do
    add('button_'..v[1],0xB86C,{[0xB8C9]=true,[0xB88F]=true},
        {[0x3501]=v[2],[0x978]=v[3],[0x4703]=v[4]})
end
for _,v in ipairs({{'owner',0,4,0x600,0},{'lost_latched',1,4,0x600,0x80},
    {'phase_wait',0,3,0x600,0x80},{'accumulator_wait',0,4,0x5ff,0x80},
    {'cancel',0,4,0x600,0x80}}) do
    add('owner_gate_'..v[1],0xB769,
        {[0xB867]=true,[0xB791]=true,[0xB790]=true,[0xB890]=true},
        {[0x93E]=v[2],[0x3525]=v[3],[0x352D]=v[4],[0x3569]=v[5]})
end
local ranges={{0,0xff},{0x7f6,0x7f7},{0x900,0x9ff},{0x34eb,0x3ffe},{0x466b,0x48ff}}
local function writeword(a,v)
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
    local cpu={}
    for k,v in pairs(emu.getState()) do
        if k:sub(1,4)=='cpu.' then
            cpu[#cpu+1]=string.format('"%s":%s',k:sub(5),tostring(v))
        end
    end
    return '{"cpu":{'..table.concat(cpu,',')..'},"mem":{'..table.concat(chunks,',')..'}}'
end
local current, entry, completed, saved = nil,nil,0,nil
local entered, captured=false,false
local function word(a)
    return emu.read(a,emu.memType.snesWorkRam) | (emu.read(a+1,emu.memType.snesWorkRam)<<8)
end
local function prepare_inputs()
    local base,context,buttons=word(0x96),word(0x9e),word(0x9a)
    local function translated(a)
        if a>=0x34eb and a<0x35eb then return base+a-0x34eb end
        if a==0x46f5 then return context+10 end
        if a==0x4703 then return buttons+8 end
        return a
    end
    local defaults={
        [0x34ef]=100,[0x34f3]=30,[0x34f9]=123,[0x34fb]=-77,[0x34fd]=256,
        [0x3501]=0xffff,[0x3513]=0x1234,[0x351b]=0x16,[0x351d]=0x32,
        [0x3525]=4,[0x352d]=0x600,[0x3531]=0x16,[0x3533]=0x32,
        [0x3549]=12,[0x354b]=9,[0x354f]=7,[0x3569]=0x84,[0x3577]=119,
        [0x7f6]=1,[0x948]=0xffff,[0x936]=2,[0x9f6]=1,[0x968]=7,
        [0x3ef5]=0xabcd,[0x3ef7]=83,[0x3efd]=-18,[0x46f5]=336,
        [0x93a]=0,[0x3559]=0,[0x3537]=0
    }
    for a,v in pairs(defaults) do writeword(translated(a),v) end
    writeword(0x93e,word(0xc2))
    for a,v in pairs(current.values) do
        if a==0x93e then v=(word(0xc2)+v)%10 end
        writeword(translated(a),v)
    end
end
local returns={[0xB866]=true,[0xB86B]=true,[0xB8C8]=true,[0xB8C9]=true,
               [0xB88F]=true,[0xB790]=true,[0xB978]=true}
local function on_instruction(address)
    local pc=address&0xffff
    if pc==0xB769 and not current then
        current=cases[completed+1];entered=false;captured=false;saved={}
        for _,r in ipairs(ranges) do
            for a=r[1],r[2] do saved[a]=emu.read(a,emu.memType.snesWorkRam) end
        end
        local base=word(0x96)
        -- Route this genuine mode-12 invocation to the target branch by
        -- changing inputs only. Never change PC, stack, ROM, or CPU flags.
        writeword(0x93e,word(0xc2));writeword(base+0x7e,0)
        writeword(0x948,current.pc==0xB7F7 and 29 or 1)
        if current.pc==0x9D7A then
            writeword(0x948,0xffff);writeword(base+0x12,0xffff)
        end
        if current.pc==0xB867 then writeword(0x93e,(word(0xc2)+1)%10) end
        if current.pc==0xB890 then
            writeword(base+0x7e,0x80);writeword(base+0x3a,4);writeword(base+0x42,0x600)
        end
    end
    if not current then return end
    if not entered and pc==current.pc then
        prepare_inputs();entry=snapshot();entered=true;return
    end
    if entered and not captured and current.exits[pc] then
        file:write(string.format('{"call":%d,"provenance":"controlled-ROM:%s","entry_pc":"86%04x","exit_pc":"86%04x","entry":%s,"exit":%s}\n',
            completed+1,current.name,current.pc,pc,entry,snapshot()))
        file:flush();captured=true
    end
    if captured and returns[pc] then
        for a,v in pairs(saved) do emu.write(a,v,emu.memType.snesWorkRam) end
        completed=completed+1;current=nil
        if completed==#cases then
            file:close()
            local done=assert(io.open(out..'/capture_complete.txt','wb'))
            done:write('controlled-ROM cases='..completed..'\n');done:close();emu.stop(0)
        end
    end
end
emu.addMemoryCallback(on_instruction,emu.callbackType.exec,0x86B769,0x86B978,
    emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(on_instruction,emu.callbackType.exec,0x869D7A,0x869D9B,
    emu.cpuType.snes,emu.memType.snesMemory)
-- Reuse only the existing menu driver. Configure its vector entry to an
-- unused sentinel PC; this script owns all controlled-case snapshots.
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
