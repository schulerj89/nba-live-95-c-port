-- Controlled clock formatter witnesses reached through native87BBE9.
-- A natural neutral-controller match reaches its first score panel first.
-- Per case, only declared WRAM words are injected; they and the original
-- clock text are restored at the formatter return before downstream use.
-- These are NOT natural gameplay trajectories or production asset sources.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local log=assert(io.open(out..'/clock-cases.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local ready,index,pending=false,0,nil
local cases={}
for _,value in ipairs({0,1,59,60,599,600,3599,3600,43199,43200,65535})do
    cases[#cases+1]={routine=0x87baf5,value=value,busy=0,event=0x20}
end
for _,value in ipairs({0,1,5,6,59,60,61,3599,3600,61440,65534,65535})do
    for _,busy in ipairs({0,1})do
        cases[#cases+1]={routine=0x87bb59,value=value,busy=busy,event=0x20}
    end
end
local addresses={0x492b,0x8de,0x8e8,0x8f6,0x928,0x92a,0x9b4,0x13e7}
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function words()
    local values={};for _,a in ipairs(addresses)do values[#values+1]=w(a)end
    return '['..table.concat(values,',')..']'
end
local function clocktext()
    local values={};for a=0x4a60,0x4a67 do values[#values+1]=emu.read(a,emu.memType.snesWorkRam)end
    return '['..table.concat(values,',')..']'
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()
    if court<0 then court=0;assert(w(0x166d)==1,'natural entry must have neutral controllers')end
end)
hook(0x83d332,function()if court>=0 then ready=true end end)
hook(0x87bbe9,function()
    if not ready or index>=#cases then return end
    assert(not pending,'nested controlled clock call')
    index=index+1;local c=cases[index]
    pending={saved={},text={}}
    for _,a in ipairs(addresses)do pending.saved[a]=w(a)end
    for a=0x4a60,0x4a67 do pending.text[a]=emu.read(a,emu.memType.snesWorkRam)end
    put(0x492b,0);put(0x8de,0);put(0x8e8,1);put(0x8f6,0xffff)
    put(0x928,c.routine==0x87baf5 and 43200 or 1)
    put(0x92a,c.value);put(0x9b4,c.busy);put(0x13e7,c.event)
end)
for _,pc in ipairs({0x87baf5,0x87bb59})do hook(pc,function()
    if not pending then return end
    assert(cases[index].routine==pc,'wrong natural child dispatched')
    assert(not pending.pre,'reentered formatter')
    pending.pre=words();pending.pretext=clocktext();pending.frame=frame
end)end
for _,pc in ipairs({0x87bb58,0x87bbe8})do hook(pc,function()
    if not pending then return end
    assert(pending.pre,'native formatter entry was not observed')
    local expected=cases[index].routine==0x87baf5 and 0x87bb58 or 0x87bbe8
    assert(pc==expected,'wrong native formatter return')
    log:write(string.format('{"case":%d,"controlled":true,"routine":%d,"entry_frame":%d,"exit_frame":%d,"entry":%s,"exit":%s,"entry_text":%s,"exit_text":%s}\n',index,cases[index].routine,pending.frame,frame,pending.pre,words(),pending.pretext,clocktext()));log:flush()
    for a,v in pairs(pending.saved)do put(a,v)end
    for a,v in pairs(pending.text)do emu.write(a,v,emu.memType.snesWorkRam)end
    pending=nil
    if index==#cases then
        log:close();local f=assert(io.open(out..'/capture_complete.txt','wb'))
        f:write('35 controlled native formatter entry/exit pairs; restored WRAM\n');f:close();emu.stop(0)
    end
end)end
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
    local input={}
    if court>=0 then
    elseif player>=0 then input.left=pulse(player,400);input.start=player>=700 and(player-700)%200<3
    elseif setup>=0 then input.start=pulse(setup,400)or(setup>=650 and(setup-650)%200<3)
    else input.start=pulse(title,850)end
    emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
    frame=frame+1;assert(frame<24000,'controlled clock journey exceeded24000frames')
    if title>=0 and setup<0 then title=title+1 end
    if setup>=0 and player<0 then setup=setup+1 end
    if player>=0 and court<0 then player=player+1 end
    if court>=0 then court=court+1 end
end,emu.eventType.endFrame)
