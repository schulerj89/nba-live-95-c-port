-- Normal cold-reset/source initializer observations; no machine-state writes.
local out=assert(os.getenv('NBA95_SCHEDULER_DIR'))
local files={};for _,n in ipairs({'instructions','bus','boundaries','callers','interrupts','upload'})do files[n]=assert(io.open(out..'/sound_init_'..n..'.jsonl','wb'))end
local counts={instructions=0,bus=0,boundaries=0,callers=0,interrupts=0,upload=0}
local call,scope,lastpc=0,0,0
local active,in_nmi,resume,upload,closed=false,false,false,false,false
local function write(name,r)
 r.event=counts[name];counts[name]=counts[name]+1
 local keys={};for k in pairs(r)do keys[#keys+1]=k end;table.sort(keys)
 local fields={};for _,k in ipairs(keys)do local v=r[k];fields[#fields+1]='"'..k..'":'..(type(v)=='string'and('"'..v..'"')or tostring(v))end
 files[name]:write('{'..table.concat(fields,',')..'}\n')
end
local function state(pc)
 local s=emu.getState();return{pc=pc,call=call,cpu_cycles=s['cpu.cycleCount'],master_clock=s.masterClock,
 a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],ps=s['cpu.ps'],db=s['cpu.dbr'],dp=s['cpu.d'],sp=s['cpu.sp']}
end
local function dump(name,memory,size)
 local f=assert(io.open(out..'/'..name,'wb'))
 for base=0,size-1,1024 do local bytes={}
  for i=base,math.min(size-1,base+1023)do bytes[#bytes+1]=string.char(emu.read(i,memory,false))end
  f:write(table.concat(bytes))
 end;f:close()
end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
emu.addMemoryCallback(function(pc)
 if resume then in_nmi=false;resume=false end
 if active and not in_nmi then lastpc=pc;write('instructions',state(pc))end
end,emu.callbackType.exec,0x808000,0x80ffff,emu.cpuType.snes,emu.memType.snesMemory)
for _,pc in ipairs({0x80814a,0x82abf2,0x82ad48})do hook(pc,function()
 if closed then return end
 local r=state(pc);r.call=call+1;r.tag='init.caller';write('callers',r)
end)end
hook(0x809b73,function()
 if closed then return end;assert(not active);active=true;call=call+1;lastpc=0x809b73
 local r=state(lastpc);r.tag='init.entry';write('boundaries',r)
 dump(string.format('sound_init_%02d_entry.wram',call),emu.memType.snesWorkRam,0x20000)
 write('instructions',state(lastpc))
end)
hook(0x80815a,function()if active then in_nmi=true;local r=state(0x80815a);r.tag='nmi.entry';write('interrupts',r)end end)
hook(0x80859b,function()if active and in_nmi then local r=state(0x80859b);r.tag='nmi.exit';write('interrupts',r);resume=true end end)
for _,bank in ipairs({0,0x80,0x82})do
 for _,kind in ipairs({'read','write'})do
  emu.addMemoryCallback(function(address,value)
   if not active or in_nmi then return end
   local r=state(lastpc);r.kind=kind;r.address=address;r.value=value;write('bus',r)
   if kind=='read'and(address&0xffff)==0x2140 then
    r=state(lastpc);r.tag='init.spc_read';write('boundaries',r)
    dump(string.format('sound_init_%02d_exit.wram',call),emu.memType.snesWorkRam,0x20000);active=false
   end
  end,kind=='read'and emu.callbackType.read or emu.callbackType.write,bank<<16,(bank<<16)|0xffff,emu.cpuType.snes,emu.memType.snesMemory)
 end
end
hook(0x80ab06,function()
 if closed then return end;assert(not upload);upload=true
 local r=state(0x80ab06);r.tag='upload.entry';r.source=emu.read(0x0c,emu.memType.snesWorkRam,false)|(emu.read(0x0d,emu.memType.snesWorkRam,false)<<8)|(emu.read(0x0e,emu.memType.snesWorkRam,false)<<16);write('upload',r)
end)
hook(0x80ab7d,function()
 if not upload then return end;upload=false
 local r=state(0x80ab7d);r.tag='upload.exit';write('upload',r)
 dump('sound_init_uploaded.spc',emu.memType.spcRam,0x10000)
end)
hook(0x81cf62,function()if scope<4 then scope=scope+1 end end)
hook(0x81ba8e,function()if scope>0 and scope<4 then scope=scope+1 end end)
hook(0x80eec6,function()
 if scope~=4 or closed then return end;closed=true;assert(not active and not upload)
 for _,f in pairs(files)do f:close()end
 local f=assert(io.open(out..'/sound_init_complete.txt','wb'))
 f:write(string.format('ok; calls=%d; instructions=%d; bus=%d; boundaries=%d; callers=%d; interrupts=%d; upload=%d\n',call,counts.instructions,counts.bus,counts.boundaries,counts.callers,counts.interrupts,counts.upload));f:close()
end)
dofile(out..'/sound_prefix_base.lua')
