-- Fresh ordinary CPU game. Only controller inputs and read-only observations.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local duration=tonumber(os.getenv('NBA95_DRIBBLE_FRAMES') or '240')
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local trace=assert(io.open(out..'/frames.jsonl','wb'))
local draws=assert(io.open(out..'/draws.jsonl','wb'))
local poses=assert(io.open(out..'/poses.jsonl','wb'))
local calls=assert(io.open(out..'/calls.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local first,images,serial,active=-1,0,0,nil
local draw_active,draw_serial=nil,0
local function w(a)return emu.read(a&0x1ffff,emu.memType.snesWorkRam)|(emu.read((a+1)&0x1ffff,emu.memType.snesWorkRam)<<8)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function array(values)return '['..table.concat(values,',')..']'end
local globals={0x93e,0x94a,0x970,0x9f6,0x968,0x962,0x948}
local ball={0x3eed,0x3eef,0x3ef1,0x3ef3,0x3ef5,0x3ef7,0x3ef9,0x3efb,0x3efd}
local offsets={2,4,6,8,10,12,0x2a,0x2c,0x28,0x3a,0x5e,0xa8,0x4e,0x50,0x52}
local tail={0x13e5,0x13e7,0x936,0x922,0x924}
local function append(values,addresses,base)
 for _,a in ipairs(addresses)do values[#values+1]=w(a+(base or 0))end
end
local function isdribble()
 local owner=w(0x93e)
 if owner>=10 or w(0x936)>=0x80 or w(0x978)~=0 then return false end
 local base=0x34eb+owner*0x100
 return w(base+0x5e)==11 and (w(base+0x38)==9 or w(base+0x38)==11)
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0;assert(w(0x166d)==1,'expected neutral controllers')end end)
hook(0x859a37,function()
 if court<0 or not isdribble() then return end
 assert(not active,'unfinished owned-ball call')
 assert(w(0xc6)==2,'unexpected physical scheduler quantum')
 local values={};append(values,globals);values[#values+1]=w(0x93a)
 append(values,ball);append(values,offsets,0x34eb+w(0x93e)*0x100)
 for i=0,9 do values[#values+1]=w(0x34eb+i*0x100+0x16)end
 append(values,tail);assert(#values==47)
 serial=serial+1
 active={input=values,frame=frame,court=court,serial=serial,
  base=w(0x34eb+w(0x93e)*0x100+0x38),subject=w(0x940)}
end)
hook(0x85a7c7,function()
 if not active then return end
 local values={};append(values,globals);append(values,ball)
 for i=0,9 do values[#values+1]=w(0x34eb+i*0x100+0x16)end
 append(values,tail);assert(#values==31)
 calls:write(string.format('{"call":%d,"entry_frame":%d,"exit_frame":%d,"court":%d,"base":%d,"subject":%d,"input":%s,"expected":%s}\n',
  active.serial,active.frame,frame,active.court,active.base,active.subject,array(active.input),array(values)))
 calls:flush();active=nil
end)
hook(0x80af1e,function()
 if court<0 or not isdribble() then return end
 assert(not draw_active,'unfinished owner draw')
 local s=emu.getState();local v={}
 append(v,{0xd6,0xd4,0xda,0xd8,0x47,0x51,0xc0})
 v[#v+1]=s['cpu.a'];v[#v+1]=w(0x884);v[#v+1]=s['cpu.x'];v[#v+1]=s['cpu.y']
 append(v,{0x9a,0x3f31,0x4015,0x3f33})
 v[#v+1]=(w(0xa2)+w(0x3f97))&0xffff
 append(v,{0x92,0x8e})
 draw_serial=draw_serial+1
 draw_active={input=v,parts={},frame=frame,court=court,
  actor_depth=w(0x34eb+w(0x93e)*0x100+0x68),ball_depth=w(0x3f53),serial=draw_serial}
end)
hook(0x80b348,function()
 if not draw_active then return end
 local s=emu.getState()
 draw_active.parts[#draw_active.parts+1]={s['cpu.a'],w(0x14),s['cpu.x'],s['cpu.y']}
end)
hook(0x80b0ab,function()
 if not draw_active then return end
 local parts={};for _,part in ipairs(draw_active.parts)do parts[#parts+1]=array(part)end
 poses:write(string.format('{"call":%d,"frame":%d,"court":%d,"actor_depth":%d,"ball_depth":%d,"input":%s,"parts":[%s]}\n',
  draw_active.serial,draw_active.frame,draw_active.court,draw_active.actor_depth,draw_active.ball_depth,array(draw_active.input),table.concat(parts,',')))
 poses:flush();draw_active=nil
end)
hook(0x80b11b,function()
 if court<0 then return end
 local s=emu.getState()
 draws:write(string.format('{"frame":%d,"court":%d,"owner":%d,"x":%d,"y":%d,"resource":%d,"dp_x":%d,"dp_y":%d}\n',
  frame,court,w(0x93e),s['cpu.x'],s['cpu.y'],w(0),w(0x92),w(0x8e)))
end)
local function dump(name,kind,size)
 local b={};for a=0,size-1 do b[#b+1]=string.char(emu.read(a,kind))end
 local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(b));f:close()
end
local function shot(index)
 local pixels=emu.getScreenBuffer();assert(#pixels==256*239)
 local chunks={}
 for i,color in ipairs(pixels)do chunks[i]=string.char((color>>16)&255,(color>>8)&255,color&255)end
 local f=assert(io.open(out..string.format('/frame_%04d.rgb',index),'wb'))
 f:write(table.concat(chunks));f:close()
 dump(string.format('frame_%04d.oam',index),emu.memType.snesSpriteRam,0x220)
 if index==1 then
  dump('vram.bin',emu.memType.snesVideoRam,0x10000)
  dump('cgram.bin',emu.memType.snesCgRam,0x200)
 end
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
 frame=frame+1;assert(frame<10000,'ordinary route timeout')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court<0 then return end
 court=court+1
 if first<0 and isdribble() then first=court end
 local owner=w(0x93e);local values={};append(values,ball)
 local actor={}
 if owner<10 then
  append(actor,{4,8,12,0x28,0x2a,0x2c,0x34,0x36,0x38,0x3a,0x3c,0x42,0x44,0x4e,0x50,0x52,0x5e},0x34eb+owner*0x100)
 end
 local captured=first>=0 and court-first<duration
 if captured then images=images+1;shot(images)end
 trace:write(string.format('{"frame":%d,"court":%d,"image":%d,"owner":%d,"ball":%s,"actor":%s,"dribble":%s}\n',
  frame,court,captured and images or 0,owner,array(values),array(actor),tostring(isdribble())))
 if first>=0 and court-first>=duration and not active and not draw_active then
  assert(images==duration and serial>0,'missing dribble evidence')
  local f=assert(io.open(out..'/capture_complete.txt','wb'))
  f:write(string.format('frames=%d court=%d first=%d images=%d calls=%d\n',frame,court,first,images,serial));f:close()
  trace:close();calls:close();draws:close();poses:close();emu.stop(0)
 end
end,emu.eventType.endFrame)
