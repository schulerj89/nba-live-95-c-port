-- Original execution, normal controller inputs only. No state/ROM patches.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local selection=assert(tonumber(os.getenv('NBA95_PASS_CATCH_SELECTION')))
local stop_at=assert(tonumber(os.getenv('NBA95_PASS_CATCH_FRAMES')))
assert(selection==0 or selection==2)
local h=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
h:write(emu.getScriptDataFolder()..'\n');h:close()
local log=assert(io.open(out..'/boundaries.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local index,active,calls,metric=0,false,0,false
local prefix,geometry,cancel=false,false,false
local action,upper=false,false
local aligned,lane,aligned_upper=false,false,false
local pose=false
local catch=false;local catch_rng,catch_direction,catch_lane=false,false,false
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function check(test,message)
 if test then return end
 local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
 log:flush();emu.stop(1);error(message)
end
local function snap(tag,pc,raw)
 index=index+1;local s=emu.getState();local file=''
 local indirect_addr,indirect_word=0,0
 if tag=='catch.rating'then
  indirect_addr=((w(0)|((w(2)&255)<<16))+0x42)&0xffffff
  indirect_word=emu.read(indirect_addr,emu.memType.snesMemory)|(emu.read((indirect_addr+1)&0xffffff,emu.memType.snesMemory)<<8)
 end
 if raw then
  file=string.format('raw_%05d.bin',index)
  local f=assert(io.open(out..'/'..file,'wb'))
  for _,r in ipairs({{0,0x100},{0x500,0x500},{0x1600,0x300},{0x3400,0x1600}})do
   local b={};for a=r[1],r[1]+r[2]-1 do b[#b+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
   f:write(table.concat(b))
  end;f:close()
 end
 log:write(string.format('{"index":%d,"tag":"%s","pc":%d,"frame":%d,"court":%d,"raw":"%s","indirect_addr":%d,"indirect_word":%d,"cpu_d":%d,"cpu_x":%d,"cpu_y":%d,"cpu_ps":%d,"actor":%d,"owner":%d,"live":%d,"offense":%d,"direction":%d,"candidate":%d,"score":%d}\n',
  index,tag,pc,frame,court,file,indirect_addr,indirect_word,s['cpu.d']or 0,s['cpu.x']or 0,s['cpu.y']or 0,s['cpu.ps']or 0,w(0xc2),w(0x93e),w(0x936),w(0x93a),w(0x90c)>=0x47eb and w(w(0x90c)+6)or 65535,w(0x92),w(0xaa)))
 log:flush()
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0;snap('player.entry',0x81a489,true)end end)
hook(0x87a47a,function()if court<0 then court=0;snap('court.entry',0x87a47a,true);check(w(0x166d)==selection,'wrong native selection')end end)
hook(0x84df7a,function()
 if court<0 then return end
 check(not active,'nested pass');active='selection';calls=calls+1;snap('pass.entry',0x84df7a,true)
end)
for _,t in ipairs({{0x84dfb2,'pass.directional'},{0x84e0b5,'pass.neutral'},
 {0x84e016,'candidate.directional'},{0x84e11e,'candidate.neutral'}})do
 hook(t[1],function()if active=='selection' then snap(t[2],t[1],false)end end)
end
hook(0x84e098,function()
 if active then check(active=='selection'and not metric,'bad pass callsite');snap('pass.callsite',0x84e098,false);active='waitingchild'end
end)
hook(0x86ab2d,function()
 if active=='waitingchild'then snap('pass.initialize',0x86ab2d,true);active='child';prefix=false;geometry=false end
end)
hook(0x87b538,function()
 if active=='child'and not prefix then check(not cancel,'nested cancellation');cancel=true;snap('init.cancel.entry',0x87b538,true)end
end)
hook(0x87b554,function()
 if cancel then snap('init.cancel.exit',0x87b554,true);cancel=false end
end)
hook(0x86ab83,function()
 if active=='child'then check(not prefix and not cancel,'misordered prefix');snap('init.prefix',0x86ab83,true);prefix=true end
end)
hook(0x86abe9,function()
 if active=='child'then check(prefix and not geometry,'misordered geometry');snap('init.geometry.entry',0x86abe9,true)end
end)
hook(0x86abed,function()
 if active=='child'then check(prefix and not geometry,'misordered geometry return');snap('init.geometry.exit',0x86abed,true)end
end)
hook(0x86ac50,function()
 if active=='child'then
  check(prefix,'missing prefix');snap(geometry and'init.revisit'or'init.ready',0x86ac50,not geometry)
  if not geometry then action='gate' end;geometry=true
 end
end)
hook(0x86ad0e,function()
 if action=='gate'then snap('action.offaxis',0x86ad0e,true);action=false;aligned=true;snap('aligned.entry',0x86ad0e,true)end
end)
hook(0x86aca9,function()
 if action=='gate'then snap('action.normal',0x86aca9,true);action=false end
end)
hook(0x86afc4,function()
 if action=='gate'then snap('action.boost',0x86afc4,true);action=false end
end)
hook(0x86b00b,function()
 if action=='gate'then snap('action.ground.entry',0x86b00b,true);action='ground' end
end)
hook(0x87b47a,function()
 if action=='ground'then check(not upper,'nested upper action');snap('action.upper.entry',0x87b47a,true);upper=true end
end)
hook(0x87b4da,function()
 if upper then snap('action.upper.exit',0x87b4da,true);upper=false end
end)
hook(0x86b04b,function()
 if action=='ground'then check(not upper,'missing upper return');snap('action.ground.exit',0x86b04b,true);action='ground-return' end
end)
hook(0x86af1d,function()
 if action=='ground-return'then snap('action.ready',0x86af1d,true);action=false end
end)
hook(0x86ad3d,function()
 if aligned then snap('aligned.catch',0x86ad3d,true);aligned=false end
end)
hook(0x86ae10,function()if aligned then snap('aligned.choice.entry',0x86ae10,true)end end)
hook(0x86ae52,function()if aligned then snap('aligned.aligned',0x86ae52,true)end end)
hook(0x85f473,function()
 if aligned then check(not lane,'nested lane');snap('aligned.lane.entry',0x85f473,true);lane=true end
end)
hook(0x85f5e3,function()
 if lane then snap('aligned.lane.exit',0x85f5e3,true);lane=false end
end)
hook(0x86aed9,function()if aligned then check(not lane,'missing lane return');snap('aligned.chosen',0x86aed9,true)end end)
hook(0x87b47a,function()
 if aligned then check(not aligned_upper,'nested aligned upper');snap('aligned.upper.entry',0x87b47a,true);aligned_upper=true end
end)
for _,pc in ipairs({0x87b4c0,0x87b4ce,0x87b4da})do
 hook(pc,function()if aligned_upper then snap('aligned.upper.exit',pc,true);aligned_upper=false end end)
end
hook(0x87b3bd,function()
 if aligned then snap('aligned.both',0x87b3bd,true);aligned=false end
end)
hook(0x86af30,function()
 if aligned then snap('aligned.commit',0x86af30,true);aligned=false end
end)
hook(0x86af1d,function()
 if aligned then check(not aligned_upper and not lane,'unfinished aligned child');snap('aligned.ready',0x86af1d,true);aligned=false end
end)
-- This continuation is reached independently from any earlier pass family.
-- Observe only the actual AF1D caller; no forced owner/state/PC or expected seed.
hook(0x86af1d,function()
 if active=='child'then check(not pose,'nested pose caller');snap('pose.entry',0x86af1d,true);pose='resolve-wait'end
end)
hook(0x87aec3,function()
 if pose then check(pose=='resolve-wait','misordered pose resolver');snap('pose.resolve.entry',0x87aec3,true);pose='resolve'end
end)
hook(0x87af74,function()
 if pose=='resolve'then snap('pose.resolve.exit',0x87af74,true);pose='attach-wait'end
end)
hook(0x87b649,function()
 if pose then check(pose=='attach-wait','misordered attachment');snap('pose.attach.entry',0x87b649,true);pose='offset-wait'end
end)
hook(0x87b832,function()
 if pose then check(pose=='offset-wait','misordered attachment offset');snap('pose.offset.entry',0x87b832,true);pose='offset'end
end)
for _,pc in ipairs({0x87b8eb,0x87b952})do
 hook(pc,function()if pose=='offset'then snap('pose.offset.exit',pc,true);pose='attach-return'end end)
end
hook(0x87b669,function()
 if pose then check(pose=='attach-return','misordered attachment return');snap('pose.attach.exit',0x87b669,true);pose='commit-wait'end
end)
hook(0x86af30,function()
 if pose then check(pose=='commit-wait','misordered state commit');snap('pose.commit',0x86af30,true);pose='commit'end
end)
hook(0x86af4d,function()
 if pose then check(pose=='commit','misordered stack continuation');snap('pose.ready',0x86af4d,true);pose=false end
end)
hook(0x86ad3d,function()
 if active=='child'then check(not catch,'nested catch');catch='prefix';snap('catch.entry',0x86ad3d,true)end
end)
hook(0x86ad83,function()if catch then snap('catch.rating',0x86ad83,true)end end)
hook(0x86ad98,function()if catch then snap('catch.geometry',0x86ad98,true)end end)
hook(0x86ada3,function()if catch then snap('catch.attempt',0x86ada3,true)end end)
hook(0x80cee7,function()
 if catch=='prefix'then check(not catch_rng,'nested catch RNG');snap('catch.rng.entry',0x80cee7,true);catch_rng=true end
end)
for _,pc in ipairs({0x80cef5,0x80cefc})do
 hook(pc,function()if catch_rng then snap('catch.rng.exit',pc,true);catch_rng=false end end)
end
hook(0x85f02d,function()
 if catch=='prefix'then check(not catch_direction,'nested catch direction');snap('catch.direction.entry',0x85f02d,true);catch_direction=true end
end)
for _,pc in ipairs({0x85f092,0x85f099})do
 hook(pc,function()if catch_direction then snap('catch.direction.exit',pc,true);catch_direction=false end end)
end
hook(0x85f5e4,function()
 if catch=='prefix'then check(not catch_lane,'nested catch lane');snap('catch.lane.entry',0x85f5e4,true);catch_lane=true end
end)
hook(0x85f727,function()if catch_lane then snap('catch.lane.exit',0x85f727,true);catch_lane=false end end)
hook(0x86af66,function()
 if catch=='prefix'then check(not catch_rng and not catch_direction and not catch_lane,'unfinished catch child');snap('catch.receiver',0x86af66,true);catch='receiver'end
end)
hook(0x86b468,function()
 if catch=='receiver'then snap('catch.receiver.child',0x86b468,true);catch=false end
end)
hook(0x86ae10,function()
 if catch=='prefix'then check(not catch_rng and not catch_direction and not catch_lane,'unfinished catch child');snap('catch.ready',0x86ae10,true);catch=false end
end)
hook(0x84e09c,function()
 if active then
  check(not catch and not catch_rng and not catch_direction and not catch_lane and not pose and not aligned and not lane and not aligned_upper and not action and not upper and not metric and not cancel and(active=='selection'or(active=='child'and prefix and geometry)),'bad pass resume')
  snap(active=='selection'and'pass.no_receiver'or'pass.resume',0x84e09c,true);active='restoring'
 end
end)
hook(0x84e0b4,function()
 if active then check(active=='restoring','bad pass return');snap('pass.return',0x84e0b4,true);active=false end
end)
hook(0x85f1c1,function()
 if active=='selection'then check(not metric,'nested metric');metric=true;snap('metric.entry',0x85f1c1,true)end
end)
for _,pc in ipairs({0x85f1f3,0x85f1ff,0x85f21c,0x85f228})do
 hook(pc,function()if metric then snap('metric.exit',pc,true);metric=false end end)
end
-- The other B branch is only observed as route context, not replayed here.
hook(0x84e141,function()if court>=0 then snap('switch.observed',0x84e141,false)end end)
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
  if court>=stop_at and not active and not metric then
   log:close();local f=assert(io.open(out..'/capture_complete.txt','wb'))
   f:write(string.format('selection=%d\nframes=%d\nboundaries=%d\ncalls=%d\n',selection,court,index,calls));f:close();emu.stop(0)
  end
 end
end,emu.eventType.endFrame)
