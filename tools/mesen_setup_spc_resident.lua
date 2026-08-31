-- Cold reset observations only. No state, memory, port, or register writes.
local out=assert(os.getenv('NBA95_SPC_RESIDENT_DIR'))
local files={};local counts={}
for _,n in ipairs({'instructions','bus','cpu_ports','boundaries'})do files[n]=assert(io.open(out..'/spc_resident_'..n..'.jsonl','wb'));counts[n]=0 end
local function emit(name,r)
 r.event=counts[name];counts[name]=counts[name]+1
 local keys={};for k in pairs(r)do keys[#keys+1]=k end;table.sort(keys)
 local f={};for _,k in ipairs(keys)do local v=r[k];f[#f+1]='"'..k..'":'..(type(v)=='string'and('"'..v..'"')or tostring(v))end
 files[name]:write('{'..table.concat(f,',')..'}\n')
end
local lastpc,commands,init_count=0,0,0
local started,active,finished=false,false,false
local function state(pc)
 local s=emu.getState();local r={pc=pc,spc_cycle=s['spc.cycle'],master_clock=s.masterClock,cpu_cycles=s['cpu.cycleCount'],a=s['spc.a'],x=s['spc.x'],y=s['spc.y'],sp=s['spc.sp'],ps=s['spc.ps']}
 for i=0,3 do r['input'..i]=s['spc.cpuRegs['..i..']'];r['output'..i]=s['spc.outputReg['..i..']'] end
 return r
end
local function snapshot(tag)
 local r=state(lastpc);r.tag=tag;emit('boundaries',r)
 local s=emu.getState();local g=assert(io.open(out..'/spc_resident_'..tag..'.state','wb'));local keys={}
 for k,v in pairs(s)do if k:sub(1,4)=='spc.'and type(v)~='table'then keys[#keys+1]=k end end;table.sort(keys)
 for _,k in ipairs(keys)do g:write(k..'='..tostring(s[k])..'\n')end;g:close()
 g=assert(io.open(out..'/spc_resident_'..tag..'.aram','wb'))
 for base=0,65535,1024 do local b={};for i=base,base+1023 do b[#b+1]=string.char(emu.read(i,emu.memType.spcRam,false))end;g:write(table.concat(b))end;g:close()
end
emu.addMemoryCallback(function(pc)
 if finished then return end;lastpc=pc
 if pc==0x380 and not started then started=true;snapshot('upload_entry')end
 if not started then return end
 if pc==0x447 and not active then active=true;snapshot('poll_entry')end
 if not active then
  init_count=init_count+1
  if init_count<=120 then local r=state(pc);r.tag='init.source';emit('boundaries',r);files.boundaries:flush()end
  return
 end
 if pc==0x613 then commands=commands+1 end
 if pc==0x443 and commands==8 then
  snapshot('end');finished=true
  for _,f in pairs(files)do f:close()end
  local f=assert(io.open(out..'/spc_resident_complete.txt','wb'));f:write('ok; commands=8\n');f:close();emu.stop(0);return
 end
 emit('instructions',state(pc))
 assert(counts.instructions<200000,'bounded capture exceeded')
end,emu.callbackType.exec,0,0xffff,emu.cpuType.spc,emu.memType.spcMemory)
for _,kind in ipairs({'read','write'})do
 emu.addMemoryCallback(function(address,value)
  if not active or finished then return end
  local r=state(lastpc);r.address=address;r.value=value;r.kind=kind;emit('bus',r)
 end,kind=='read'and emu.callbackType.read or emu.callbackType.write,0,0xffff,emu.cpuType.spc,emu.memType.spcMemory)
 for _,bank in ipairs({0,0x80,0x82})do
  emu.addMemoryCallback(function(address,value)
   if not active or finished or (kind=='read'and counts.cpu_ports>=128)then return end
   local r=state(lastpc);r.address=address;r.value=value;r.kind=kind;emit('cpu_ports',r)
  end,kind=='read'and emu.callbackType.read or emu.callbackType.write,(bank<<16)|0x2140,(bank<<16)|0x2143,emu.cpuType.snes,emu.memType.snesMemory)
 end
end
