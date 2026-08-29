-- Capture portable `$87:A846-$A979` inputs and final B344 call arguments.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local f=assert(io.open(out..'/draw-indicator-calls.jsonl','wb'));f:setvbuf('no')
local pending,calls={},0
local function w(a)return (emu.read(a,emu.memType.snesWorkRam,false)or 0)|((emu.read(a+1,emu.memType.snesWorkRam,false)or 0)<<8)end
local function cpu(n)return emu.getState()['cpu.'..n]or 0 end
if os.getenv('NBA95_INDICATOR_CONTROL')=='1' then
 local cases={}
 for controller=0,3 do
  for _,point in ipairs({{-21,100},{276,100},{100,-21},{100,288},
                         {-21,-21},{276,-21},{-21,288},{276,288}})do
   cases[#cases+1]={point[1],point[2],controller}
  end
 end
 local ci=0
 emu.addMemoryCallback(function()
  ci=ci+1;local c=cases[((ci-1)%#cases)+1];local x=cpu('x')&0xffff
  local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
  put(x+0x6a,c[1]);put(x+0x68,c[2]);put(x+0x16,c[3])
 end,emu.callbackType.exec,0x87a846,0x87a846,emu.cpuType.snes,emu.memType.snesMemory)
end
emu.addMemoryCallback(function()
 local x=cpu('x')&0xffff
 pending[#pending+1]={x=x,sx=w(x+0x6a),sy=w(x+0x68),controller=w(x+0x16)}
end,emu.callbackType.exec,0x87a846,0x87a846,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
 local p=table.remove(pending);if not p then return end;calls=calls+1
 f:write(string.format('{"call":%d,"input":[%d,%d,%d],"expected":[%d,%d,%d,%d,%d]}\n',calls,p.sx,p.sy,p.controller,w(0),cpu('a')&0xffff,cpu('x')&0xffff,cpu('y')&0xffff,w(p.x+0x6a)))
end,emu.callbackType.exec,0x87a979,0x87a979,emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
