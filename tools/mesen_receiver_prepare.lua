-- C2 ordinary controller route. Read-only WRAM/bus/CPU; input only, no seeds.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local selection=assert(tonumber(os.getenv('NBA95_C2_SELECTION')))
local stop_at=assert(tonumber(os.getenv('NBA95_C2_FRAMES')))
assert(selection==0 or selection==2)
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'));home:write(emu.getScriptDataFolder()..'\n');home:close()
local log=assert(io.open(out..'/boundaries.jsonl','wb'))
local writes=assert(io.open(out..'/alternate-writes.jsonl','wb'))
local writers={}
local frame,title,setup,player,court=0,-1,-1,-1,-1
local index,calls,afcalls,humanpasses,nmidepth=0,0,0,0,0
local human=false;local receiver=false;local active=false;local callsp=0
local abbefore=nil;local pose_sp=nil;local accuracy_sp=nil;local divide_sp=nil
local function bus(a)return emu.read(a&0xffffff,emu.memType.snesMemory)end
local function bw(a)return bus(a)|(bus(a+1)<<8)end
local function w(a)return emu.read(a&0x1ffff,emu.memType.snesWorkRam)|(emu.read((a+1)&0x1ffff,emu.memType.snesWorkRam)<<8)end
local function check(ok,message)
 if ok then return end
 local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close();log:flush();emu.stop(1);error(message)
