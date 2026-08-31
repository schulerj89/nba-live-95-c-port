-- Read-only residual producer observations around the five separately frozen
-- codec calls. Both preceding observers remain byte-identical copied scripts.
local out=assert(os.getenv('NBA95_SCHEDULER_DIR'))
local trace=assert(io.open(out..'/producer_instructions.jsonl','wb'))
local events=assert(io.open(out..'/producer_boundaries.jsonl','wb'))
local bus=assert(io.open(out..'/producer_bus.jsonl','wb'))
local active,in_codec,in_nmi,started=false,false,false,false
local scope,instruction,event,bus_event,last_pc=0,0,0,0,0
local function byte(a)return emu.read(a,emu.memType.snesWorkRam,false)end
local function word(a)return byte(a)|(byte(a+1)<<8)end
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
local function state(pc)
 local s=emu.getState()
 return {pc=pc,scope=scope,master_clock=s.masterClock,cpu_cycles=s['cpu.cycleCount'],
  a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],ps=s['cpu.ps'],db=s['cpu.dbr'],dp=s['cpu.d'],sp=s['cpu.sp']}
end
local function boundary(tag,pc)
 local v=state(pc);v.tag=tag;v.event=event;event=event+1;write(events,v)
end
local function observe(pc)
 last_pc=pc
 if scope~=1 then return end
 local v=state(pc);v.instruction=instruction;instruction=instruction+1;write(trace,v)
end
local function dump(stage)
 local f=assert(io.open(out..string.format('/producer_%02d_%s.wram',scope,stage),'wb'))
 for base=0,0x1ffff,1024 do
  local bytes={};for a=base,base+1023 do bytes[#bytes+1]=string.char(byte(a))end
  f:write(table.concat(bytes))
 end
 f:close()
end
local function hook(pc,fn)
 emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
emu.addMemoryCallback(function(address)
 if active and not in_codec and not in_nmi and address~=0x80c62b and address~=0x80eec6 then observe(address)end
end,emu.callbackType.exec,0x808000,0x81ffff,emu.cpuType.snes,emu.memType.snesMemory)
local function bus_observe(kind,address,value)
 if not active or in_codec or in_nmi or scope~=1 then return end
 local s=emu.getState();local mask=0
 for channel=0,7 do
  if s[string.format('dmaController.channel[%d].dmaActive',channel)] then mask=mask|(1<<channel)end
 end
 write(bus,{kind=kind,address=address,value=value,pc=last_pc,event=bus_event,scope=scope,dma_active=mask,
  cpu_cycles=s['cpu.cycleCount'],master_clock=s.masterClock})
 bus_event=bus_event+1
end
emu.addMemoryCallback(function(a,v)bus_observe('read',a,v)end,emu.callbackType.read,0,0xffffff,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function(a,v)bus_observe('write',a,v)end,emu.callbackType.write,0,0xffffff,emu.cpuType.snes,emu.memType.snesMemory)
hook(0x80815a,function()in_nmi=true end)
hook(0x80859b,function()in_nmi=false end)
hook(0x81cf62,function()if scope<4 then started=true;scope=scope+1 end end)
hook(0x81ba8e,function()if started and scope<4 then scope=scope+1 end end)
hook(0x80ec68,function()
 if not started or scope>4 then return end
 assert(not active);active=true;boundary('producer.entry',0x80ec68);dump('entry');observe(0x80ec68)
end)
hook(0x80c62b,function()
 if not active or in_nmi then return end
 assert(not in_codec);boundary('codec.entry',0x80c62b);in_codec=true
end)
hook(0x80c682,function()
 if not active or in_nmi then return end
 assert(in_codec);in_codec=false;boundary('codec.exit',0x80c682);observe(0x80c682)
end)
hook(0x80eec6,function()
 if not active then return end
 assert(not in_codec);boundary('producer.exit',0x80eec6);dump('exit');active=false
 trace:flush();events:flush();bus:flush()
 if scope==4 then
  trace:close();events:close();bus:close()
  local f=assert(io.open(out..'/producer_complete.txt','wb'))
  f:write(string.format('ok; scopes=4; boundaries=%d; instructions=%d; bus=%d\n',event,instruction,bus_event));f:close()
 end
end)
dofile(out..'/codec_base.lua')
