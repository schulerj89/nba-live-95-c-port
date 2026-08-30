-- Controlled rectangle/diagonal witnesses on real `$85:96B5` entries.
-- Restore the original actor record after capturing each native return so
-- the matrix does not permanently move actors or install mode8 in gameplay.
local cases={}
for _,mode in ipairs({11,8})do for _,axis in ipairs({'x','y'})do
 local limit=axis=='x' and 394 or 224
 for _,sign in ipairs({1,-1})do
  for _,kind in ipairs({'stationary_outside','crossing','boundary_outward','exact_boundary','outside_inward'})do
   local v=kind=='stationary_outside' and 0 or kind=='crossing' and 0x123 or kind=='outside_inward' and -0x80 or 0x80
   local integer=(kind=='stationary_outside' or kind=='outside_inward') and limit+9 or (kind=='crossing' or kind=='exact_boundary') and limit-1 or limit
   local fraction=kind=='crossing' and 0xf2ef or kind=='exact_boundary' and 0 or 0x12ef
   local c={mode=mode,axis=axis,kind=kind,x=0,y=0,xf=0x12ef,yf=0x34cd,vx=0,vy=0}
   c[axis]=sign*integer;c[axis=='x' and 'xf' or 'yf']=fraction;c[axis=='x' and 'vx' or 'vy']=sign*v
   cases[#cases+1]=c
  end
 end
end end
for _,mode in ipairs({11,8})do for _,p in ipairs({{394,224},{394,-224},{-394,224},{-394,-224}})do
 cases[#cases+1]={mode=mode,axis='both',kind='corner_outward',x=p[1],y=p[2],xf=0x12ef,yf=0x34cd,vx=p[1]>0 and 0x80 or -0x80,vy=p[2]>0 and 0x80 or -0x80}
end end
for _,mode in ipairs({11,8})do for _,p in ipairs({{403,230},{-403,-230},{394,224},{-394,-224}})do
 cases[#cases+1]={mode=mode,axis='both',kind='diagonal_stationary',x=p[1],y=p[2],xf=0x12ef,yf=0x34cd,vx=0,vy=0}
end end
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local labels=assert(io.open(out..'/actor-commit-edge-cases.jsonl','wb'))
local traces=assert(io.open(out..'/actor-commit-edge-pcs.jsonl','wb'))
local index,pending=0,nil
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x8596b5,function()
 if index>=#cases then return end
 assert(not pending,'nested actor edge entry');assert(w(0xc6)==2,'expected native C6=2')
 index=index+1;local c=cases[index];local a=w(0x96);pending={base=a,bytes={},pcs={}}
 for offset=0,255 do pending.bytes[offset]=emu.read(a+offset,emu.memType.snesWorkRam)end
 put(a+2,c.xf);put(a+4,c.x);put(a+6,c.yf);put(a+8,c.y)
 put(a+0xa,0);put(a+0xc,0);put(a+0xe,c.vx);put(a+0x10,c.vy);put(a+0x12,0)
 put(a+0x30,0);put(a+0x32,0);put(a+0x5e,c.mode);put(a+0x60,0x123)
 put(a+0x7e,2);put(a+0xa0,0xabcd)
 labels:write(string.format('{"case":%d,"controlled":true,"mode":%d,"axis":"%s","kind":"%s","x":%d,"y":%d,"xf":%d,"yf":%d,"vx":%d,"vy":%d,"timer":291}\n',index,c.mode,c.axis,c.kind,c.x,c.y,c.xf,c.yf,c.vx,c.vy));labels:flush()
end)
emu.addMemoryCallback(function()
 if not pending then return end
 local s=emu.getState();local pc=s['cpu.k']*65536+s['cpu.pc'];pending.pcs[pc]=true
 if pc==0x85990f or pc==0x859961 then
  local pcs={};for p in pairs(pending.pcs)do pcs[#pcs+1]=p end;table.sort(pcs)
  traces:write('{"case":'..index..',"executed":['..table.concat(pcs,',')..']}\n');traces:flush()
 end
end,emu.callbackType.exec,0x8596b5,0x859a13,emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
local function restore()if pending then for o,v in pairs(pending.bytes)do emu.write(pending.base+o,v,emu.memType.snesWorkRam)end;pending=nil end end
hook(0x85990f,restore);hook(0x859961,restore)
