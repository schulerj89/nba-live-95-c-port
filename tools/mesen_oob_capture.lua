-- Neutral-controller journey; optional declared owner-position WRAM fixture.
-- ROM, CPU registers, event words and HUD state are never patched.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local trace=assert(io.open(out..'/frames.jsonl','wb'))
local calls=assert(io.open(out..'/calls.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local first,retired,images=-1,-1,0
local active=nil
local controlled=os.getenv('NBA95_OOB_CONTROLLED')=='1'
local injected=false
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function dump(name,kind,size)
 local b={};for a=0,size-1 do b[#b+1]=string.char(emu.read(a,kind))end
 local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(b));f:close()
end
local function buffers(name)
 dump(name..'.wram',emu.memType.snesWorkRam,0x20000)
 dump(name..'.vram',emu.memType.snesVideoRam,0x10000)
 dump(name..'.cgram',emu.memType.snesCgRam,0x200)
end
local function shot(name)
 local p=emu.getScreenBuffer();assert(#p==256*239)
 local b={};for i,c in ipairs(p)do b[i]=string.char((c>>16)&255,(c>>8)&255,c&255)end
 local f=assert(io.open(out..'/'..name..'.rgb','wb'));f:write(table.concat(b));f:close()
 dump(name..'.vram',emu.memType.snesVideoRam,0x10000)
 dump(name..'.cgram',emu.memType.snesCgRam,0x200)
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0;assert(w(0x166d)==1,'expected neutral controllers')end end)
hook(0x8792a5,function()
 if not controlled or injected or court<300 or w(0x93e)>=10 or w(0x936)>=0x80 or
    w(0x8de)<0x8000 or w(0x9b6)~=0 or w(0x964)~=0 or w(0x978)~=0 then return end
 -- Place the grounded owner one unit beyond the sideline. The original
 -- bounds detector, possession turnover and HUD dispatcher do everything else.
 buffers('scenario_before')
 local owner=w(0x93e);local base=0x34eb+owner*0x100
 local writes={}
 for _,pair in ipairs({{base+4,0},{base+8,209},{base+12,0}})do
  local address,value=pair[1],pair[2]
  writes[#writes+1]=string.format('{"address":%d,"before":%d,"after":%d}',address,w(address),value)
  emu.write(address,value&255,emu.memType.snesWorkRam)
  emu.write(address+1,value>>8,emu.memType.snesWorkRam)
 end
 local f=assert(io.open(out..'/scenario.json','wb'))
 f:write(string.format('{"pc":%d,"frame":%d,"court":%d,"owner":%d,"writes":[%s]}\n',
  0x8792a5,frame,court,owner,table.concat(writes,',')));f:close()
 injected=true;buffers('scenario_after')
end)
for _,entry in ipairs({{0x83da12,0x83da8b},{0x83da8c,0x83db28},{0x83ebdb,0x83ed46}})do
 local pc,exit=entry[1],entry[2]
 hook(pc,function()
  if court<0 or w(0x8f0)~=3 or w(0x8e8)~=17 or retired>=0 then return end
  if first<0 then if pc~=0x83da12 then return end;first=court end
  assert(not active,'nested HUD child')
  active={pc=pc,frame=frame,court=court,name=string.format('child_%06x',pc)}
  buffers(active.name..'_before')
 end)
 hook(exit,function()
  if not active or active.pc~=pc then return end
  buffers(active.name..'_after')
  calls:write(string.format('{"pc":%d,"name":"%s","entry_frame":%d,"exit_frame":%d,"court":%d}\n',
   pc,active.name,active.frame,frame,active.court));calls:flush();active=nil
  if pc==0x83ebdb then retired=court end
 end)
end
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
 frame=frame+1;assert(frame<18000,'ordinary OOB route timeout')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court<0 then return end
 court=court+1
 local capture=first>=0 and (court-first<=24 or (court-first)%50==0 or (retired>=0 and court-retired<=12))
 local name=''
 if capture then images=images+1;name=string.format('frame_%04d',court);shot(name)end
 if first>=0 then
  trace:write(string.format('{"frame":%d,"court":%d,"name":"%s","timer":%d,"sequence":%d,"kind":%d,"event":%d,"actor":%d,"teams":[%d,%d],"clear":%d}\n',
   frame,court,name,w(0x8de),w(0x8e6),w(0x8e8),w(0x8f0),w(0x492d),w(0x46eb),w(0x476b),w(0x8ee)))
 end
 if retired>=0 and court-retired>=12 and not active then
  local f=assert(io.open(out..'/capture_complete.txt','wb'))
  f:write(string.format('frames=%d first=%d retired=%d images=%d\n',frame,first,retired,images));f:close()
  trace:close();calls:close();emu.stop(0)
 end
end,emu.eventType.endFrame)
