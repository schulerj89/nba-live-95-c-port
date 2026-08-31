-- Natural human99C4 launches and arithmetic/NMI boundaries. No state/PC writes.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local selection=assert(tonumber(os.getenv('NBA95_PASS_LAUNCH_SELECTION')))
local stop_at=assert(tonumber(os.getenv('NBA95_PASS_LAUNCH_FRAMES')))
assert(selection==0 or selection==2)
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local log=assert(io.open(out..'/boundaries.jsonl','wb'))
local title,setup,player,court=-1,-1,-1,-1
local frame,index,passes,calls,origin,component,nmi_depth=0,0,0,0,0,0,0
local human=nil;local passing=false;local mode=false;local active=false
local origins={};local prefix={};local math_kind=nil;local math_sp=0
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function check(test,message)
 if test then return end
 local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
 log:flush();emu.stop(1);error(message)
end
local function capture(tag,pc)
 local s=emu.getState();local r={tag=tag,pc=pc,frame=frame,court=court,parts={},origin=origin,call=calls,component=component,nmi_depth=nmi_depth}
 for _,key in ipairs({'a','x','y','ps','d','sp','dbr','k','pc'})do r['cpu_'..key]=assert(s['cpu.'..key],key)end
 for _,part in ipairs({{0,0x2000},{0x3400,0x1600}})do
  local b={};for a=part[1],part[1]+part[2]-1 do b[#b+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
  r.parts[#r.parts+1]=table.concat(b)
 end
 r.actor=w(0xc2);r.actor_pointer=w(0x96);r.owner=w(0x93e);r.live=w(0x936);r.offense=w(0x93a)
 r.stack={};check(r.cpu_sp<0x2000,'stack outside captured low WRAM')
 for n=1,math.min(23,0x1fff-r.cpu_sp)do r.stack[n]=emu.read(r.cpu_sp+n,emu.memType.snesMemory)end
 return r
end
local function emit(r)
 index=index+1;local name=string.format('raw_%05d.bin',index)
 local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(r.parts));f:close()
 log:write(string.format('{"index":%d,"tag":"%s","pc":%d,"frame":%d,"court":%d,"raw":"%s",',index,r.tag,r.pc,r.frame,r.court,name))
 for _,key in ipairs({'origin','call','component','nmi_depth','cpu_a','cpu_x','cpu_y','cpu_ps','cpu_d','cpu_sp','cpu_dbr','cpu_k','cpu_pc','actor','actor_pointer','owner','live','offense'})do log:write(string.format('"%s":%d,',key,r[key]))end
 log:write('"stack":['..table.concat(r.stack,',')..']}\n');log:flush()
end
local function snap(tag,pc)emit(capture(tag,pc))end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0;snap('player.entry',0x81a489)end end)
hook(0x87a47a,function()if court<0 then court=0;snap('court.entry',0x87a47a);check(w(0x166d)==selection,'wrong selection')end end)
hook(0x84e2ac,function()if court>=0 then human=(w(0xae)&0x8000)~=0 and capture('human.entry',0x84e2ac)or nil end end)
hook(0x84df7a,function()if court>=0 and human then
 check(not passing and not active,'nested human pass');passes=passes+1;origin=passes
 human.origin=origin;emit(human);human=nil;passing=true;snap('pass.entry',0x84df7a)
end end)
hook(0x86af4d,function()if passing then
 check(w(w(0x96)+0x5e)==15,'initializer failed mode15');origins[w(0x96)]=origin;snap('init.return',0x86af4d)
end end)
hook(0x8791c3,function()if passing then snap('human.return',0x8791c3);passing=false;origin=0 end;human=nil end)
hook(0x879244,function()if court>=0 and origins[w(0x96)]and w(w(0x96)+0x5e)==15 then
 check(not mode and not active,'nested mode15');mode=true;origin=origins[w(0x96)];component=0
 prefix={capture('dispatch.entry',0x879244)}
end end)
for _,v in ipairs({{'dispatch.call',0x879258},{'wrapper.entry',0x879c53},{'mode.entry',0x86a6b3}})do
 local tag,pc=v[1],v[2];hook(pc,function()if mode and not active then prefix[#prefix+1]=capture(tag,pc)end end)
end
hook(0x8699c4,function()if mode then
 check(not active and #prefix==4,'launch missing native dispatch');active=true;calls=calls+1
 for _,r in ipairs(prefix)do r.call=calls;emit(r)end;prefix={};snap('launch.entry',0x8699c4)
end end)
hook(0x85f78b,function()if active then
 check(not math_kind,'nested signed arithmetic');math_kind='mul';component=component+1
 math_sp=assert(emu.getState()['cpu.sp']);snap('mul.entry',0x85f78b)
end end)
for _,v in ipairs({{'mul.exit.signed',0x85f7ae},{'mul.exit.unsigned',0x85f820}})do
 local tag,pc=v[1],v[2];hook(pc,function()if active and math_kind=='mul'and emu.getState()['cpu.sp']==math_sp then
  snap(tag,pc);math_kind=nil
 end end)
end
hook(0x85f8d9,function()if active then
 check(not math_kind,'nested signed arithmetic');math_kind='divide';component=component+1
 math_sp=assert(emu.getState()['cpu.sp']);snap('divide.entry',0x85f8d9)
end end)
hook(0x85f928,function()if active then
 check(math_kind=='divide'and emu.getState()['cpu.sp']==math_sp,'wrong divide return');snap('divide.exit',0x85f928);math_kind=nil
end end)
for _,v in ipairs({{'prediction.ready',0x869a8b},{'clamp.x.upper',0x869bb1},{'clamp.x.lower',0x869bb8},
 {'clamp.y.upper',0x869bfb},{'clamp.y.lower',0x869c02},{'target.ready',0x869aaf},{'velocity.ready',0x869b13},
 {'launch.restore',0x869b92},{'launch.exit',0x869bb0},{'launch.return',0x86a75f},{'wrapper.exit',0x879c57}})do
 local tag,pc=v[1],v[2];hook(pc,function()if active then snap(tag,pc)end end)
end
hook(0x80815a,function()if active then nmi_depth=nmi_depth+1;snap('nmi.entry',0x80815a)end end)
for _,v in ipairs({{'nmi.exit.reentrant',0x808171},{'nmi.exit.normal',0x80859b}})do
 local tag,pc=v[1],v[2];hook(pc,function()if active and nmi_depth>0 then snap(tag,pc);nmi_depth=nmi_depth-1 end end)
end
hook(0x87925c,function()if mode then
 if active then check(not math_kind and nmi_depth==0,'unfinished launch child');snap('mode.return',0x87925c)end
 if w(w(0x96)+0x5e)~=15 then origins[w(0x96)]=nil end
 mode=false;active=false;origin=0;component=0;prefix={}
end end)
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
 local input={}
 if court>=0 then
  if court>=120 then
   local block=(court-120)//120;local n=(court-120)%120;local d=block%9
   if n>=10 and n<50 then
    input.up=d==0 or d==1 or d==7;input.right=d==1 or d==2 or d==3
    input.down=d==3 or d==4 or d==5;input.left=d==5 or d==6 or d==7
   end
   input.b=pulse(n,30)or(n>=80 and n<112)
  end
 elseif player>=0 then input.left=selection==0 and(pulse(player,400)or pulse(player,460));input.start=player>=700 and(player-700)%200<3
 elseif setup>=0 then input.start=pulse(setup,400)or(setup>=650 and(setup-650)%200<3)
 else input.start=pulse(title,850)end
 emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
 frame=frame+1;check(frame<18000,'normal journey did not complete')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then court=court+1;if court>=stop_at and not active and not passing then
  log:close();local f=assert(io.open(out..'/capture_complete.txt','wb'))
  f:write(string.format('selection=%d\nframes=%d\nboundaries=%d\ncalls=%d\npasses=%d\n',selection,court,index,calls,passes));f:close();emu.stop(0)
 end end
end,emu.eventType.endFrame)
