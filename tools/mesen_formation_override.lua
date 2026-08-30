-- Separate ten-call native override witness. This changes WRAM only at a
-- genuinely reached AD6B entry, never ROM/CPU/stack/RNG. See capture metadata.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local labels=assert(io.open(out..'/formation-override-cases.jsonl','wb'))
local traces=assert(io.open(out..'/formation-override-pcs.jsonl','wb'))
local cases={}
for _,slot in ipairs({2,7})do for play=6,9 do
 cases[#cases+1]={slot=slot,play=play,kind='positive',tx=-133,ty=224,candidate=slot<5 and 4 or 9}
end end
cases[#cases+1]={slot=2,play=9,kind='skip_x_nonnegative',tx=0,ty=224,candidate=-1}
cases[#cases+1]={slot=7,play=9,kind='skip_y_negative',tx=-133,ty=-1,candidate=-1}
local index,pending,active=0,nil,false
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x87a47a,function()active=true end)
hook(0x85ad6b,function()
 if not active or index>=#cases then return end
 assert(not pending,'nested formation override entry');assert(w(0xc6)==2,'expected native C6=2')
 index=index+1;local c=cases[index];local saved={}
 local function change(a,v)if saved[a]==nil then saved[a]=w(a)end;put(a,v)end
 local actor=0x34eb+c.slot*0x100;local context=c.slot<5 and 0x46eb or 0x476b
 local actor_bytes={}
 for a=0x34eb,0x3eea do actor_bytes[a]=emu.read(a,emu.memType.snesWorkRam)end
 change(0xc2,c.slot);change(0x96,actor);change(0x9e,context)
 -- Same actor's existing roster profile pointer, per87:9127-9136.
 change(0xe0,w(0x3449+c.slot*4));change(0xe2,w(0x344b+c.slot*4))
 change(0x46f5,-336);change(0x4775,336)
 change(0x93e,0xffff);change(0x968,0);change(0x936,0x82)
 change(0x954,c.slot);change(0x958,c.tx);change(0x95a,c.ty)
 change(0x9a2,0xffff);change(0x996,c.play);change(0x998,0);change(0x99c,0)
 change(0x948,0);change(0x97c,0);change(0x5c,0)
 put(actor+2,0x1200);put(actor+4,31);put(actor+6,0x3400);put(actor+8,-17)
 put(actor+0xa,0);put(actor+0xc,0);put(actor+0xe,0x80);put(actor+0x10,-0x40)
 put(actor+0x5c,300);put(actor+0x6e,c.slot<5 and 0 or 5)
 put(actor+0x72,0);put(actor+0x7e,0)
 for i=0,9 do
  local base=0x34eb+i*0x100
  put(base,i);put(base+0x16,0xffff)
  put(base+0x56,120+i);put(base+0x58,-45-i)
 end
 pending={saved=saved,actors=actor_bytes,pcs={0x85ad6b}}
 labels:write(string.format('{"case":%d,"controlled":true,"slot":%d,"context":%d,"anchor":%d,"kind":"%s","play":%d,"exclude":%d,"humans":0,"tx":%d,"ty":%d,"candidate":%d}\n',index,c.slot,context,c.slot<5 and -336 or 336,c.kind,c.play,c.slot,c.tx,c.ty,c.candidate));labels:flush()
end)
for _,pc in ipairs({0x85adf5,0x85adfc,
 0x85ae39,0x85ae3c,0x85ae3f,0x85ae41,0x85ae44,0x85ae47,0x85ae49,0x85ae4c,
 0x85ae4e,0x85ae51,0x85ae53,0x85ae56,0x85ae58,0x85ae5a,0x85ae5d,0x85ae5e,
 0x85ae61,0x85ae62,0x85ae63,0x85ae67,0x85ae68,0x85ae6a,0x85ae6d,0x85ae6f,
 0x85ae72,0x85ae75,0x85ae77,0x85ae7a,0x85ae7c,0x85ae7d,0x85ae7e,0x85ae81,
 0x85ae82,0x85ae84,0x85ae86,0x85ae88,0x85ae8b,0x85ae8e,0x85ae91,0x85ae94,
 0x85ae95,0x85ae97,0x85aeef,0x85af22,0x85af44})do
 hook(pc,function()if pending then pending.pcs[#pending.pcs+1]=pc end end)
end
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
local function finish(pc)
 if not pending then return end
 pending.pcs[#pending.pcs+1]=pc
 traces:write(string.format('{"case":%d,"executed":[%s]}\n',index,table.concat(pending.pcs,',')));traces:flush()
 for a,v in pairs(pending.actors)do emu.write(a,v,emu.memType.snesWorkRam)end
 for a,v in pairs(pending.saved)do put(a,v)end
 pending=nil
end
hook(0x85ad77,function()finish(0x85ad77)end)
hook(0x85af5b,function()finish(0x85af5b)end)
