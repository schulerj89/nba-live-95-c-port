-- C1: ordinary cold boot/neutral CPU input only. Never writes emulated state.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local f=assert(io.open(out..'/observed-script-data-folder.txt','wb'));f:write(emu.getScriptDataFolder()..'\n');f:close()
local log=assert(io.open(out..'/events.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local serial,contact,active,keep,drop=0,0,0,0,0
local tracked={}
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function dump(name)
 local chunks={}
 for base=0,0x1ffff,1024 do
  local bytes={};for i=base,base+1023 do bytes[#bytes+1]=string.char(emu.read(i,emu.memType.snesWorkRam))end
  chunks[#chunks+1]=table.concat(bytes)
 end
 local h=assert(io.open(out..'/'..name,'wb'));h:write(table.concat(chunks));h:close()
end
local function record(tag,pc)
 serial=serial+1;local s=emu.getState();local x=s['cpu.x']or 0
 local name=string.format('%05d.wram',serial);dump(name)
 log:write(string.format('{"serial":%d,"contact":%d,"tag":"%s","pc":%d,"frame":%d,"court":%d,"cpu_a":%d,"cpu_x":%d,"cpu_y":%d,"cpu_d":%d,"cpu_ps":%d,"cpu_sp":%d,"cpu_dbr":%d,"cpu_k":%d,"cpu_pc":%d,"owner":%d,"passer":%d,"receiver":%d,"pass_active":%d,"rng":%d,"mode":%d,"timer":%d,"raw":"%s"}\n',serial,contact,tag,pc,frame,court,s['cpu.a']or 0,x,s['cpu.y']or 0,s['cpu.d']or 0,s['cpu.ps']or 0,s['cpu.sp']or 0,s['cpu.dbr']or 0,s['cpu.k']or 0,s['cpu.pc']or 0,word(0x93e),word(0x942),word(0x946),word(0x9c4),word(0x7f6),word((x+0x5e)&0x1ffff),word((x+0x60)&0x1ffff),name));log:flush()
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0;assert(word(0x166d)==1);record('first_court',0x87a47a)end end)
hook(0x86bff6,function()
 if court<0 then return end
 contact=contact+1;active=contact;record('contact.entry',0x86bff6)
end)
for _,pair in ipairs({{0x86bffa,'cancel.return'},{0x86c0f5,'owner.rng.entry'},{0x86c0f9,'owner.rng.return'},{0x86c0fe,'owner.keep'},{0x86c101,'owner.drop.roll'},{0x86c15a,'owner.drop.test'},{0x86c189,'owner.after'},{0x86c205,'pose.entry'},{0x86c236,'contact.exit'}})do
 local pc,tag=pair[1],pair[2]
 hook(pc,function()
  if active==0 then return end
  record(tag,pc)
  if tag=='owner.keep'then keep=keep+1 end
  if tag=='owner.drop.roll'then drop=drop+1 end
  if tag=='contact.exit'then local x=emu.getState()['cpu.x'];tracked[x]=court+180;active=0 end
 end)
end
hook(0x86c239,function()active=0 end)
hook(0x86c6ad,function()
 local x=word(0x96)
 if tracked[x]and court<=tracked[x]then record('recovery.entry',0x86c6ad)end
end)
hook(0x86c758,function()
 local x=word(0x96)
 if tracked[x]and court<=tracked[x]then record('recovery.exit',0x86c758);if word(x+0x5e)~=8 then tracked[x]=nil end end
end)
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
 frame=frame+1;assert(frame<22000,'C1 native route bound')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then
  court=court+1
  local done=next(tracked)==nil and keep>=2 and drop>=1
  if court>=14000 or(done and court>=3000)then
   local h=assert(io.open(out..'/capture_complete.txt','wb'));h:write(string.format('C1 natural CPU capture; frames=%d court=%d contacts=%d keep=%d drop=%d records=%d\n',frame,court,contact,keep,drop,serial));h:close();log:close();emu.stop(0)
  end
 end
end,emu.eventType.endFrame)
