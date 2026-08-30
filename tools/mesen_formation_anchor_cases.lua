-- Genuine AD6B calls with coherent actor/context WRAM inputs. No CPU, ROM,
-- stack or RNG edits. All changed input WRAM and actor records are restored
-- after the generic driver captures the native return.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local labels=assert(io.open(out..'/formation-anchor-cases.jsonl','wb'))
local traces=assert(io.open(out..'/formation-anchor-pcs.jsonl','wb'))
local cases={}
for _,slot in ipairs({2,7})do for _,anchor in ipairs({-336,336})do
 for _,kind in ipairs({'ordinary0','ordinary8','ordinary14','special'})do
  for _,mirror in ipairs({0,1})do
   local play=kind=='ordinary8' and 8 or kind=='ordinary14' and 14 or 0
   cases[#cases+1]={slot=slot,anchor=anchor,kind=kind,mirror=mirror,play=play}
  end
 end
end end
local index,pending,active=0,nil,false
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x87a47a,function()active=true end)
hook(0x85ad6b,function()
 if not active or index>=#cases then return end
 assert(not pending,'nested formation anchor entry');assert(w(0xc6)==2,'expected native C6=2')
 index=index+1;local c=cases[index];local saved={}
 local function change(a,v)if saved[a]==nil then saved[a]=w(a)end;put(a,v)end
 local actor=0x34eb+c.slot*0x100;local context=c.slot<5 and 0x46eb or 0x476b
 local actor_bytes={}
 for a=0x34eb,0x3eea do actor_bytes[a]=emu.read(a,emu.memType.snesWorkRam)end
 change(0xc2,c.slot);change(0x96,actor);change(0x9e,context)
 -- `$87:9127-$9136` publishes the same actor's long roster pointer before
 -- dispatch. A82C consumes its +42 movement profile through DP E0.
 change(0xe0,w(0x3449+c.slot*4));change(0xe2,w(0x344b+c.slot*4))
 change(0x46f5,c.slot<5 and c.anchor or -c.anchor)
 change(0x4775,c.slot<5 and -c.anchor or c.anchor)
 change(0x93e,0xffff);change(0x968,0);change(0x936,0x82)
 change(0x954,c.slot==2 and 0 or 5);change(0x958,394);change(0x95a,12)
 change(0x9a2,c.kind=='special' and c.slot or 0xffff)
 change(0x996,c.play);change(0x998,0);change(0x99c,c.mirror)
 change(0x948,0);change(0x97c,0);change(0x5c,0)
 put(actor+2,0x1200);put(actor+4,31);put(actor+6,0x3400);put(actor+8,-17)
 put(actor+0xa,0);put(actor+0xc,0);put(actor+0xe,0x80);put(actor+0x10,-0x40)
 put(actor+0x16,0xffff);put(actor+0x56,123);put(actor+0x58,-45)
 put(actor+0x5c,300);put(actor+0x6e,c.slot<5 and 0 or 5)
 put(actor+0x72,0);put(actor+0x7e,c.kind=='special' and 8 or 0)
 pending={saved=saved,actors=actor_bytes,pcs={0x85ad6b}}
 labels:write(string.format('{"case":%d,"controlled":true,"slot":%d,"context":%d,"anchor":%d,"kind":"%s","play":%d,"mirror":%d}\n',index,c.slot,context,c.anchor,c.kind,c.play,c.mirror));labels:flush()
end)
for _,pc in ipairs({0x85ada3,0x85adf5,0x85adfc,0x85ae13,0x85ae1f,0x85ae2c,0x85aeb5,0x85aeef,0x85af22,0x85af44})do
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
