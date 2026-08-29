-- Controlled real-entry cases for `$85:93F5`; no PC/ROM/stack patching.
local function put(a,v)
 emu.write(a,v&255,emu.memType.snesWorkRam)
 emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)
end
local cases={
 {name='busy',values={[0x9b6]=1,[0x964]=1}},
 {name='empty',values={[0x9b6]=0,[0x964]=0}},
 {name='defensive-negative-timer',values={[0x964]=1,[0x93a]=0,[0x8de]=0xffff,[0x9bc]=0}},
 {name='offensive-busy-presentation-short',values={[0x964]=13,[0x93a]=5,[0x8de]=12,[0x9bc]=1,[0x9ca]=1}}
}
local index=0
emu.addMemoryCallback(function()
 index=index+1;local c=cases[index] or cases[#cases]
 local defaults={[0x9b6]=0,[0x964]=0,[0x8f0]=0xffff,[0x9ba]=1,
  [0x497f]=7,[0x4937]=0,[0x9bc]=0,[0x13e7]=0x20,[0x93a]=0,
  [0x13e9]=0,[0x8de]=0xffff,[0x8e2]=9,[0x8e6]=0,[0x8e8]=0,
  [0x9ca]=0,[0x9cc]=0}
 for a,v in pairs(defaults)do put(a,v)end
 for a,v in pairs(c.values)do put(a,v)end
end,emu.callbackType.exec,0x8593f5,0x8593f5,emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
