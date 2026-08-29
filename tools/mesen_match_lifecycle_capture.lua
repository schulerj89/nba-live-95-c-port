-- Controlled period-expiry capture.  This never changes PC, stack, ROM, or
-- code.  It waits for a natural live possession, then seeds only period,
-- remaining clock, and score so the retail dispatcher owns every later write.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local name=assert(os.getenv('NBA95_LIFECYCLE_CASE'))
local cases={
 q1={period=0,left=10,right=8,final=false},
 halftime={period=1,left=10,right=8,final=false},
 regulation_tie={period=3,left=10,right=10,final=false},
 regulation_final={period=3,left=10,right=8,final=true},
}
local case=assert(cases[name],'unknown lifecycle case')
local file=assert(io.open(out..'/events.jsonl','wb'))
local frames,armed,injected,entered,done=0,false,false,false,false
local entered_frame=-1
local title_frame,setup_frame=-1,-1
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local fields={0x926,0x928,0x92c,0x930,0x936,0x93e,0x946,0x9b4,0x9c0,0xa0c,0x13e7,0x4711,0x4715,0x4791,0x4795}
local function snapshot(pc,event)
 local values={};for _,a in ipairs(fields)do values[#values+1]=string.format('"%04x":%d',a,word(a))end
 file:write(string.format('{"case":"%s","frame":%d,"pc":"%06x","event":"%s","state":{%s}}\n',name,frames,pc,event,table.concat(values,',')));file:flush()
end
local function finish(pc,event)
 if done then return end;done=true;snapshot(pc,event);file:close()
 local f=assert(io.open(out..'/capture_complete.txt','wb'));f:write(name..' complete\n');f:close();emu.stop(0)
end

-- Configure the existing reliable Exhibition/CPU-vs-CPU launch route.
hook(0x828553,function()if not armed then put(0x16fb,3);put(0x16fd,18)end end)
hook(0x86e285,function()if not armed then for a=0x166d,0x1675,2 do put(a,1)end end end)
hook(0x87a47a,function()armed=true end)

-- `$85:EDC6` is the verified clock writer.  Seed one tick only after the ROM
-- has naturally established live play and an owner; the helper performs the
-- actual decrement and all dispatch after it is untouched.
hook(0x85edc6,function()
 if armed and not injected and word(0x936)<0x80 and word(0x93e)<10 then
  put(0x926,case.period);put(0x928,1);put(0x4711,case.left);put(0x4791,case.right)
  put(0x9b4,0);put(0x13e7,word(0x13e7)&0xf7ff)
  injected=true;snapshot(0x85edc6,'controlled_seed_before_native_clock_writer')
 end
end)

for _,spec in ipairs({
 {0x8697cd,'expiry_clock_test'},{0x8697e6,'expiry_latch_entry'},
 {0x8697e9,'expiry_latch_store'},{0x878eb2,'dispatcher_gate'},
 {0x878ebc,'dispatcher_owned_ball'},{0x878ec7,'dispatcher_low_ball'},
 {0x878ecf,'dispatcher_resolved_flight'},{0x8795e9,'period_scene_entry'},
 {0x8796fb,'stamina_grant_1000'},{0x879716,'halftime_extra_grant_6000'},
 {0x87974b,'tie_extra_grant_3000'},{0x87976e,'period_increment'},
 {0x87979d,'final_period_store'},{0x8797a0,'postgame_entry'},
 {0x86dd2d,'next_period_clock_select'},{0x86dd44,'next_period_clock_store'},
 {0x86dd47,'next_period_clock_stored'},
})do local pc,event=spec[1],spec[2];hook(pc,function()
 if injected then
  if pc==0x8795e9 and not entered then entered=true;entered_frame=frames end
  snapshot(pc,event)
  if case.final and pc==0x8797a0 then finish(pc,'final_postgame_handoff')end
  if not case.final and pc==0x86dd47 then finish(pc,'next_period_clock_ready')end
 end
end)end

local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
 local input={}
 if not armed then
  if setup_frame<0 then input.start=pulse(title_frame,850)
  elseif pulse(setup_frame,400) then input.start=true
  elseif setup_frame>=650 and (setup_frame-650)%200<3 then input.start=true end
 elseif entered and frames-entered_frame>900 and (frames-entered_frame-900)%180<3 then
  -- The between-period presentation waits for acknowledgement after its
  -- native animation/audio gate.  Pulse Start without changing code/state.
  input.start=true
 end
 emu.setInput(input,0);for pad=1,4 do emu.setInput({},pad)end
end,emu.eventType.inputPolled)
hook(0x80e1b1,function()if title_frame<0 then title_frame=0 end end)
hook(0x80a2bf,function()if title_frame>=850 and setup_frame<0 then setup_frame=0 end end)
emu.addEventCallback(function()
 frames=frames+1
 if title_frame>=0 and setup_frame<0 then title_frame=title_frame+1 end
 if setup_frame>=0 and not armed then
  if setup_frame>=300 and setup_frame<400 then put(0x16fb,0)end
  setup_frame=setup_frame+1
 end
 if injected and frames>30000 then
  snapshot(0,'capture_timeout');file:close();emu.stop(1);error('lifecycle capture timeout')
 end
end,emu.eventType.endFrame)
