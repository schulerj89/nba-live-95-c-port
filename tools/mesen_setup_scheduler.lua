-- Natural, controller-only Setup/Rules/reentry scheduler observation.
-- No CPU, RAM, PPU, ROM, save-state or timing mutation is performed.
local out=assert(os.getenv('NBA95_SCHEDULER_DIR'))
local observed=assert(io.open(out..'/observed_environment.txt','wb'))
observed:write('output='..out..'\nhome='..emu.getScriptDataFolder()..'\n')
observed:close()
local file=assert(io.open(out..'/scheduler.jsonl','wb'))
local global_frame,title,setup,normalization=0,-1,-1,-1
local event=0
local rules_ready=0
local header_count=0
local wait_active=false
local wait_owner=0
local in_header=false
local nmi_depth=0
local function word(a)
 return emu.read(a,emu.memType.snesWorkRam,false)|(emu.read(a+1,emu.memType.snesWorkRam,false)<<8)
end
local function active()
 return (setup>=465 and setup<=620) or (setup>=825 and setup<=965) or
        (setup>=1095 and setup<=1250) or (setup>=1455 and setup<=1600)
end
local function dump(name)
 local f=assert(io.open(out..'/'..name,'wb'))
 for base=0,0x1ffff,1024 do
  local bytes={}
  for a=base,base+1023 do bytes[#bytes+1]=string.char(emu.read(a,emu.memType.snesWorkRam,false)) end
  f:write(table.concat(bytes))
 end
 f:close()
end
local function log(tag,pc,extra)
 if not active() then return end
 local st=emu.getState()
 local values={event=event,tag=tag,pc=pc,label=setup,global_frame=global_frame,
  ppu_frame=st['ppu.frameCount'],scanline=st['ppu.scanline'],master_clock=st.masterClock,
  cpu_cycles=st['cpu.cycleCount'],cpu_a=st['cpu.a'],cpu_x=st['cpu.x'],cpu_y=st['cpu.y'],
  cpu_ps=st['cpu.ps'],cpu_d=st['cpu.d'],cpu_sp=st['cpu.sp'],nmi_depth=nmi_depth,
  epoch0564=word(0x564),epoch_block059c=word(0x59c),busy05cb=word(0x5cb),
  dma_mode0561=word(0x561),brightness0562=word(0x562),queue_head=word(0x35),
  queue_tail=word(0x37),queue_budget=word(0x39),palette_size0568=word(0x568),
  bg2_phase168f=word(0x168f),bg2_scroll0613=word(0x613),
  callback_05c2=word(0x5c2)|(emu.read(0x5c4,emu.memType.snesWorkRam,false)<<16),
  callback_05c5=word(0x5c5)|(emu.read(0x5c7,emu.memType.snesWorkRam,false)<<16),
  ppu_vram_address=st['ppu.vramAddress'],header_count=header_count,
  header_active=in_header and 1 or 0,wait_owner=wait_owner}
 if extra then for k,v in pairs(extra)do values[k]=v end end
 local keys={};for k in pairs(values)do keys[#keys+1]=k end;table.sort(keys)
 local fields={}
 for _,k in ipairs(keys)do
  local v=values[k]
  assert(type(v)=='number' or type(v)=='string','unsupported state field '..k)
  fields[#fields+1]='"'..k..'":'..(type(v)=='string' and ('"'..v..'"') or tostring(v))
 end
 file:write('{'..table.concat(fields,',')..'}\n')
 event=event+1
end
local function hook(pc,fn)
 emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
hook(0x80e1b1,function()if title<0 then title=0 end end)
hook(0x80a2bf,function()
 if title>=850 and setup<0 then setup=0;normalization=0 end
end)
hook(0x81d318,function()rules_ready=rules_ready+1 end)
hook(0x80815a,function()nmi_depth=nmi_depth+1;log('nmi.entry',0x80815a)end)
hook(0x808171,function()log('nmi.reentrant_exit',0x808171);nmi_depth=nmi_depth-1 end)
hook(0x8084a8,function()log('epoch.before_increment',0x8084a8)end)
hook(0x8084ab,function()log('epoch.after_guard',0x8084ab)end)
hook(0x80859b,function()log('nmi.exit',0x80859b);nmi_depth=nmi_depth-1 end)
hook(0x8086b0,function()
 if active()then wait_active=true;wait_owner=in_header and header_count or 0;log('wait.entry',0x8086b0)end
end)
hook(0x8086b7,function()
 if active() and wait_active then
  log('wait.loaded',0x8086b7,{loaded_epoch=emu.getState()['cpu.a']});wait_active=false
 end
end)
hook(0x8086bc,function()log('wait.resume',0x8086bc)end)
hook(0x80eec6,function()
 if active()then header_count=header_count+1;in_header=true
  log('header.entry',0x80eec6);dump(string.format('header_%02d_entry.wram',header_count))
 end
end)
hook(0x80ef1a,function()log('header.before_wait',0x80ef1a)end)
hook(0x80ef1e,function()
 log('header.after_wait',0x80ef1e)
 if active()then dump(string.format('header_%02d_after_wait.wram',header_count))end
end)
hook(0x80ef8d,function()log('header.exit',0x80ef8d);in_header=false end)
for _,pair in ipairs({{0x81f9fc,'setup.nmi_callback'},{0x80ec68,'backdrop.entry'},
 {0x8086da,'queue.wait_entry'},{0x8086e7,'queue.wait_exit'},
 {0x808a02,'palette.submit'},{0x808a41,'palette.immediate_exit'},
 {0x808a56,'palette.queued_exit'},{0x808ad2,'fill.submit'},
 {0x808ba1,'copy.submit'},{0x808c2a,'copy.queued_exit'},
 {0x80c62b,'decompress.entry'},{0x80c682,'decompress.exit'},
 {0x81cf62,'rules.constructor'},{0x81ba8e,'main.constructor'},
 {0x8081e3,'nmi.before_publish'},{0x8083ce,'nmi.queue_completed'},
 {0x8082e3,'nmi.queue_budget_exhausted'},{0x808bcf,'copy.immediate_exit'},
 {0x808ad1,'fill.immediate_exit'}})do
 hook(pair[1],function()log(pair[2],pair[1])end)
end
for _,bank in ipairs({0,0x80,0x81,0x82})do
 emu.addMemoryCallback(function(_,mask)
  if not active()then return end
  local st=emu.getState()
  local pc=(st['cpu.k']<<16)|st['cpu.pc']
  for channel=0,7 do if (mask&(1<<channel))~=0 then
   local a=0x4300+channel*16
   local function r(x)return emu.read(x,emu.memType.snesMemory,false)end
   log('dma.submit',pc,{mask=mask,channel=channel,dma_mode=r(a),bbus=r(a+1),
       source=r(a+2)|(r(a+3)<<8)|(r(a+4)<<16),size=r(a+5)|(r(a+6)<<8)})
  end end
 end,emu.callbackType.write,(bank<<16)|0x420b,(bank<<16)|0x420b,emu.cpuType.snes,emu.memType.snesMemory)
end
local function pulse(n,at)return n>=at and n<at+3 end
emu.addEventCallback(function()
 local input={}
 if normalization>=0 then
  input.down=pulse(normalization,400)or pulse(normalization,520)or pulse(normalization,580)
  input.right=pulse(normalization,460)or pulse(normalization,640)
  input.up=pulse(normalization,700)or pulse(normalization,760)or pulse(normalization,820)
 elseif setup<0 then input.start=pulse(title,850)
 else
  for i=0,3 do if pulse(setup,380+12*i)or pulse(setup,1010+12*i)then input.down=true end end
  input.a=pulse(setup,470)or pulse(setup,1100)
  input.down=input.down or pulse(setup,620)or pulse(setup,630)or pulse(setup,1250)or pulse(setup,1260)
  input.right=pulse(setup,640)or pulse(setup,1270)
  input.start=pulse(setup,830)or pulse(setup,1460)
 end
 emu.setInput(input,0)
end,emu.eventType.inputPolled)
emu.addEventCallback(function()
 global_frame=global_frame+1
 assert(global_frame<6000,'native scheduler journey exceeded bound')
 if title>=0 then title=title+1 end
 if setup<0 then return end
 if normalization>=0 then
  normalization=normalization+1
  if normalization>=920 then normalization=-1;setup=370 end
  return
 end
 log('frame.end',0)
 if setup==469 then
  assert(word(0x16fb)==0 and word(0x16fd)==1 and word(0x16ff)==0 and word(0x1701)==0,'natural UI normalization failed')
  local st=emu.getState();local f=assert(io.open(out..'/state_fields.txt','wb'))
  local keys={};for k in pairs(st)do keys[#keys+1]=k end;table.sort(keys)
  for _,k in ipairs(keys)do f:write(k..'='..tostring(st[k])..'\n')end;f:close()
 end
 if setup==1650 then
  assert(rules_ready>=2 and header_count==4,'required natural constructors not observed')
  file:close()
  local f=assert(io.open(out..'/capture_complete.txt','wb'))
  f:write('ok; headers=4; normal controller-only Rules repeat journey\n');f:close()
  emu.stop(0);return
 end
 setup=setup+1
end,emu.eventType.endFrame)
