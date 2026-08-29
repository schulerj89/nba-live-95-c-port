-- Controlled real-entry cases for `$87:9B38-$9BC8`; no code/stack patching.
local function put(a,v) emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam) end
local cases={
 {camera=0,owner=0xffff,x=121,y=0xffed},
 {camera=5,owner=2,x=0xfed4,y=207}
}
local index=0
emu.addMemoryCallback(function()
 index=index+1;local c=cases[index] or cases[#cases]
 local poison={[0x952]=0xaaaa,[0x936]=3,[0x92e]=7,[0x9d6]=9,[0x92c]=11,[0x9c6]=13,
  [0x968]=15,[0x96a]=17,[0x9b0]=19,[0x9b2]=21,[0x97c]=23,[0x910]=25,[0x940]=27,[0x9c4]=29,
  [0x3ef9]=31,[0x3efb]=33}
 for a,v in pairs(poison)do put(a,v)end
 put(0x93a,c.camera);put(0x93e,c.owner);put(0x3eef,c.x);put(0x3ef3,c.y)
 if c.owner~=0xffff then put(0x34eb+c.owner*0x100+0x5e,11) end
end,emu.callbackType.exec,0x879b38,0x879b38,emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
