-- Actual nested human pass return frames; no state/PC/ROM writes.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local selection=assert(tonumber(os.getenv('NBA95_PASS_RETURN_SELECTION')))
local stop_at=assert(tonumber(os.getenv('NBA95_PASS_RETURN_FRAMES')))
assert(selection==0 or selection==2)
local h=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
h:write(emu.getScriptDataFolder()..'\n');h:close()
local log=assert(io.open(out..'/boundaries.jsonl','wb'))
local title,setup,player,court=-1,-1,-1,-1
local frame,index,calls=0,0,0
local active=false;local human_entry=nil;local child=false
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function check(test,message)
 if test then return end
 local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
 log:flush();emu.stop(1);error(message)
end
local function capture(tag,pc)
 local s=emu.getState();local result={tag=tag,pc=pc,frame=frame,court=court,parts={}}
 for _,key in ipairs({'a','x','y','ps','d','sp','dbr','k','pc'})do result['cpu_'..key]=assert(s['cpu.'..key],key)end
 for _,r in ipairs({{0,0x2000},{0x3400,0x1600}})do
  local b={};for a=r[1],r[1]+r[2]-1 do b[#b+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
  result.parts[#result.parts+1]=table.concat(b)
 end
 result.actor=w(0xc2);result.owner=w(0x93e);result.live=w(0x936);result.offense=w(0x93a)
 result.stack={}
 check(result.cpu_sp<0x2000,'stack outside captured bank0 WRAM')
 for n=1,math.min(19,0x1fff-result.cpu_sp)do result.stack[n]=emu.read(result.cpu_sp+n,emu.memType.snesMemory)end
 return result
end
local function emit(r)
 index=index+1;local name=string.format('raw_%05d.bin',index)
 local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(r.parts));f:close()
 log:write(string.format('{"index":%d,"tag":"%s","pc":%d,"frame":%d,"court":%d,"raw":"%s",',index,r.tag,r.pc,r.frame,r.court,name))
 for _,key in ipairs({'cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_sp','cpu_dbr','cpu_k','cpu_pc','actor','owner','live','offense'})do log:write(string.format('"%s":%d,',key,r[key]))end
 log:write('"stack":['..table.concat(r.stack,',')..']}\n');log:flush()
end
local function snap(tag,pc)emit(capture(tag,pc))end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0;snap('player.entry',0x81a489)end end)
hook(0x87a47a,function()if court<0 then court=0;snap('court.entry',0x87a47a);check(w(0x166d)==selection,'wrong native selection')end end)
hook(0x84e2ac,function()
 if court>=0 then
  check(not active,'nested human pass caller')
  -- Preserve the actual caller prestate only when its incoming B bit is set.
  -- Emit it only if the original owner/gates later choose DF7A naturally.
  human_entry=(w(0xae)&0x8000)~=0 and capture('human.entry',0x84e2ac)or nil
 end
end)
hook(0x84df7a,function()
 if court>=0 then
  check(not active and human_entry,'pass lacks original human frame')
  emit(human_entry);human_entry=nil;active='select';calls=calls+1;child=false;snap('pass.entry',0x84df7a)
 end
end)
hook(0x86ab2d,function()if active then check(active=='select','duplicate initializer');child=true;active='initializer';snap('init.entry',0x86ab2d)end end)
hook(0x86af4d,function()if active then check(active=='initializer','wrong initializer unwind');snap('init.restore',0x86af4d);active='init-pop'end end)
hook(0x86af65,function()if active then check(active=='init-pop','wrong initializer RTL');snap('init.rtl',0x86af65);active='init-return'end end)
hook(0x84e09c,function()
 if active then check(active=='init-return'or active=='select','wrong pass unwind');snap('pass.restore',0x84e09c);active='pass-pop'end
end)
hook(0x84e0b4,function()if active then check(active=='pass-pop','wrong pass RTL');snap('pass.rtl',0x84e0b4);active='pass-return'end end)
hook(0x84e2e8,function()if active then check(active=='pass-return','wrong human B resume');snap('human.resume',0x84e2e8);active='human-jump'end end)
hook(0x84e3e6,function()if active then check(active=='human-jump','wrong human unwind');snap('human.restore',0x84e3e6);active='human-pop'end end)
hook(0x84e3e9,function()
 if active then check(active=='human-pop','wrong human RTL');snap('human.rtl',0x84e3e9);active='human-return'end
 human_entry=nil
end)
hook(0x8791c3,function()if active then check(active=='human-return','wrong gameplay caller return');snap('human.return',0x8791c3);active=false end end)
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
 local input={}
 if court>=0 then
  -- Repeated natural B presses: a brief directed press, then a neutral one.
  -- Eight direction pairs and neutral use the game's ordinary input decoder.
  -- Pass/switch choice remains the native owner/receiver decision.
  if court>=120 then
   local block=(court-120)//120;local n=(court-120)%120;local d=block%9
   if n>=10 and n<50 then
    input.up=d==0 or d==1 or d==7
    input.right=d==1 or d==2 or d==3
    input.down=d==3 or d==4 or d==5
    input.left=d==5 or d==6 or d==7
   end
   input.b=pulse(n,30)or pulse(n,80)
  end
 elseif player>=0 then
  input.left=selection==0 and(pulse(player,400)or pulse(player,460))
  input.start=player>=700 and(player-700)%200<3
 elseif setup>=0 then input.start=pulse(setup,400)or(setup>=650 and(setup-650)%200<3)
 else input.start=pulse(title,850)end
 emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
 frame=frame+1;check(frame<18000,'normal journey did not complete')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then
  court=court+1
  if court>=stop_at and not active then
   log:close();local f=assert(io.open(out..'/capture_complete.txt','wb'))
   f:write(string.format('selection=%d\nframes=%d\nboundaries=%d\ncalls=%d\n',selection,court,index,calls));f:close();emu.stop(0)
  end
 end
end,emu.eventType.endFrame)
