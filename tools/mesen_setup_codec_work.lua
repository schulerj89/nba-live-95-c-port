-- New observation only. The frozen scheduler base supplies all natural input.
local out=assert(os.getenv('NBA95_SCHEDULER_DIR'))
local trace=assert(io.open(out..'/codec_instructions.jsonl','wb'))
local events=assert(io.open(out..'/codec_boundaries.jsonl','wb'))
local writes=assert(io.open(out..'/codec_writes.jsonl','wb'))
local backdrop,in_nmi,started=false,false,false
local scope,call,instruction,event,write_event=0,0,0,0,0
local active,selected,last_pc=false,false,0
local seen={}
local function byte(a)return emu.read(a,emu.memType.snesWorkRam,false)end
local function word(a)return byte(a)|(byte(a+1)<<8)end
local function long(a)return word(a)|(byte(a+2)<<16)end
local function state(pc)
 local s=emu.getState()
 return {pc=pc,scope=scope,call=call,master_clock=s.masterClock,cpu_cycles=s['cpu.cycleCount'],
  ppu_frame=s['ppu.frameCount'],scanline=s['ppu.scanline'],hclock=s['memoryManager.hClock'],
  a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],ps=s['cpu.ps'],db=s['cpu.dbr'],dp=s['cpu.d'],sp=s['cpu.sp'],
  source=long(0x0c),destination=long(0x10),mode=word(0x561),head=word(0x35),tail=word(0x37)}
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
local function observe(pc)
 last_pc=pc
 if not selected then return end
 local s=emu.getState()
 write(trace,{pc=pc,call=call,instruction=instruction,cpu_cycles=s['cpu.cycleCount'],master_clock=s.masterClock,
  a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],ps=s['cpu.ps'],db=s['cpu.dbr'],sp=s['cpu.sp']})
 instruction=instruction+1
end
local function dump(stage)
 local f=assert(io.open(out..string.format('/codec_%02d_%s.wram',call,stage),'wb'))
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
 if active and not in_nmi and address~=0x80c682 then observe(address)end
end,emu.callbackType.exec,0x808000,0x80ffff,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function(address,value)
 if not active or not selected or in_nmi then return end
 local s=emu.getState()
 write(writes,{call=call,event=write_event,pc=last_pc,address=address,value=value,
  cpu_cycles=s['cpu.cycleCount'],master_clock=s.masterClock})
 write_event=write_event+1
end,emu.callbackType.write,0,0xffffff,emu.cpuType.snes,emu.memType.snesMemory)
hook(0x80815a,function()in_nmi=true end)
hook(0x80859b,function()in_nmi=false end)
hook(0x81cf62,function()if scope<4 then started=true;scope=scope+1 end end)
hook(0x81ba8e,function()if started and scope<4 then scope=scope+1 end end)
hook(0x80ec68,function()if started and scope<=4 then backdrop=true end end)
hook(0x80eec6,function()
 if not backdrop then return end
 assert(not active);backdrop=false
 if scope==4 then
  trace:close();events:close();writes:close()
  local f=assert(io.open(out..'/codec_complete.txt','wb'))
  f:write(string.format('ok; scopes=4; calls=%d; instructions=%d; writes=%d\n',call,instruction,write_event));f:close()
 end
end)
hook(0x80c62b,function()
 if not backdrop or in_nmi then return end
 assert(not active);active=true;call=call+1
 local src=long(0x0c);selected=not seen[src];seen[src]=true
 boundary('codec.entry',0x80c62b);dump('entry');observe(0x80c62b)
end)
hook(0x80c682,function()
 if not active or in_nmi then return end
 boundary('codec.exit',0x80c682);dump('exit');active=false;selected=false
 trace:flush();events:flush();writes:flush()
end)
dofile(out..'/scheduler_base.lua')
