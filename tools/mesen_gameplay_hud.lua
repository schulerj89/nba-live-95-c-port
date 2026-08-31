-- Natural Player Setup neutral -> CPU match -> first score-panel publisher.
-- Only controller input is supplied. No RAM, CPU, savestate or ROM mutation.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local alternate=os.getenv('NBA95_CONTROL_TEAM_VARIANT')=='1'
local log=assert(io.open(out..'/hud.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local published=-1
local basket=-1
local calls={}
local function word(a)
    return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)
end
local function dump(name,kind,size)
    local chunks={}
    for base=0,size-1,1024 do
        local bytes={}
        for i=base,math.min(base+1023,size-1) do bytes[#bytes+1]=string.char(emu.read(i,kind))end
        chunks[#chunks+1]=table.concat(bytes)
    end
    local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(chunks));f:close()
end
local function rgb(name)
    local pixels=emu.getScreenBuffer();assert(#pixels==256*239)
    local bytes={}
    for _,v in ipairs(pixels)do bytes[#bytes+1]=string.char((v>>16)&255,(v>>8)&255,v&255)end
    local f=assert(io.open(out..'/'..name..'.rgb','wb'));f:write(table.concat(bytes));f:close()
end
local function state(tag,pc)
    local s=emu.getState()
    log:write(string.format('{"tag":"%s","pc":%d,"frame":%d,"court":%d,"cpu_a":%d,"cpu_x":%d,"cpu_y":%d,"cpu_d":%d,"teams":[%d,%d],"scores":[%d,%d],"period":%d,"clock":%d,"requester":%d,"timer":%d,"sequence":%d,"kind":%d,"phase":%d,"counter":%d,"canvas_x":%d,"canvas_y":%d}\n',tag,pc,frame,court,s['cpu.a']or 0,s['cpu.x']or 0,s['cpu.y']or 0,s['cpu.d']or 0,word(0x46eb),word(0x476b),word(0x4711),word(0x4791),word(0x926),word(0x928),word(0x95e),word(0x8de),word(0x8e6),word(0x8e8),word(0x8e4),word(0x4941),word(0x18c6),word(0x18c8)))
    log:flush()
end
local function complete(status)
    local f=assert(io.open(out..'/capture_complete.txt','wb'));f:write(status);f:close();log:close();emu.stop(0)
end
local function hook(pc,fn)
    emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()
    if court<0 then
        court=0;state('first_court',0x87a47a)
        assert(word(0x166d)==1,'controller must be neutral for natural CPU match')
        dump('first_court.wram',emu.memType.snesWorkRam,0x20000)
        dump('first_court.vram',emu.memType.snesVideoRam,0x10000)
        dump('first_court.cgram',emu.memType.snesCgRam,0x200)
    end
end)
for _,pc in ipairs({0x83cc10,0x83ce36,0x83cfd5,0x83d0ad,0x83d156,0x83d157,0x83d1b0,0x83d1b1,0x83d1fc,0x83d1fd,0x83d24f,0x83d2e0,0x83d332,0x83ebdb,0x87bbe9,0x87bd2e})do
    hook(pc,function()
        if court<0 then return end
        state('publisher',pc)
        calls[pc]=(calls[pc]or 0)+1
        if calls[pc]<=1 and pc~=0x83cc10 and pc~=0x87bbe9 and pc~=0x87bd2e then
            local name=string.format('publisher_%06x',pc)
            dump(name..'.wram',emu.memType.snesWorkRam,0x20000)
            dump(name..'.vram',emu.memType.snesVideoRam,0x10000)
            dump(name..'.cgram',emu.memType.snesCgRam,0x200)
        end
        if pc==0x83ce36 and basket<0 then basket=court end
        if pc==0x83d157 and published<0 then published=court end
    end)
end
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
    local input={}
    if court>=0 then -- Neutral controllers; no gameplay button injection.
    elseif player>=0 then
        input.left=pulse(player,400)
        input.start=player>=700 and (player-700)%200<3
    elseif setup>=0 then
        if alternate then
            input.start=pulse(setup,400)or(setup>=850 and (setup-850)%200<3)
            input.right=pulse(setup,650)or pulse(setup,750)
            input.l=pulse(setup,700)
        else input.start=pulse(setup,400)or(setup>=650 and (setup-650)%200<3)end
    else input.start=pulse(title,850)end
    emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
    frame=frame+1
    assert(frame<24000,'natural HUD journey exceeded24000frames')
    if title>=0 and setup<0 then title=title+1 end
    if setup>=0 and player<0 then setup=setup+1 end
    if player>=0 and court<0 then player=player+1 end
    if court>=0 then
        state('end_frame',0)
        if court==30 or (basket>=0 and (published<0 or court<=published+60))then
            local name=string.format('court_%05d',court)
            rgb(name)
            dump(name..'.vram',emu.memType.snesVideoRam,0x10000)
            dump(name..'.cgram',emu.memType.snesCgRam,0x200)
            dump(name..'.wram',emu.memType.snesWorkRam,0x20000)
            local s=emu.getState();local keys={}
            for key in pairs(s)do if key:sub(1,4)=='ppu.'then keys[#keys+1]=key end end
            table.sort(keys)
            local f=assert(io.open(out..'/'..name..'.ppu','wb'))
            for _,key in ipairs(keys)do f:write(key..'='..tostring(s[key])..'\n')end
            f:close()
        end
        if published>=0 and court>=published+60 then complete('first natural HUD publication observed; court='..published..'\n');return end
        court=court+1
        if court==12000 then complete('HUD publisher not reached in12000 native courtframes\n');return end
    end
end,emu.eventType.endFrame)
