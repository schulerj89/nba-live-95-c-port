-- Observe the actual F1 write commit, after its cycle and before its effects.
local out=assert(os.getenv('NBA95_SPC_CONTROL_DIR'))
local active,pending,finished=false,false,false
local pc,count,write_value=0,0,0
local function snapshot(tag)
 local s=emu.getState();local f=assert(io.open(out..'/spc_control_'..count..'_'..tag..'.state','wb'));local keys={}
 f:write('source_pc='..pc..'\nvalue='..write_value..'\n')
 for k,v in pairs(s)do if k:sub(1,4)=='spc.'and type(v)~='table'then keys[#keys+1]=k end end;table.sort(keys)
 for _,k in ipairs(keys)do f:write(k..'='..tostring(s[k])..'\n')end;f:close()
 f=assert(io.open(out..'/spc_control_'..count..'_'..tag..'.aram','wb'))
 for base=0,65535,1024 do local b={};for i=base,base+1023 do b[#b+1]=string.char(emu.read(i,emu.memType.spcRam,false))end;f:write(table.concat(b))end;f:close()
end
emu.addMemoryCallback(function(address)
 if finished then return end
 if pending then
  snapshot('after');pending=false
  if count==2 then finished=true;local f=assert(io.open(out..'/spc_control_complete.txt','wb'));f:write('ok; publications=2\n');f:close();emu.stop(0);return end
 end
 pc=address;if pc==0x380 then active=true end
end,emu.callbackType.exec,0,65535,emu.cpuType.spc,emu.memType.spcMemory)
emu.addMemoryCallback(function(address,value)
 if not active or finished then return end
 assert(not pending);count=count+1;assert(count<=2);write_value=value;snapshot('before');pending=true
end,emu.callbackType.write,0xf1,0xf1,emu.cpuType.spc,emu.memType.spcMemory)
