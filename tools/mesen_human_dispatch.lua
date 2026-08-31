-- Normal pad inputs only. Sparse RAW WRAM, not generated expected C values.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local selection=assert(tonumber(os.getenv('NBA95_HUMAN_SELECTION')))
local stop_at=assert(tonumber(os.getenv('NBA95_HUMAN_FRAMES')))
assert(selection==0 or selection==2)
local h=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
h:write(emu.getScriptDataFolder()..'\n');h:close()
local log=assert(io.open(out..'/boundaries.jsonl','wb'))
local frame,title,setup,player,court=0,-1,-1,-1,-1
local index,gate,motion,action=0,false,false,false
local offense_start=-1
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function require_ok(test,message)
 if test then return end
 local f=assert(io.open(out..'/capture_error.txt','wb'));f:write(message);f:close()
 log:flush();emu.stop(1);error(message)
end
local function snap(tag,pc,raw)
 index=index+1
 local s=emu.getState()
 local file=''
 if raw then
  file=string.format('raw_%05d.bin',index)
  local f=assert(io.open(out..'/'..file,'wb'))
  -- Exact fixed memory map retained in the capture manifest (7936 bytes).
  for _,r in ipairs({{0,0x100},{0x500,0x500},{0x1600,0x300},{0x3400,0x1600}})do
   local b={};for a=r[1],r[1]+r[2]-1 do b[#b+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
   f:write(table.concat(b))
  end
  f:close()
 end
 log:write(string.format('{"index":%d,"tag":"%s","pc":%d,"frame":%d,"court":%d,"raw":"%s","cpu_x":%d,"cpu_y":%d,"cpu_d":%d,"actor":%d,"owner":%d,"group":%d,"offense":%d,"pressed":%d}\n',
  index,tag,pc,frame,court,file,s['cpu.x']or 0,s['cpu.y']or 0,s['cpu.d']or 0,
  w(0xc2),w(0x93e),w(0x96)>=0x34eb and w(w(0x96)+0x6e)or 65535,w(0x93a),w(0xae)))
 log:flush()
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()if title>=850 and setup<0 then setup=0 end end)
hook(0x81a489,function()if player<0 then player=0;snap('player.entry',0x81a489,true)end end)
hook(0x86e208,function()snap('initialize.entry',0x86e208,true)end)
hook(0x86e24b,function()snap('initialize.exit',0x86e24b,true)end)
hook(0x87a47a,function()
 if court<0 then court=0;snap('court.entry',0x87a47a,true);require_ok(w(0x166d)==selection,'wrong normal selection')end
end)
hook(0x879138,function()
 if court<0 then return end
 require_ok(not gate,'unterminated input gate');gate=true;snap('gate.entry',0x879138,true)
end)
hook(0x87915d,function()
 if gate then snap('gate.publish',0x87915d,false);gate=false end
end)
hook(0x84e2ac,function()
 if court<0 then return end
 require_ok(not action,'nested action prefix');action=true;snap('b.entry',0x84e2ac,true)
end)
for _,t in ipairs({{0x84e2e4,'b.pass'},{0x84e2eb,'b.switch'},{0x84e2f2,'b.other'},{0x84e3e6,'b.return'}})do
 hook(t[1],function()if action then snap(t[2],t[1],false);action=false end end)
end
hook(0x8791c3,function()
 if court<0 then return end
 require_ok(not motion,'nested motion stage');motion=true;snap('motion.entry',0x8791c3,true)
end)
hook(0x87922a,function()if motion then snap('motion.accelerate',0x87922a,false)end end)
hook(0x87922e,function()
 if gate then snap('gate.skip',0x87922e,false);gate=false end
 if motion then snap('motion.exit',0x87922e,true);motion=false end
end)
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
 local input={}
 if court>=0 then
  input.right=court>=60 and court<90
  input.up=court>=110 and court<140
  input.b=pulse(court,170)
  input.a=pulse(court,230)
  input.x=pulse(court,260)
  input.y=pulse(court,300)
  -- Observe the original allocation; only button inputs are changed here.
  -- Wait for actual possession by this native controller after the tip,
  -- including inbound possession that needs B to become live, before the
  -- second short input sequence, instead of forcing possession or a savestate.
  if offense_start<0 and court>=400 and w(0x936)~=0x81 and w(0x47ed)==w(0x93e)then
   offense_start=court;snap('offense.inputs.begin',0,false)
  end
  if offense_start>=0 then
   local n=court-offense_start
   input.right=input.right or (n>=10 and n<30)
   input.up=input.up or (n>=40 and n<60)
   input.b=input.b or pulse(n,80)
   input.l=n>=110 and n<130
   input.a=input.a or pulse(n,160)
   input.x=input.x or pulse(n,200)
   input.y=input.y or pulse(n,240)
  end
 elseif player>=0 then
  input.left=selection==0 and (pulse(player,400)or pulse(player,460))
  input.start=player>=700 and (player-700)%200<3
 elseif setup>=0 then input.start=pulse(setup,400)or(setup>=650 and (setup-650)%200<3)
 else input.start=pulse(title,850)end
 emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
 frame=frame+1;require_ok(frame<18000,'normal journey did not complete')
 if title>=0 and setup<0 then title=title+1 end
 if setup>=0 and player<0 then setup=setup+1 end
 if player>=0 and court<0 then player=player+1 end
 if court>=0 then
  court=court+1
  if court==stop_at then
   -- End only between stages. A frame interrupt can occur inside a stage.
   stop_at=stop_at+1
   if not gate and not motion and not action then
    log:close()
    local f=assert(io.open(out..'/capture_complete.txt','wb'))
    f:write(string.format('selection=%d\nframes=%d\nboundaries=%d\noffense_start=%d\n',selection,court,index,offense_start));f:close()
    emu.stop(0)
   end
  end
 end
end,emu.eventType.endFrame)
