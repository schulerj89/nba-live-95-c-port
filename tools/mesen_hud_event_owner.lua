-- Natural neutral-controller journey, read-only shared HUD/event writer audit.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local log=assert(io.open(out..'/owners.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local active=false
local lastpc=0
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function record(tag,address,value)
    local s=emu.getState()
    log:write(string.format('{"tag":"%s","frame":%d,"court":%d,"active":%s,"lastpc":%d,"cpu_pc":%d,"cpu_k":%d,"address":%d,"value":%d,"event":%d,"secondary_event":%d,"rng":%d,"clock":%d,"snapshot":%d,"timer":%d,"sequence":%d,"busy":%d}\n',
        tag,frame,court,tostring(active),lastpc,s['cpu.pc']or 0,s['cpu.k']or 0,address,value,
        word(0x13e7),word(0x13e9),word(0x7f6),word(0x928),word(0x92a),word(0x8de),word(0x8e6),word(0x9b4)))
    log:flush()
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()
    if court<0 then court=0;assert(word(0x166d)==1,'controllers must be neutral');record('first_court',0,0)end
end)
hook(0x83d0ad,function()if court>=0 and not active then active=true;lastpc=0x83d0ad;record('layout_entry',0,0)end end)
hook(0x83d156,function()
    if not active then return end
    lastpc=0x83d156;record('layout_exit',0,0)
    local f=assert(io.open(out..'/capture_complete.txt','wb'));f:write('natural D0AD shared-writer capture complete\n');f:close()
    log:close();emu.stop(0)
end)
for _,range in ipairs({{0x82fd65,0x82ffff},{0x879400,0x8794ff}})do
    emu.addMemoryCallback(function(address)lastpc=address end,emu.callbackType.exec,range[1],range[2],emu.cpuType.snes,emu.memType.snesMemory)
end
for _,range in ipairs({{0x13e7,0x13ea},{0x92a,0x92b},{0x7f6,0x7f7}})do
    emu.addMemoryCallback(function(address,value)
        local low=address&0xffff -- Physical WRAM callback reports the mirrored CPU bus address.
        if active or (court>=960 and low>=0x92a and low<=0x92b)then record('write',address,value)end
    end,emu.callbackType.write,range[1],range[2],emu.cpuType.snes,emu.memType.snesWorkRam)
end
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
    local input={}
    if court>=0 then
    elseif player>=0 then input.left=pulse(player,400);input.start=player>=700 and (player-700)%200<3
    elseif setup>=0 then input.start=pulse(setup,400)or(setup>=650 and (setup-650)%200<3)
    else input.start=pulse(title,850)end
    emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
    frame=frame+1;assert(frame<24000,'HUD owner journey exceeded24000frames')
    if title>=0 and setup<0 then title=title+1 end
    if setup>=0 and player<0 then setup=setup+1 end
    if player>=0 and court<0 then player=player+1 end
    if court>=0 then court=court+1 end
end,emu.eventType.endFrame)
