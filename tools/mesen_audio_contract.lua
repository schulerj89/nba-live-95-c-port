-- Original ROM execution: normal title/menu/team/controller navigation.
-- Controlled mode changes only declared audio inputs at82FD65 after100court
-- frames. Every before/after write is retained. No CPU/ROM/PPU/SRAM edits.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local mode=assert(os.getenv('NBA95_AUDIO_CONTRACT'))
assert(mode=='natural' or mode=='controlled')
local max_court=mode=='natural' and 2200 or 400
local function json(v)
    if type(v)=='string' then return '"'..v:gsub('\\','\\\\'):gsub('"','\\"')..'"' end
    if type(v)~='table' then return tostring(v) end
    local parts={}
    if #v>0 then for _,x in ipairs(v)do parts[#parts+1]=json(x)end
        return '['..table.concat(parts,',')..']' end
    local keys={};for k in pairs(v)do keys[#keys+1]=k end;table.sort(keys)
    for _,k in ipairs(keys)do parts[#parts+1]=json(k)..':'..json(v[k])end
    return '{'..table.concat(parts,',')..'}'
end
local events=assert(io.open(out..'/events.jsonl','wb'));events:setvbuf('no')
local actions_log=assert(io.open(out..'/actions.jsonl','wb'));actions_log:setvbuf('no')
local writes=assert(io.open(out..'/writes.jsonl','wb'));writes:setvbuf('no')
local frames=assert(io.open(out..'/frames.jsonl','wb'));frames:setvbuf('no')
local audio_writes=assert(io.open(out..'/audio-writes.jsonl','wb'));audio_writes:setvbuf('no')
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder());home:close()
local frame,title,setup,player,court=0,-1,-1,-1,-1
local dispatch,active_dispatch,controlled_case=0,0,0
local injecting=false
local action_index,action_tick=1,-1
local function word(a)
    return emu.read(a,emu.memType.snesWorkRam,false)|
        (emu.read(a+1,emu.memType.snesWorkRam,false)<<8)
end
local function words(a,n)
    local values={};for i=0,n-1 do values[#values+1]=word(a+i*2)end;return values
end
local function bytes(a,n,mem)
    local values={};for i=0,n-1 do values[#values+1]=emu.read(a+i,mem,false)end;return values
end
local function snapshot(kind,pc)
    local s=emu.getState(); local sp=s['cpu.sp'] or 0
    return {kind=kind,pc=pc,frame=frame,court=court,dispatch=active_dispatch,
        controlled_case=controlled_case,action=action_index,
        cycle=s['cpu.cycleCount'],scanline=s['ppu.scanline'],native_frame=s['ppu.frameCount'],
        cpu={a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],d=s['cpu.d'],dbr=s['cpu.dbr'],
            ps=s['cpu.ps'],sp=sp,k=s['cpu.k'],pc=s['cpu.pc']},stack=bytes(sp+1,10,emu.memType.snesWorkRam),
        main=words(0x17ab,4),options=words(0x17b5,7),working=words(0x16fb,7),row=word(0x1693),
        selections=words(0x166d,5),ownership=words(0x08d4,5),
        raw={rng07f6=word(0x07f6),event13e7=word(0x13e7),crowd13e9=word(0x13e9),bounce13e5=word(0x13e5),
            ticks0564=word(0x0564),clock0928=word(0x0928),elapsed0938=word(0x0938),
            loop0854=word(0x0854),live0936=word(0x0936),owner093e=word(0x093e),
            active0946=word(0x0946),slow094e=word(0x094e),slow0950=word(0x0950),
            music15c5=word(0x15c5),sfx15c7=word(0x15c7)},
        driver=bytes(0x0620,0x90,emu.memType.snesWorkRam)}
end
local function event(kind,pc) events:write(json(snapshot(kind,pc))..'\n')end
local function fail(message)
    local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
    emu.stop(1);error(message)
end
local function require_case(ok,message)if not ok then fail(message)end end
local function hook(pc,fn)
    emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
local function dump(name,mem,n)
    local chunks={};for base=0,n-1,1024 do
        local b={};for i=base,math.min(base+1023,n-1)do b[#b+1]=string.char(emu.read(i,mem,false))end
        chunks[#chunks+1]=table.concat(b)
    end
    local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(chunks));f:close()
end
local actions={}
local function add(key,label,wait,hold)
    actions[#actions+1]={key=key,label=label,wait=wait or 18,hold=hold or 3}
end
for i=1,5 do add('down','main_to_options')end
add('a','open_options',300)
if mode=='natural' then
    for row=0,1 do
        for i=1,31 do add('left','gain'..row..'_down_'..i)end
        for i=1,46 do add('right','gain'..row..'_up_'..i)end
        for i=1,15 do add('left','gain'..row..'_return30_'..i)end
        add('down','next_option')
    end
    add('right','music_off',120);add('right','music_mono',120);add('right','music_stereo',120)
    add('down','crowd_row');add('right','crowd_off',120);add('left','crowd_on',120)
end
add('start','commit_options',300)
add('start','confirm_exhibition',300)
local f=assert(io.open(out..'/actions.json','wb'));f:write(json(actions));f:close()
local cases={}
local function test(name,mask,crowd,bounce,on,rng)
    cases[#cases+1]={name=name,event13e7=mask,crowd13e9=crowd,bounce13e5=bounce,crowd17bb=on,rng07f6=rng}
end
-- Repeated adjacent invocations have identical event bits, intentionally no
-- quiet reset dispatch. RNG is preserved except explicitly named seed cases.
for _,bit in ipairs({1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192})do
    test('repeat_'..bit..'_a',bit,0,0x01f0,1)
    test('repeat_'..bit..'_b',bit,0,0x01f0,1)
end
for _,crowd in ipairs({1,2,4,8,15})do
    test('crowd_on_'..crowd,0,crowd,0,1)
    test('crowd_off_'..crowd,0,crowd,0,0)
end
for _,bounce in ipairs({0,15,16,255,256,0x7ff,0x800,0xffff})do test('bounce_volume_'..bounce,1,0,bounce,1)end
test('all_families_seed0',0x3fff,15,0xffff,1,0)
test('unhandled_bits_preserved',0xc000,0xfff0,0,1,0x8000)
test('all_families_crowd_off',0xffff,0xffff,0x01f0,0,1)
local cf=assert(io.open(out..'/controlled-cases.json','wb'));cf:write(json(cases));cf:close()
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x81ba8e,function()
    if title>=850 and setup<0 then setup=0;event('main_builder',0x81ba8e)end
end)
hook(0x81a489,function()if player<0 then player=0;event('player_setup',0x81a489)end end)
hook(0x87a47a,function()
    if court<0 then
        court=0;event('first_court',0x87a47a)
        require_case(word(0x166d)==1,'normal Player Setup did not select center CPU mode')
        require_case(word(0x17b5)==30 and word(0x17b7)==30 and word(0x17b9)==2 and word(0x17bb)==1,'natural Options did not commit intended audio values')
        dump('first_court.wram',emu.memType.snesWorkRam,0x20000)
        dump('first_court.spc',emu.memType.spcRam,0x10000)
        dump('first_court.dsp',emu.memType.spcDspRegisters,0x80)
    end
end)
hook(0x82fd65,function()
    require_case(active_dispatch==0,'overlapping audio dispatch calls')
    dispatch=dispatch+1;active_dispatch=dispatch;event('dispatch.entry_before_control',0x82fd65)
    if mode=='controlled' and court>=100 and controlled_case<#cases then
        controlled_case=controlled_case+1
        local c=cases[controlled_case]
        local changes={{0x13e7,c.event13e7},{0x13e9,c.crowd13e9},{0x13e5,c.bounce13e5},{0x17bb,c.crowd17bb}}
        if c.rng07f6~=nil then changes[#changes+1]={0x07f6,c.rng07f6}end
        for _,change in ipairs(changes)do
            writes:write(json({kind='controlled_word',frame=frame,court=court,dispatch=active_dispatch,case=controlled_case,name=c.name,address=change[1],before=word(change[1]),after=change[2]})..'\n')
            injecting=true
            emu.write(change[1],change[2]&255,emu.memType.snesWorkRam)
            emu.write(change[1]+1,(change[2]>>8)&255,emu.memType.snesWorkRam)
            injecting=false
        end
    end
    event('dispatch.entry',0x82fd65)
end)
hook(0x82ff84,function()event('dispatch.exit',0x82ff84);active_dispatch=0 end)
for _,item in ipairs({{0x808930,'rng.wrapper.entry'},{0x808934,'rng.wrapper.exit'},
    {0x80cee7,'rng.shared.entry'},{0x80cef5,'rng.shared.exit'},{0x80cefc,'rng.shared.zero_exit'},
    {0x809df3,'command.entry'},{0x809f0f,'crowd_queue.entry'},
    {0x82fd92,'command.return'},{0x82fdc7,'command.return'},
    {0x82fde5,'command.return'},{0x82fe03,'command.return'},
    {0x82fe21,'command.return'},{0x82fe3f,'command.return'},
    {0x82fe5d,'command.return'},{0x82fe7b,'command.return'},
    {0x82fe99,'command.return'},{0x82feb7,'command.return'},
    {0x82fecb,'command.return'},{0x82fedf,'command.return'},
    {0x82fef3,'command.return'},{0x82ff07,'command.return'},
    {0x80a82f,'voice_volume.entry'},{0x82fda9,'bounce_volume.return'},
    {0x878c2d,'working_gain.entry'},{0x878c65,'working_gain.exit'},
    {0x809c47,'driver_gain.entry'},{0x809c74,'driver_gain.exit'},
    {0x82f89a,'presentation_audio.entry'},{0x82f8b7,'presentation_audio.exit'},
    {0x8795e3,'logical_iteration.end'},{0x879b0d,'elapsed.entry'},
    {0x878e5b,'slow_motion.entry'},{0x878e7f,'slow_motion.exit'}})do
    local pc,kind=item[1],item[2]
    hook(pc,function()if court>=0 or kind:find('gain') or kind=='command.entry' then event(kind,pc)end end)
end
for _,range in ipairs({{0x07f6,0x07f7},{0x13e5,0x13e9},{0x0626,0x0628},{0x0631,0x0641},{0x15c5,0x15c8}})do
    emu.addMemoryCallback(function(address,value)
        if court<0 and setup<0 then return end
        local s=emu.getState()
        writes:write(json({kind=injecting and 'injected_callback_write' or 'native_wram_write',frame=frame,court=court,dispatch=active_dispatch,
            address=address,value=value,pc=(s['cpu.k']<<16)|s['cpu.pc'],cycle=s['cpu.cycleCount'],scanline=s['ppu.scanline']})..'\n')
    end,emu.callbackType.write,range[1],range[2],emu.cpuType.snes,emu.memType.snesWorkRam)
end
local dsp_address=0
emu.addMemoryCallback(function(_,value)dsp_address=value&127 end,
    emu.callbackType.write,0xf2,0xf2,emu.cpuType.spc,emu.memType.spcMemory)
emu.addMemoryCallback(function(_,value)
    if setup<0 then return end
    local s=emu.getState();local row={kind='dsp_write',frame=frame,court=court,
        action=action_index,dispatch=active_dispatch,register=dsp_address,value=value,
        spc_cycle=s['spc.cycle'],cpu_cycle=s['cpu.cycleCount']}
    if dsp_address==0x4c and value~=0 then
        row.voices={}
        local directory=emu.read(0x5d,emu.memType.spcDspRegisters,false)*256
        for voice=0,7 do if value&(1<<voice)~=0 then
            local registers=bytes(voice*16,8,emu.memType.spcDspRegisters)
            row.voices[#row.voices+1]={voice=voice,registers=registers,
                directory=directory,source_entry=bytes(directory+registers[5]*4,4,emu.memType.spcRam)}
        end end
    end
    audio_writes:write(json(row)..'\n')
end,emu.callbackType.write,0xf3,0xf3,emu.cpuType.spc,emu.memType.spcMemory)
for bank=0,0xbf do if bank<=0x3f or bank>=0x80 then
    emu.addMemoryCallback(function(address,value)
        if setup<0 then return end
        local s=emu.getState()
        audio_writes:write(json({kind='cpu_apu_port_write',frame=frame,court=court,
            action=action_index,dispatch=active_dispatch,address=address,value=value,
            pc=(s['cpu.k']<<16)|s['cpu.pc'],cpu_cycle=s['cpu.cycleCount'],spc_cycle=s['spc.cycle']})..'\n')
    end,emu.callbackType.write,bank*65536+0x2140,bank*65536+0x2143,
        emu.cpuType.snes,emu.memType.snesMemory)
end end
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
    local input={}
    if court>=0 then
    elseif player>=0 then
        input.left=pulse(player,400)
        input.start=player>=700 and (player-700)%200<3
    elseif setup>=0 then
        if action_tick>=0 and action_index<=#actions then
            local a=actions[action_index];if action_tick<a.hold then input[a.key]=true end
        elseif action_index>#actions and setup%200<3 then input.start=true end
    else input.start=pulse(title,850)end
    emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
    frame=frame+1;require_case(frame<24000,'audio journey timeout')
    if title>=0 and setup<0 then title=title+1 end
    if setup>=0 and player<0 then
        setup=setup+1
        if action_tick<0 and setup>=400 then action_tick=0;actions_log:write(json(snapshot('before_first_action',0))..'\n')end
        if action_tick>=0 and action_index<=#actions then
            action_tick=action_tick+1
            if action_tick>=actions[action_index].wait then
                actions_log:write(json(snapshot('after_action',0))..'\n')
                action_index=action_index+1;action_tick=0
            end
        end
    end
    if player>=0 and court<0 then player=player+1 end
    if court>=0 then
        frames:write(json({state=snapshot('end_frame',0),dsp=bytes(0,128,emu.memType.spcDspRegisters)})..'\n')
        court=court+1
        if court>=max_court then
            require_case(active_dispatch==0,'capture stopped inside audio dispatch')
            if mode=='controlled' then require_case(controlled_case==#cases,'controlled population incomplete')end
            local done=assert(io.open(out..'/capture_complete.json','wb'))
            done:write(json({frames=frame,court=court,dispatches=dispatch,controlled_cases=controlled_case,mode=mode}));done:close()
            events:close();writes:close();frames:close();actions_log:close();audio_writes:close();emu.stop(0)
        end
    end
end,emu.eventType.endFrame)
