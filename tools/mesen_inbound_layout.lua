-- Controlled original-ROM C37D cases. Menus use real buttons. Only the
-- listed WRAM inputs change at a genuine call; stop at its first C5C0.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local case=assert(tonumber(os.getenv('NBA95_LAYOUT_CASE')))
local cases={
 {1,404,-45,336,404}, {1,-404,45,-336,-404},
 {1,404,-300,336,404}, {1,-404,300,-336,-404},
 {1,404,-45,-336,404}, {4,404,-45,336,404},
 {4,404,-45,-336,404}, {-1,404,-45,336,404}
}
assert(case>=0 and case<=#cases)
local frame,title,setup,player,court=0,-1,-1,-1,-1
local active=false
local pcs={}
local function file(name,data)local f=assert(io.open(out..'/'..name,'wb'));f:write(data);f:close()end
file('observed-script-data-folder.txt',emu.getScriptDataFolder()..'\n')
local function put(a,v)v=v&0xffff;emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,v>>8,emu.memType.snesWorkRam)end
local function snapshot(name)
 local bytes={};for a=0,0x4aff do bytes[#bytes+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
 file(name..'.bin',table.concat(bytes))
 local s=emu.getState();local parts={}
 for _,k in ipairs({'a','x','y','ps','sp','d','dbr','k','pc'})do parts[#parts+1]='"'..k..'":'..s['cpu.'..k]end
 file(name..'.json','{"frame":'..frame..',"court":'..court..',"cpu":{'..table.concat(parts,',')..'}}\n')
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0 end end)
hook(0x85c37d,function()
 if court<0 then return end
 assert(not active);snapshot('before')
 if case>0 then
  local c=cases[case]
  put(0x956,c[1]);put(0x9b0,c[2]);put(0x9b2,c[3]);put(0x952,5)
  put(0x4775,c[4]);put(0x3eef,c[5]);put(0x7f6,0x9146);put(0x994,0)
 end
 active=true;snapshot('entry')
end)
emu.addMemoryCallback(function()
 if active then local s=emu.getState();pcs[#pcs+1]=s['cpu.k']*65536+s['cpu.pc'] end
end,emu.callbackType.exec,0x85c37d,0x85c65b,emu.cpuType.snes,emu.memType.snesMemory)
hook(0x85c5c0,function()
 if not active then return end
 snapshot('exit');file('pcs.json','['..table.concat(pcs,',')..']\n')
 file('capture_complete.txt','case='..case..'\n');active=false;emu.stop(0)
end)
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
 local input={}
 if court>=0 then
  -- The established human-right capture route. All-neutral Player Setup
  -- did not reach an inbound call in the retained first capture attempt.
  if court>=120 then
   local block=(court-120)//120;local n=(court-120)%120;local d=block%9
   if n>=10 and n<50 then
    input.up=d==0 or d==1 or d==7;input.right=d==1 or d==2 or d==3
    input.down=d==3 or d==4 or d==5;input.left=d==5 or d==6 or d==7
   end
   input.b=pulse(n,30)or pulse(n,80)
  end
 elseif player>=0 then input.start=player>=700 and(player-700)%200<3
 elseif setup>=0 then input.start=pulse(setup,400)or(setup>=650 and(setup-650)%200<3)
 else input.start=pulse(title,850)end
 emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
 frame=frame+1;assert(frame<18000,'no natural inbound call')
 if frame%600==0 then file('progress.json',string.format('{"frame":%d,"title":%d,"setup":%d,"player":%d,"court":%d}\n',frame,title,setup,player,court))end
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then court=court+1 end
end,emu.eventType.endFrame)
