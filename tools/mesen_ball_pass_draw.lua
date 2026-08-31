-- Ordinary neutral CPU route; read-only source draw boundaries, no state seeds.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'));home:write(emu.getScriptDataFolder()..'\n');home:close()
local log=assert(io.open(out..'/boundaries.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local serial,calls,actor,active,nmidepth=0,0,0,false,0
local sampled={}
local function w(a)return emu.read(a&0x1ffff,emu.memType.snesWorkRam)|(emu.read((a+1)&0x1ffff,emu.memType.snesWorkRam)<<8)end
local function snap(tag,pc)
 local s=emu.getState();serial=serial+1;local name=string.format('raw-%05d.bin',serial)
 local chunks={};for base=0,0x1ffff,1024 do local b={};for a=base,base+1023 do b[#b+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end;chunks[#chunks+1]=table.concat(b)end
 local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(chunks));f:close()
 local r={index=serial,call=calls,pc=pc,frame=frame,court=court,actor=actor,nmi_depth=nmidepth,master_clock=assert(s.masterClock),cpu_cycles=assert(s['cpu.cycleCount']),ppu_frame=assert(s['ppu.frameCount']),scanline=assert(s['ppu.scanline'])}
 for _,key in ipairs({'a','x','y','ps','d','sp','dbr','k','pc'})do r['cpu_'..key]=assert(s['cpu.'..key])end
 log:write(string.format('{"tag":"%s","raw":"%s"',tag,name));local keys={};for k in pairs(r)do keys[#keys+1]=k end;table.sort(keys)
 for _,key in ipairs(keys)do log:write(string.format(',"%s":%d',key,r[key]))end;log:write('}\n');log:flush()
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0;assert(w(0x166d)==1);snap('court.entry',0x87a47a)end end)
hook(0x87a4e1,function()
 if court<0 then return end
 assert(not active,'unfinished previous draw')
 actor=assert(emu.getState()['cpu.x'])
 local mode=w(actor+0x5e);local key=string.format('%d:%d',mode,w(actor+0x52))
 local max=mode==15 and 12 or 1
 if (sampled[key]or 0)>=max or calls>=64 then return end
 sampled[key]=(sampled[key]or 0)+1;calls=calls+1;active=true;snap('actor.entry',0x87a4e1)
end)
for _,v in ipairs({{'resources.ready',0x87a51c},{'direction.commit',0x87a61e},{'subject.entry',0x80af1e},{'ordinary.entry',0x80ad92},{'ball.entry',0x80b0ff},{'ball.submit',0x80b11b},{'actor.return',0x87a6a8}})do
 local tag,pc=v[1],v[2];hook(pc,function()if active then snap(tag,pc);if tag=='actor.return'then active=false end end end)
end
hook(0x87a72f,function()active=false end)
hook(0x80815a,function()if active then nmidepth=nmidepth+1;snap('nmi.entry',0x80815a)end end)
for _,pc in ipairs({0x808171,0x80859b})do hook(pc,function()if active and nmidepth>0 then snap('nmi.exit',pc);nmidepth=nmidepth-1 end end)end
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
 frame=frame+1;assert(frame<6500,'ordinary route timeout')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then court=court+1;if court>=600 then
  assert(not active,'unfinished draw at stop')
  local f=assert(io.open(out..'/capture_complete.txt','wb'));f:write(string.format('Ball ordinary CPU draw; frames=%d court=%d calls=%d records=%d\n',frame,court,calls,serial));f:close();log:close();emu.stop(0)
 end end
end,emu.eventType.endFrame)
