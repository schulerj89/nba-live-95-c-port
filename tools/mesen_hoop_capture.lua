-- Ordinary CPU game: controller inputs and read-only PPU observations.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local duration=tonumber(os.getenv('NBA95_HOOP_FRAMES') or '24')
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local trace=assert(io.open(out..'/frames.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local counts={north=0,south=0}
local selected,initial,events,narrow_source=nil,nil,{},nil
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function signed(a)return a>=32768 and a-65536 or a end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function dump(name,kind,size)
 local b={};for a=0,size-1 do b[#b+1]=string.char(emu.read(a,kind))end
 local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(b));f:close()
end
local function shot(prefix)
 local pixels=emu.getScreenBuffer();assert(#pixels==256*239)
 local b={};for i,color in ipairs(pixels)do b[i]=string.char((color>>16)&255,(color>>8)&255,color&255)end
 local f=assert(io.open(out..'/'..prefix..'.rgb','wb'));f:write(table.concat(b));f:close()
 dump(prefix..'.vram',emu.memType.snesVideoRam,0x10000)
 dump(prefix..'.cgram',emu.memType.snesCgRam,0x200)
 dump(prefix..'.oam',emu.memType.snesSpriteRam,0x220)
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0;assert(w(0x166d)==1,'expected neutral controllers')end end)
-- This is after the real scroll/window upload and raster IRQ setup.
hook(0x8084a1,function()
 if court<0 then return end
 local s=emu.getState();local fields={}
 for k,v in pairs(s)do if k:match('^ppu%.')then fields[#fields+1]=k..'='..tostring(v)..'\n' end end
 table.sort(fields)
 initial={frame=frame,court=court,x=signed(w(0x85c)),y=signed(w(0x860)),
  hscroll=w(0x87c),vscroll=w(0x87e),left=w(0x802),right=w(0x800),
  timer=w(0x1807),state=table.concat(fields)}
 events={};narrow_source=nil
end)
hook(0x85ef14,function()if initial then narrow_source=w(0x800)end end)
local function onwrite(address,value)
 if not initial then return end
 local reg=address&0xffff
 if reg~=0x212c and reg~=0x2129 then return end
 local s=emu.getState()
 events[#events+1]=string.format('[%d,%d,%d,%d]',s['ppu.scanline'],reg,value,(s['cpu.k']<<16)|s['cpu.pc'])
end
for bank=0,0xbf do
 if bank<=0x3f or bank>=0x80 then
  emu.addMemoryCallback(onwrite,emu.callbackType.write,bank*0x10000+0x2129,bank*0x10000+0x212c,emu.cpuType.snes,emu.memType.snesMemory)
 end
end
-- Capture the just-completed scanout before any next-frame upload.
hook(0x808188,function()
 if not initial then return end
 if not selected then
  if counts.south==0 and initial.x < -365 then selected='south'
  elseif counts.north==0 and initial.x>150 and initial.y>-215 then selected='north' end
 end
 if not selected then return end
 local side=selected;counts[side]=counts[side]+1
 local prefix=string.format('%s_%03d',side,counts[side])
 shot(prefix)
 local state=assert(io.open(out..'/'..prefix..'.state','wb'));state:write(initial.state);state:close()
 trace:write(string.format('{"name":"%s","frame":%d,"court":%d,"camera":[%d,%d],"scroll":[%d,%d],"window":[%d,%d],"timer":%d,"narrow_source":%s,"writes":[%s]}\n',
  prefix,initial.frame,initial.court,initial.x,initial.y,initial.hscroll,initial.vscroll,initial.left,initial.right,initial.timer,narrow_source and tostring(narrow_source)or'null',table.concat(events,',')))
 trace:flush()
 if counts[side]==duration then selected=nil end
 if counts.north==duration and counts.south==duration then
  trace:close();local f=assert(io.open(out..'/capture_complete.txt','wb'))
  f:write(string.format('%d north and %d south frames; no state injection\n',counts.north,counts.south));f:close();emu.stop(0)
 end
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
 frame=frame+1;assert(frame<14000,'ordinary route timeout')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then court=court+1 end
end,emu.eventType.endFrame)
