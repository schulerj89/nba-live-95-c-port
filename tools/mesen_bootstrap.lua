-- Original cold power-on, no inputs, ROM patches, RAM/register/port writes.
local out=assert(os.getenv('NBA95_BOOTSTRAP_DIR'))
local files={};local counts={};local cpu_pc=0;local spc_pc=0;local done=false
for _,n in ipairs({'cpu','spc','cpu_bus','spc_bus','boundaries'})do files[n]=assert(io.open(out..'/'..n..'.jsonl','wb'));counts[n]=0 end
local function row(name,r)
 r.event=counts[name];counts[name]=counts[name]+1;local keys={};for k in pairs(r)do keys[#keys+1]=k end;table.sort(keys)
 local a={};for _,k in ipairs(keys)do local v=r[k];a[#a+1]='"'..k..'":'..(type(v)=='string'and('"'..v..'"')or tostring(v))end
 files[name]:write('{'..table.concat(a,',')..'}\n')
end
local function snapshot(tag)
 local s=emu.getState();local f=assert(io.open(out..'/'..tag..'.state','wb'));local keys={}
 for k,v in pairs(s)do if type(v)~='table'then keys[#keys+1]=k end end;table.sort(keys)
 for _,k in ipairs(keys)do f:write(k..'='..tostring(s[k])..'\n')end;f:close()
 for _,spec in ipairs({{'aram',emu.memType.spcRam,65536},{'wram',emu.memType.snesWorkRam,131072}})do
  f=assert(io.open(out..'/'..tag..'.'..spec[1],'wb'))
  for base=0,spec[3]-1,1024 do local b={};for i=base,base+1023 do b[#b+1]=string.char(emu.read(i,spec[2],false))end;f:write(table.concat(b))end;f:close()
 end
 row('boundaries',{tag=tag,master=s.masterClock,cpu=s['cpu.cycleCount'],spc=s['spc.cycle'],cpu_pc=cpu_pc,spc_pc=spc_pc})
end
local function finish()
 done=true;for _,f in pairs(files)do f:close()end
 local f=assert(io.open(out..'/complete.txt','wb'));f:write('ok; normal power-on through actual post-F1 callback\n');f:close();emu.stop(0)
end
emu.addMemoryCallback(function(pc)
 if done then return end;cpu_pc=pc;local s=emu.getState()
 row('cpu',{pc=pc,master=s.masterClock,cycles=s['cpu.cycleCount'],a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],sp=s['cpu.sp'],ps=s['cpu.ps'],db=s['cpu.dbr'],dp=s['cpu.d'],emulation=s['cpu.emulationMode']})
 if pc==0x800d then snapshot('power_entry')end
 if pc==0x8080bc then snapshot('cpu_80bc')end
 if pc==0x808a8d then
  snapshot('cpu_dma_request')
 end
 assert(counts.cpu<50000,'CPU bound')
end,emu.callbackType.exec,0,0xffffff,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function(pc)
 if done then return end;spc_pc=pc;local s=emu.getState()
 row('spc',{pc=pc,master=s.masterClock,cycles=s['spc.cycle'],a=s['spc.a'],x=s['spc.x'],y=s['spc.y'],sp=s['spc.sp'],ps=s['spc.ps']})
 if pc==0x380 then snapshot('resident_entry')end
 if pc==0x387 then snapshot('post_f1');finish()end
 assert(counts.spc<50000,'SPC bound')
end,emu.callbackType.exec,0,65535,emu.cpuType.spc,emu.memType.spcMemory)
for _,kind in ipairs({'read','write'})do
 emu.addMemoryCallback(function(a,v)
  if done then return end;local s=emu.getState();row('cpu_bus',{pc=cpu_pc,address=a,value=v,kind=kind,master=s.masterClock,cycles=s['cpu.cycleCount']})
 end,kind=='read'and emu.callbackType.read or emu.callbackType.write,0,0xffffff,emu.cpuType.snes,emu.memType.snesMemory)
 emu.addMemoryCallback(function(a,v)
  if done then return end;local s=emu.getState();row('spc_bus',{pc=spc_pc,address=a,value=v,kind=kind,master=s.masterClock,cycles=s['spc.cycle']})
 end,kind=='read'and emu.callbackType.read or emu.callbackType.write,0,65535,emu.cpuType.spc,emu.memType.spcMemory)
end
local f=assert(io.open(out..'/environment.txt','wb'));f:write('output='..out..'\nhome='..tostring(emu.getScriptDataFolder())..'\n');f:close()
