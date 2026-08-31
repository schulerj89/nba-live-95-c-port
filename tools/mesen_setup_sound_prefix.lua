-- Read-only isolated-component witnesses, never normal initialization assets.
local out=assert(os.getenv('NBA95_SCHEDULER_DIR'))
local trace=assert(io.open(out..'/sound_prefix_instructions.jsonl','wb'))
local bus=assert(io.open(out..'/sound_prefix_bus.jsonl','wb'))
local bounds=assert(io.open(out..'/sound_prefix_boundaries.jsonl','wb'))
local scope,call,ordinal,event,busevent=0,0,0,0,0
local started,backdrop,active=false,false,false
local lastpc=0
local function write(f,row)
 local keys={};for k in pairs(row)do keys[#keys+1]=k end;table.sort(keys)
 local fields={};for _,k in ipairs(keys)do
  local v=row[k];fields[#fields+1]='"'..k..'":'..(type(v)=='string' and ('"'..v..'"')or tostring(v))
 end;f:write('{'..table.concat(fields,',')..'}\n')
end
local function state(pc)
 local s=emu.getState()
 return {pc=pc,call=call,scope=scope,cpu_cycles=s['cpu.cycleCount'],master_clock=s.masterClock,
  a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],ps=s['cpu.ps'],db=s['cpu.dbr'],dp=s['cpu.d'],sp=s['cpu.sp']}
end
local function dump(stage)
 local f=assert(io.open(out..string.format('/sound_prefix_%02d_%s.wram',call,stage),'wb'))
 for base=0,0x1ffff,1024 do local b={}
  for i=base,base+1023 do b[#b+1]=string.char(emu.read(i,emu.memType.snesWorkRam,false))end
  f:write(table.concat(b))
 end;f:close()
end
local function boundary(tag,pc)
 local r=state(pc);r.tag=tag;r.event=event;event=event+1;write(bounds,r)
end
local function finish(tag,pc)
 boundary(tag,pc);dump('exit');active=false
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
emu.addMemoryCallback(function(pc)
 if active then lastpc=pc;local r=state(pc);r.instruction=ordinal;ordinal=ordinal+1;write(trace,r)end
end,emu.callbackType.exec,0x808000,0x80ffff,emu.cpuType.snes,emu.memType.snesMemory)
hook(0x81cf62,function()if scope<4 then started=true;scope=scope+1 end end)
hook(0x81ba8e,function()if started and scope<4 then scope=scope+1 end end)
hook(0x80ec68,function()if started then backdrop=true end end)
hook(0x80eec6,function()
 if not backdrop then return end;backdrop=false
 if scope==4 then
  assert(not active);trace:close();bus:close();bounds:close()
  local f=assert(io.open(out..'/sound_prefix_complete.txt','wb'))
  f:write(string.format('ok; calls=%d; instructions=%d; bus=%d; boundaries=%d\n',call,ordinal,busevent,event));f:close()
 end
end)
hook(0x80a137,function()
 if not backdrop then return end;assert(not active);call=call+1;active=true;lastpc=0x80a137
 boundary('sound.entry',lastpc);dump('entry');local r=state(lastpc);r.instruction=ordinal;ordinal=ordinal+1;write(trace,r)
end)
hook(0x80a2ce,function()if active then finish('sound.sequencer_unimplemented',0x80a2ce)end end)
for _,bank in ipairs({0,0x80,0x82})do
 for _,kind in ipairs({'read','write'})do
  emu.addMemoryCallback(function(address,value)
   if not active then return end
   local r=state(lastpc);r.kind=kind;r.address=address;r.value=value;r.event=busevent;busevent=busevent+1;write(bus,r)
   if kind=='read' and (address&0xffff)==0x2140 then finish('sound.spc_read',lastpc)end
  end,kind=='read'and emu.callbackType.read or emu.callbackType.write,bank<<16,(bank<<16)|0xffff,emu.cpuType.snes,emu.memType.snesMemory)
 end
end
dofile(out..'/interrupt_base.lua')
