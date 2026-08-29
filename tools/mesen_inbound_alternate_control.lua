-- Force the native ordered selector through `$86:F5D2` and fallback gate.
local function get(a)return (emu.read(a,emu.memType.snesWorkRam,false)or 0)|((emu.read(a+1,emu.memType.snesWorkRam,false)or 0)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local call=0
emu.addMemoryCallback(function()
 call=call+1;local ptr=get(0x96);local actor=get(ptr);local base=math.floor(actor/5)*5
 put(0x9aa,0xffff)
 if call==1 then
  local candidate=base+((actor-base+1)%5);put(0x9ac,candidate);put(0x9ae,0xffff);put(0x92e,0)
  local cp=0x34eb+candidate*0x100
  put(cp+0x5e,2);put(cp+4,(get(ptr+4)+10)&0xffff);put(cp+8,get(ptr+8));put(cp+0x86,3);put(cp+0x8a,64)
 else
  put(0x9ac,0xffff);put(0x9ae,0xffff);put(0x92e,60)
 end
end,emu.callbackType.exec,0x86f5c7,0x86f5c7,emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
