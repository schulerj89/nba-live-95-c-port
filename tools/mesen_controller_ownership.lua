-- Natural Player Setup -> first-court controller ownership investigation.
-- No emu.write, savestate, CPU mutation or RAM repair. Inputs use pad0 only.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local target=assert(tonumber(os.getenv('NBA95_CONTROL_SELECTION')))
local team_variant=os.getenv('NBA95_CONTROL_TEAM_VARIANT')=='1'
local pause_at=tonumber(os.getenv('NBA95_CONTROL_PAUSE_AT')) or -1
assert(target>=0 and target<=2)
local log=assert(io.open(out..'/ownership.jsonl','wb'))
local journey=assert(io.open(out..'/journey.txt','wb'))
local frame,title,setup,player,court,pause_frame=0,-1,-1,-1,-1,-1
local snapshots,init_calls,update_calls,human_calls=0,0,0,0
local function word(a)
    return emu.read(a,emu.memType.snesWorkRam)|
           (emu.read(a+1,emu.memType.snesWorkRam)<<8)
end
local function words(base,count,stride)
    local result={}
    for i=0,count-1 do result[#result+1]=tostring(word(base+i*(stride or 2)))end
    return '['..table.concat(result,',')..']'
end
local function dump_wram(name)
    local chunks={}
    for base=0,0x1ffff,1024 do
        local bytes={}
        for offset=0,1023 do bytes[#bytes+1]=string.char(emu.read(base+offset,emu.memType.snesWorkRam))end
        chunks[#chunks+1]=table.concat(bytes)
    end
    local f=assert(io.open(out..'/'..name..'.wram','wb'));f:write(table.concat(chunks));f:close()
end
local function require_case(condition,message)
    if condition then return end
    local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
    log:flush();journey:flush();emu.stop(1);error(message)
end
local function snapshot(tag,pc)
    local s=emu.getState(); local records,actors={},{}
    for i=0,4 do records[#records+1]=words(0x47eb+i*0x40,0x20)end
    for i=0,9 do
        local base=0x34eb+i*0x100
        actors[#actors+1]=string.format(
            '{"slot":%d,"index":%d,"x_frac":%d,"x":%d,"y_frac":%d,"y":%d,"z":%d,"vx":%d,"vy":%d,"controller":%d,"mode":%d,"group":%d}',
            i,word(base),word(base+2),word(base+4),word(base+6),word(base+8),
            word(base+0x0c),word(base+0x22),word(base+0x24),word(base+0x16),
            word(base+0x5e),word(base+0x6e))
    end
    log:write(string.format(
        '{"tag":"%s","native_pc":%d,"global_frame":%d,"court_frame":%d,"cpu_d":%d,"cpu_a":%d,"cpu_x":%d,"cpu_y":%d,"selections":%s,"previous":%s,"flags1681":%s,"records": [%s],"actors":[%s],"counts":[%d,%d],"globals":{"07f8":%d,"090c":%d,"0936":%d,"093a":%d,"093e":%d,"0978":%d,"166b":%d,"095e":%d,"08d2":%d,"4715":%d,"4795":%d},"dp96":%d,"dp9a":%d,"dpae":%d,"dpb6":%d,"poll0576":%s}\n',
        tag,pc,frame,court,s['cpu.d'] or -1,s['cpu.a'] or 0,s['cpu.x'] or 0,s['cpu.y'] or 0,
        words(0x166d,5),words(0x1677,5),words(0x1681,5),table.concat(records,','),table.concat(actors,','),
        word(0x4726),word(0x47a6),word(0x07f8),word(0x090c),word(0x0936),word(0x093a),word(0x093e),word(0x0978),word(0x166b),
        word(0x095e),word(0x08d2),word(0x4715),word(0x4795),
        word(0x96),word(0x9a),word(0xae),word(0xb6),words(0x576,5)))
    log:flush(); snapshots=snapshots+1
end
local function rgb(name)
    local pixels=emu.getScreenBuffer();assert(#pixels==256*239)
    local b={};for _,v in ipairs(pixels)do b[#b+1]=string.char((v>>16)&255,(v>>8)&255,v&255)end
    local f=assert(io.open(out..'/'..name..'.rgb','wb'));f:write(table.concat(b));f:close()
end
local function hook(pc,fn)
    emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()
    if player<0 then player=0;journey:write('PlayerSetup entry at '..frame..'\n');snapshot('player_setup.entry',0x81a489)end
end)
hook(0x86e208,function()init_calls=init_calls+1;snapshot('initialize.entry',0x86e208)end)
hook(0x86e24b,function()snapshot('initialize.exit',0x86e24b)end)
hook(0x86e24c,function()update_calls=update_calls+1;snapshot('allocate.entry',0x86e24c)end)
hook(0x86e389,function()
    snapshot('allocate.exit',0x86e389)
    dump_wram('allocate_exit_'..update_calls)
end)
hook(0x87a47a,function()
    if court<0 then
        court=0;journey:write('Firstcourt at '..frame..'\n');snapshot('first_court',0x87a47a)
        dump_wram('first_court')
        require_case(init_calls>0,'native controller initialization was not observed')
        require_case(word(0x166d)==target,'native selection changed between PlayerSetup and court')
    end
end)
hook(0x86818d,function()
    if pause_at>=0 and court>=0 then snapshot('pause_request.entry',0x86818d);dump_wram('pause_request_entry')end
end)
hook(0x8681d2,function()
    if pause_at>=0 and court>=0 then
        pause_frame=0;snapshot('pause_team.exit',0x8681d2);dump_wram('pause_team_exit')
    end
end)
-- Observe the common action dispatcher only for actually controller-owned
-- actors. The native caller supplies virtual/current buttons in DP$AE.
hook(0x84e2ac,function()
    if court>=0 then
        local actor=word(0x96)
        if actor>=0x34eb and actor<0x3eeb and word(actor+0x16)<5 then
            human_calls=human_calls+1;snapshot('action.entry',0x84e2ac)
        end
    end
end)
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
    local input={}
    if pause_frame>=0 then
        -- The optional bounded pause case ends at native team publication.
        -- It does not choose a menu item or claim timeout confirmation.
    elseif court>=0 then
        input.right=court>=60 and court<90
        input.up=court>=110 and court<140
        input.b=court>=170 and court<185
        input.a=pulse(court,230)
        input.y=court>=300 and court<320
        if pause_at>=0 then input.start=pulse(court,pause_at)end
    elseif player>=0 then
        if target<2 and pulse(player,400)then input.left=true end
        if target<1 and pulse(player,460)then input.left=true end
        if player>=700 and (player-700)%200<3 then input.start=true end
    elseif setup>=0 then
        if team_variant then
            input.start=pulse(setup,400) or (setup>=850 and (setup-850)%200<3)
            input.right=pulse(setup,650) or pulse(setup,750)
            input.l=pulse(setup,700)
        else input.start=pulse(setup,400) or (setup>=650 and (setup-650)%200<3)end
    else input.start=pulse(title,850)end
    emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
    frame=frame+1
    require_case(frame<16000,'natural controller journey exceeded16000frames')
    if title>=0 and setup<0 then title=title+1 end
    if setup>=0 and player<0 then setup=setup+1 end
    if player>=0 and court<0 then
        player=player+1
        if player==350 then snapshot('player_setup.before_input',0);rgb('player_before')end
        if player==580 then
            snapshot('player_setup.after_input',0);rgb('player_after')
            dump_wram('player_after')
            require_case(word(0x166d)==target,'natural left inputs did not reach requested selection')
        end
    end
    if court>=0 then
        if pause_frame>=0 then pause_frame=pause_frame+1 end
        if court%10==0 then snapshot('court.end_frame',0)end
        if court==0 or court==90 or court==140 or court==185 or court==320 then rgb('court_'..court)end
        court=court+1
        if pause_at>=0 then require_case(court<2000,'natural requesting-controller pause journey did not complete')end
        if (pause_at<0 and court==400) or (pause_at>=0 and pause_frame>=0) then
            log:close();journey:close()
            local f=assert(io.open(out..'/capture_complete.txt','wb'))
            f:write(string.format('selection=%d\nsnapshots=%d\ninit_calls=%d\nallocation_calls=%d\nhuman_action_calls=%d\n',target,snapshots,init_calls,update_calls,human_calls));f:close();emu.stop(0)
        end
    end
end,emu.eventType.endFrame)
