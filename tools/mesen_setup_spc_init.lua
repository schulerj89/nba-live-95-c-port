-- Unmodified cold-reset SPC source observation, no injected hardware state.
local out=assert(os.getenv('NBA95_SPC_INIT_DIR'))
local ins=assert(io.open(out..'/spc_init_instructions.bin','wb'))
local writes=assert(io.open(out..'/spc_init_writes.bin','wb'))
local io_reads=assert(io.open(out..'/spc_init_io_reads.bin','wb'))
local active,finished=false,false
local pc,n,w=0,0,0
local function snapshot(tag)
 local s=emu.getState();local f=assert(io.open(out..'/spc_init_'..tag..'.state','wb'));local keys={}
 for k,v in pairs(s)do if k:sub(1,4)=='spc.'and type(v)~='table'then keys[#keys+1]=k end end;table.sort(keys)
 for _,k in ipairs(keys)do f:write(k..'='..tostring(s[k])..'\n')end;f:close()
 f=assert(io.open(out..'/spc_init_'..tag..'.aram','wb'))
 for base=0,65535,1024 do local b={};for i=base,base+1023 do b[#b+1]=string.char(emu.read(i,emu.memType.spcRam,false))end;f:write(table.concat(b))end;f:close()
end
emu.addMemoryCallback(function(address)
 if finished then return end
 pc=address
 if pc==0x380 and not active then active=true;snapshot('entry')end
 if not active then return end
 if pc==0x387 then snapshot('post_control')end
 if pc==0x3db then snapshot('dsp_entry')end
 local s=emu.getState()
 ins:write(string.pack('<I2BBBBBI8',pc,s['spc.a'],s['spc.x'],s['spc.y'],s['spc.sp'],s['spc.ps'],s['spc.cycle']));n=n+1
 assert(n<300000,'bounded instruction capture exceeded')
end,emu.callbackType.exec,0,65535,emu.cpuType.spc,emu.memType.spcMemory)
emu.addMemoryCallback(function(address,value)
 if not active or finished then return end
 local s=emu.getState();writes:write(string.pack('<I2I2BI8',pc,address,value,s['spc.cycle']));w=w+1
end,emu.callbackType.write,0,65535,emu.cpuType.spc,emu.memType.spcMemory)
emu.addMemoryCallback(function(address,value)
 if not active or finished then return end
 local s=emu.getState();io_reads:write(string.pack('<I2I2BI8',pc,address,value,s['spc.cycle']))
 if pc==0x3db and address==0xf3 then
  snapshot('pending_dsp');finished=true
  ins:close();writes:close();io_reads:close()
  local f=assert(io.open(out..'/spc_init_complete.txt','wb'));f:write(string.format('ok; instructions=%d; writes=%d\n',n,w));f:close();emu.stop(0)
 end
end,emu.callbackType.read,0xf0,0xff,emu.cpuType.spc,emu.memType.spcMemory)
