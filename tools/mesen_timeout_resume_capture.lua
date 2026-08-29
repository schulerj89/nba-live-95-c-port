-- Controlled-menu witness for the native $86:8300-$857B timeout/resume path.
-- Gameplay reaches the pause routine naturally. Tests only seed timeout
-- counters/side at the pause boundary and select an enabled menu entry; no
-- ROM, PC, stack, flags, clock or RNG state is changed.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local case=assert(os.getenv('NBA95_TIMEOUT_CASE')) -- left, right, zero_left, zero_right
local file=assert(io.open(out..'/timeout_resume.json','wb'))
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function fields()
 return string.format('{"live":%d,"clock":%d,"shot_clock":%d,"rng":%d,"left":%d,"right":%d,"side":%d,"selection":%d,"saved_live":%d,"availability":[%d,%d,%d,%d,%d]}',
  w(0x936),w(0x928),w(0x92c),w(0x7f6),w(0x4715),w(0x4795),w(0x8d2),w(0x4981),w(0x4988),
  emu.read(0x4983,emu.memType.snesWorkRam),emu.read(0x4984,emu.memType.snesWorkRam),
  emu.read(0x4985,emu.memType.snesWorkRam),emu.read(0x4986,emu.memType.snesWorkRam),emu.read(0x4987,emu.memType.snesWorkRam))
end
local function stamina()
 local t={};for x=0x5c0,0,-0x40 do t[#t+1]=w(0x4103+x)end
 return '['..table.concat(t,',')..']'
end
local events,entry,menu,grant_before,grant_after,resume={}
local oncourt,playframes,totalframes,paused,menu_visits,input_polls=nil,0,0,false,0,0
local title_frame,setup_frame=-1,-1
local target_right=(case=='right' or case=='zero_right')
local target_zero=(case=='zero_left' or case=='zero_right')
local function event(name,pc,a)
 events[#events+1]=string.format('{"name":"%s","pc":"%06x","a":%d,"state":%s}',name,pc,a or 0,fields())
end
emu.addMemoryCallback(function()if title_frame<0 then title_frame=0 end end,emu.callbackType.exec,0x80e1b1,0x80e1b1,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()if title_frame>=850 and setup_frame<0 then setup_frame=0 end end,emu.callbackType.exec,0x80a2bf,0x80a2bf,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()if not oncourt then oncourt=true end end,emu.callbackType.exec,0x87a47a,0x87a47a,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
 paused=true;entry=fields();put(0x4715,target_zero and not target_right and 0 or 2);put(0x4795,target_zero and target_right and 0 or 3)
 event('pause_entry',0x868300,0)
end,emu.callbackType.exec,0x868300,0x868300,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
 menu_visits=menu_visits+1
 if menu_visits==1 then
  menu=fields()
  if target_zero then put(0x4981,1) else put(0x4981,0) end;input_polls=3
 elseif target_zero and menu_visits==2 then
  event('zero_navigation_landed',0x868369,0);file:write(string.format('{"case":"%s","entry":%s,"menu":%s,"events":[%s]}\n',case,entry,menu,table.concat(events,',')));file:close();local d=assert(io.open(out..'/capture_complete.txt','wb'));d:write('zero rejection captured\n');d:close();emu.stop(0)
 elseif not target_zero and menu_visits==2 then
  put(0x4981,4);input_polls=3
 end
end,emu.callbackType.exec,0x868369,0x868369,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
 put(0x8d2,target_right and 1 or 0);event('timeout_confirm',0x86844e,0)
end,emu.callbackType.exec,0x86844e,0x86844e,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()grant_before=stamina();event('grant_entry',0x868468,0)end,emu.callbackType.exec,0x868468,0x868468,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()grant_after=stamina();event('timeout_fade',0x86849d,0)end,emu.callbackType.exec,0x86849d,0x86849d,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()resume=fields();event('resume_fade',0x86854b,0)end,emu.callbackType.exec,0x86854b,0x86854b,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
 event('resume_return',0x86857b,0)
 file:write(string.format('{"case":"%s","entry":%s,"menu":%s,"grant_before":%s,"grant_after":%s,"resume":%s,"events":[%s]}\n',case,entry,menu,grant_before,grant_after,resume,table.concat(events,',')));file:close()
 local d=assert(io.open(out..'/capture_complete.txt','wb'));d:write('timeout and resume captured\n');d:close();emu.stop(0)
end,emu.callbackType.exec,0x86857b,0x86857b,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
 local s=emu.getState();event('audio_command',0x809df3,s['cpu.a'] or 0)
end,emu.callbackType.exec,0x809df3,0x809df3,emu.cpuType.snes,emu.memType.snesMemory)
emu.addEventCallback(function()
 local input={}
 if setup_frame<0 then
  if title_frame>=850 and title_frame<853 then input.start=true end
 elseif not oncourt and setup_frame>=400 and setup_frame<403 then input.start=true
 elseif not oncourt and setup_frame>=650 and ((setup_frame-650)%200)<3 then input.start=true
 end
 if oncourt then playframes=playframes+1 end
 if oncourt and not paused and playframes>=1000 and (playframes%120)<3 then input.start=true end
 if input_polls>0 then
  if target_zero and menu_visits==1 then input.up=true else input.a=true end
  input_polls=input_polls-1
 end
 emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
 totalframes=totalframes+1
 if title_frame>=0 and setup_frame<0 then title_frame=title_frame+1 end
 if setup_frame>=0 then
  if setup_frame>=300 and setup_frame<400 then put(0x16fb,0) end
  setup_frame=setup_frame+1
 end
 if totalframes>30000 then error('timeout/resume capture did not complete') end
end,emu.eventType.endFrame)
