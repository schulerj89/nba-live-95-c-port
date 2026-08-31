-- Read-only source-work capture of NMI during the four natural backdrop
-- constructors. The separate frozen scheduler script supplies all input.
local out=assert(os.getenv('NBA95_SCHEDULER_DIR'))
local trace=assert(io.open(out..'/interrupt_instructions.jsonl','wb'))
local events=assert(io.open(out..'/interrupt_boundaries.jsonl','wb'))
local bus=assert(io.open(out..'/interrupt_bus.jsonl','wb'))
local started,backdrop,in_nmi=false,false,false
local scope,nmi,instruction,event,bus_event=0,0,0,0,0
local function word(a)
 return emu.read(a,emu.memType.snesWorkRam,false)|(emu.read(a+1,emu.memType.snesWorkRam,false)<<8)
end
local function state(pc)
 local s=emu.getState()
 return {pc=pc,scope=scope,nmi=nmi,master_clock=s.masterClock,cpu_cycles=s['cpu.cycleCount'],
  ppu_frame=s['ppu.frameCount'],scanline=s['ppu.scanline'],hclock=s['memoryManager.hClock'],
  a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],ps=s['cpu.ps'],db=s['cpu.dbr'],dp=s['cpu.d'],sp=s['cpu.sp'],
  epoch=word(0x564),audio53=emu.read(0x53,emu.memType.snesWorkRam,false)}
end
local function write(f,v)
 local keys={};for k in pairs(v)do keys[#keys+1]=k end;table.sort(keys)
 local fields={}
 for _,k in ipairs(keys)do
  local value=v[k]
  assert(type(value)=='number' or type(value)=='string')
  fields[#fields+1]='"'..k..'":'..(type(value)=='string' and ('"'..value..'"') or tostring(value))
 end
 f:write('{'..table.concat(fields,',')..'}\n')
end
local function boundary(tag,pc)
 local v=state(pc);v.tag=tag;v.event=event;event=event+1;write(events,v)
end
local function dump(name)
 local f=assert(io.open(out..'/'..name,'wb'))
 for base=0,0x1ffff,1024 do
  local bytes={}
  for a=base,base+1023 do bytes[#bytes+1]=string.char(emu.read(a,emu.memType.snesWorkRam,false))end
  f:write(table.concat(bytes))
 end
 f:close()
end
local function hook(pc,fn)
 emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
-- Register broad observation first: entry is recorded explicitly by its
-- later boundary hook, while the final RTI is observed before phase clears.
emu.addMemoryCallback(function(address)
 if not in_nmi then return end
 local v=state(address);v.instruction=instruction;instruction=instruction+1
 write(trace,v)
end,emu.callbackType.exec,0x808000,0x80ffff,emu.cpuType.snes,emu.memType.snesMemory)
hook(0x81cf62,function()
 if scope>=4 then return end
 started=true;scope=scope+1
 boundary('rules.constructor',0x81cf62)
end)
hook(0x81ba8e,function()
 if not started or scope>=4 then return end
 scope=scope+1;boundary('main.constructor',0x81ba8e)
end)
hook(0x80ec68,function()
 if started and scope<=4 then backdrop=true;boundary('backdrop.entry',0x80ec68)end
end)
hook(0x80eec6,function()
 if not backdrop then return end
 boundary('header.entry',0x80eec6);backdrop=false
 if scope==4 then
  trace:close();events:close();bus:close()
  local f=assert(io.open(out..'/interrupt_complete.txt','wb'))
  f:write(string.format('ok; scopes=4; nmi=%d; instructions=%d; bus=%d\n',nmi,instruction,bus_event));f:close()
 end
end)
hook(0x80815a,function()
 if not backdrop then return end
 assert(not in_nmi,'unexpected nested backdrop NMI')
 in_nmi=true;nmi=nmi+1;boundary('nmi.entry',0x80815a)
 local v=state(0x80815a);v.instruction=instruction;instruction=instruction+1;write(trace,v)
 dump(string.format('interrupt_%02d_entry.wram',nmi))
end)
hook(0x80859b,function()
 if not in_nmi then return end
 boundary('nmi.exit',0x80859b)
 dump(string.format('interrupt_%02d_exit.wram',nmi))
 in_nmi=false;trace:flush();events:flush();bus:flush()
end)
for _,pair in ipairs({{0x8084ab,'epoch.after_guard'},{0x8084c8,'controller.before_call'},
 {0x8084cc,'controller.after_call'},{0x80cb8f,'controller.entry'},
 {0x80cba1,'controller.after_busy_loop'},{0x808553,'callback.before_call'},
 {0x808556,'callback.after_call'},{0x80857a,'audio.before_call'},
 {0x80a137,'audio.entry'},{0x80857d,'audio.after_call'}})do
 hook(pair[1],function()if in_nmi then boundary(pair[2],pair[1])end end)
end
-- Audio helpers execute in PB=$80 with DB=$82, so absolute $2140 reads
-- occur on the $82 mirror. V1 omitted that mirror; do not treat its empty
-- SPC read list as evidence that the audio handshake performs no reads.
for _,bank in ipairs({0,0x80,0x82})do
 for _,range in ipairs({{0x2140,0x2143},{0x4212,0x4212},{0x4218,0x421f},{0x4016,0x4017}})do
  emu.addMemoryCallback(function(address,value)
   if not in_nmi then return end
   local s=emu.getState();local pc=(s['cpu.k']<<16)|s['cpu.pc']
   local v=state(pc);v.event=bus_event;v.address=address;v.value=value;bus_event=bus_event+1
   write(bus,v)
  end,emu.callbackType.read,(bank<<16)|range[1],(bank<<16)|range[2],emu.cpuType.snes,emu.memType.snesMemory)
 end
end
-- Baseline observation/input is copied byte-for-byte and separately attested.
dofile(out..'/scheduler_base.lua')
