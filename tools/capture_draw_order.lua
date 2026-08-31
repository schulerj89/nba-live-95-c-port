-- Normal cold boot/menu input only. No WRAM/ROM/register/PC/state writes.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local log=assert(io.open(out..'/boundaries.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local index,passes,init,basket,active=0,0,0,0,false
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function check(ok,message)
 if ok then return end
 local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
 log:flush();emu.stop(1);error(message)
end
local function snap(tag,pc)
 index=index+1;local s=emu.getState();local filename=string.format('raw_%04d.bin',index)
 check((s['cpu.k']<<16)|s['cpu.pc']==pc,'hook PC mismatch')
 local f=assert(io.open(out..'/'..filename,'wb'))
 for block=0,0x1ff00,0x100 do
  local bytes={};for a=block,block+255 do bytes[#bytes+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
  f:write(table.concat(bytes))
 end;f:close()
 log:write(string.format('{"index":%d,"frame":%d,"court":%d,"tag":"%s","pc":%d,"raw":"%s","a":%d,"x":%d,"y":%d,"sp":%d,"d":%d,"dbr":%d,"ps":%d,"cycle":%d}\n',index,frame,court,tag,pc,filename,s['cpu.a'],s['cpu.x'],s['cpu.y'],s['cpu.sp'],s['cpu.d'],s['cpu.dbr'],s['cpu.ps'],s['cpu.cycleCount']))
 log:flush()
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0;check(w(0x166d)==1,'CPU selection1 required')end end)
for _,item in ipairs({{0x86da89,'init.caller'},{0x80fbe9,'init.entry'},{0x80fbfe,'init.terminal'},{0x86da8d,'init.return'}})do
 local pc,tag=item[1],item[2]
 hook(pc,function()if init<4 then snap(tag,pc);init=init+1 end end)
end
for _,item in ipairs({{0x86dbc2,'basket.before'},{0x86dbc5,'basket.after'}})do
 local pc,tag=item[1],item[2]
 hook(pc,function()if basket<2 then snap(tag,pc);basket=basket+1 end end)
end
hook(0x87a3b1,function()
 if court>=240 and passes<12 then check(not active,'nested projection');active=true;snap('depth.before',0x87a3b1)end
end)
for _,item in ipairs({{0x87a43e,'pass.caller'},{0x80fc80,'pass.entry'},{0x80fca1,'pass.terminal'},{0x87a442,'pass.return'}})do
 local pc,tag=item[1],item[2]
 hook(pc,function()
  if not active then return end;snap(tag,pc)
  if pc==0x87a442 then
   active=false;passes=passes+1
   if passes==12 then
    check(init==4 and basket==2,'missing normal init evidence');log:close()
    local f=assert(io.open(out..'/capture_complete.txt','wb'))
    f:write(string.format('frames=%d\nboundaries=%d\npasses=%d\n',frame,index,passes));f:close();emu.stop(0)
   end
  end
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
 frame=frame+1;check(frame<12000,'normal draw capture timeout')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then court=court+1 end
end,emu.eventType.endFrame)