end
local function snapshot(tag,pc)
 local s=emu.getState();local r={tag=tag,pc=pc,frame=frame,court=court,call=calls,afcall=afcalls,human=human and 1 or 0,receiver=receiver and 1 or 0,nmi_depth=nmidepth}
 for _,key in ipairs({'a','x','y','ps','d','sp','dbr','k','pc'})do r['cpu_'..key]=assert(s['cpu.'..key],key)end
 r.master_clock=assert(s.masterClock);r.cpu_cycles=assert(s['cpu.cycleCount']);r.ppu_frame=assert(s['ppu.frameCount']);r.scanline=assert(s['ppu.scanline'])
 local dp=r.cpu_d;local bank=r.cpu_dbr<<16
 r.dp96=w(dp+0x96);r.dp8e=w(dp+0x8e);r.dp9e=w(dp+0x9e);r.dpc2=w(dp+0xc2);r.dpb2=w(dp+0xb2)
 r.e0=w(dp+0xe0)|(emu.read((dp+0xe2)&0x1ffff,emu.memType.snesWorkRam)<<16)
 r.profile_address=(r.e0+0x39)&0xffffff;r.profile_word=bw(r.profile_address)
 r.alternate_address=bank|0x12c;r.alternate_word=bw(r.alternate_address)
 r.statistics_pointer_address=bank|((0x3435+((r.dpc2*2)&0xffff))&0xffff)
 r.statistics_pointer=bw(r.statistics_pointer_address);r.stamina_address=bank|((r.statistics_pointer+0x18)&0xffff);r.stamina_word=bw(r.stamina_address)
 r.rng=w(0x7f6);r.attempt=w(0x904);r.owner=w(0x93e);r.mode=w(r.dp96+0x5e);r.timer=w(r.dp96+0x60)
 for _,offset in ipairs({0x12c,0x12d})do
  local last=writers[offset]
  if last then for k,v in pairs(last)do r[string.format('writer_%04x_%s',offset,k)]=v end end
 end
 local chunks={}
 for base=0,0x1ffff,1024 do local b={};for a=base,base+1023 do b[#b+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end;chunks[#chunks+1]=table.concat(b)end
 r.rawbytes=table.concat(chunks);return r
end
local function emit(r)
 index=index+1;local name=string.format('raw_%05d.bin',index)
 local f=assert(io.open(out..'/'..name,'wb'));f:write(r.rawbytes);f:close()
 log:write(string.format('{"index":%d,"tag":"%s","raw":"%s"',index,r.tag,name))
 local keys={};for k,v in pairs(r)do if type(v)=='number'then keys[#keys+1]=k end end;table.sort(keys)
 for _,k in ipairs(keys)do log:write(string.format(',"%s":%d',k,r[k]))end
 log:write('}\n');log:flush()
end
local function snap(tag,pc)emit(snapshot(tag,pc))end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
emu.addMemoryCallback(function(address,value)
 local s=emu.getState();local pc=(s['cpu.k']<<16)|s['cpu.pc']
 local last={pc=pc,value=value,clock=s.masterClock,cycle=s['cpu.cycleCount'],x=s['cpu.x'],y=s['cpu.y'],d=s['cpu.d'],dbr=s['cpu.dbr'],ps=s['cpu.ps'],sp=s['cpu.sp']}
 -- Mesen supplies a bus address for bank7E writes even with WorkRam filter.
 -- Keep that address in the immutable write log; normalize the lookup only.
 writers[address&0x1ffff]=last
 writes:write(string.format('{"address":%d,"value":%d,"pc":%d,"clock":%d,"cycle":%d,"frame":%d,"court":%d,"x":%d,"y":%d,"d":%d,"dbr":%d,"ps":%d,"sp":%d}\n',address,value,pc,last.clock,last.cycle,frame,court,last.x,last.y,last.d,last.dbr,last.ps,last.sp));writes:flush()
end,emu.callbackType.write,0x12c,0x12d,emu.cpuType.snes,emu.memType.snesWorkRam)
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0 end end)
hook(0x87a47a,function()if court<0 then court=0;check(w(0x166d)==selection,'wrong native selection');snap('court.entry',0x87a47a)end end)
hook(0x84df7a,function()if court>=0 then human=true;humanpasses=humanpasses+1 end end)
hook(0x8791c3,function()human=false end)
hook(0x86ab2d,function()if court>=0 then abbefore=snapshot('initializer.entry',0x86ab2d)end end)
hook(0x86af66,function()if court>=0 then
 check(not active and not receiver,'nested AF66');afcalls=afcalls+1;receiver=true
 check(abbefore~=nil,'AF66 missing initializer');abbefore.afcall=afcalls;emit(abbefore);abbefore=nil;snap('receiver.entry',0x86af66)
end end)
hook(0x86b468,function()if court>=0 then
 calls=calls+1;check(not active,'nested B468');active=receiver or calls<=24
 if active then callsp=assert(emu.getState()['cpu.sp']);snap('child.entry',0x86b468)end
end end)
for _,v in ipairs({{'variant.rng.call',0x86b472},{'variant.rng.return',0x86b476},{'selector.rng.call',0x86b48b},{'selector.rng.return',0x86b48f},{'selector.ready',0x86b4bf},{'pose.return',0x86b4df},{'accuracy.call',0x86b554},{'accuracy.return',0x86b557},{'selector7.call',0x86b56c},{'divide.x.return',0x86b584},{'divide.y.return',0x86b592},{'flags.rng.call',0x86b5e2},{'flags.rng.return',0x86b5e6}})do
 local tag,pc=v[1],v[2];hook(pc,function()if active then snap(tag,pc)end end)
end
hook(0x87b7d8,function()if active then pose_sp=assert(emu.getState()['cpu.sp']);snap('pose.entry',0x87b7d8)end end)
for _,pc in ipairs({0x87b8eb,0x87b952})do hook(pc,function()if active and pose_sp==emu.getState()['cpu.sp']then snap('pose.exit',pc);pose_sp=nil end end)end
hook(0x86aa6a,function()if active then accuracy_sp=assert(emu.getState()['cpu.sp']);snap('accuracy.entry',0x86aa6a)end end)
hook(0x86aae1,function()if active and accuracy_sp then snap('accuracy.rng.call',0x86aae1)end end)
hook(0x86aae5,function()if active and accuracy_sp then snap('accuracy.rng.return',0x86aae5)end end)
hook(0x86ab0c,function()if active and accuracy_sp==emu.getState()['cpu.sp']then snap('accuracy.exit',0x86ab0c);accuracy_sp=nil end end)
hook(0x85f8d9,function()if active then check(not divide_sp,'nested division');divide_sp=assert(emu.getState()['cpu.sp']);snap('divide.entry',0x85f8d9)end end)
hook(0x85f928,function()if active and divide_sp==emu.getState()['cpu.sp']then snap('divide.exit',0x85f928);divide_sp=nil end end)
hook(0x86b624,function()if active then
 check(callsp==emu.getState()['cpu.sp']and not pose_sp and not accuracy_sp and not divide_sp and nmidepth==0,'unfinished child');snap('child.exit',0x86b624);active=false
end end)
hook(0x86af87,function()if receiver then snap('receiver.child.return',0x86af87)end end)
hook(0x86afa3,function()if receiver then snap('receiver.restore',0x86afa3)end end)
hook(0x86ae10,function()if receiver then snap('receiver.continuation',0x86ae10);receiver=false end end)
hook(0x80815a,function()if active then nmidepth=nmidepth+1;snap('nmi.entry',0x80815a)end end)
for _,pc in ipairs({0x808171,0x80859b})do hook(pc,function()if active and nmidepth>0 then snap('nmi.exit',pc);nmidepth=nmidepth-1 end end)end
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
 local input={}
 if court>=0 then
  if court>=120 then
   local block=(court-120)//120;local n=(court-120)%120;local d=block%9
   if n>=10 and n<50 then input.up=d==0 or d==1 or d==7;input.right=d==1 or d==2 or d==3;input.down=d==3 or d==4 or d==5;input.left=d==5 or d==6 or d==7 end
   input.b=pulse(n,30)or(n>=80 and n<112)
  end
 elseif player>=0 then input.left=selection==0 and(pulse(player,400)or pulse(player,460));input.start=player>=700 and(player-700)%200<3
 elseif setup>=0 then input.start=pulse(setup,400)or(setup>=650 and(setup-650)%200<3)
 else input.start=pulse(title,850)end
 emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
 frame=frame+1;check(frame<stop_at+8000,'normal route timeout')
 if title>=0 and setup<0 then title=title+1 end;if setup>=0 and player<0 then setup=setup+1 end;if player>=0 and court<0 then player=player+1 end
 if court>=0 then court=court+1;if court>=stop_at and not active and not receiver then
  log:close();writes:close();local f=assert(io.open(out..'/capture_complete.txt','wb'));f:write(string.format('C2 ordinary controller capture; selection=%d court=%d frames=%d B468=%d AF66=%d humanpasses=%d records=%d\n',selection,court,frame,calls,afcalls,humanpasses,index));f:close();emu.stop(0)
 end end
end,emu.eventType.endFrame)
