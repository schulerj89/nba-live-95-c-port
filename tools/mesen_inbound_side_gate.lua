-- Controlled branch-boundary witnesses on genuine native F61F calls.
-- Temporarily change only the gate inputs and actor ID/group labels, restoring
-- all changed WRAM at F648/F653 before native gameplay continues. No ROM or
-- CPU-state writes and no C-derived expected results are used.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local file=assert(io.open(out..'/inbound-side-gate.jsonl','wb'))
local cases={}
for _,group in ipairs({0,5})do for _,anchor in ipairs({-336,336})do
 for _,owner in ipairs({-21,-20,-19,19,20})do for _,receiver in ipairs({-1,0})do
  cases[#cases+1]={group=group,anchor=anchor,owner=owner,receiver=receiver}
 end end
end end
local index,frame,pending=0,0,nil
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function signed(v)return v>=32768 and v-65536 or v end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
hook(0x86f61f,function()
 if pending or index>=#cases then return end
 local s=emu.getState();local owner=s['cpu.y'];local context=w(0x9e);local receiver=w(0x8e)
 assert(owner~=receiver,'gate owner and receiver alias')
 index=index+1;local c=cases[index];local saved={}
 local function change(a,v)if saved[a]==nil then saved[a]=w(a)end;put(a,v)end
 change(context+0xa,c.anchor);change(owner+4,c.owner);change(receiver+4,c.receiver)
 change(owner,c.group+2);change(owner+0x6e,c.group)
 pending={saved=saved,frame=frame,owner=owner,receiver=receiver,context=context,
  a=s['cpu.a'],x=s['cpu.x'],y=s['cpu.y'],ps=s['cpu.ps'],sp=s['cpu.sp'],d=s['cpu.d'],dbr=s['cpu.dbr'],
  anchor=signed(w(context+0xa)),owner_x=signed(w(owner+4)),receiver_x=signed(w(receiver+4)),group=w(owner+0x6e),actor=w(owner),pcs={0x86f61f}}
end)
local function finish(pc)
 if not pending then return end
 local p=pending;local s=emu.getState();p.pcs[#p.pcs+1]=pc
 file:write(string.format('{"case":%d,"controlled":true,"frame":%d,"entry_pc":%d,"exit_pc":%d,"actor":%d,"group":%d,"owner_pointer":%d,"receiver_pointer":%d,"context_pointer":%d,"input":[%d,%d,%d],"entry_cpu":[%d,%d,%d,%d,%d,%d,%d],"exit_cpu":[%d,%d,%d,%d],"executed":[%s],"allowed":%s}\n',index,p.frame,0x86f61f,pc,p.actor,p.group,p.owner,p.receiver,p.context,p.anchor,p.owner_x,p.receiver_x,p.a,p.x,p.y,p.ps,p.sp,p.d,p.dbr,s['cpu.a'],s['cpu.x'],s['cpu.y'],s['cpu.ps'],table.concat(p.pcs,','),tostring(pc==0x86f648)))
 file:flush();for a,v in pairs(p.saved)do put(a,v)end;pending=nil
 if index==#cases then file:close();local f=assert(io.open(out..'/inbound-side-gate-complete.txt','wb'));f:write('controlled_cases='..index..'\n');f:close();emu.stop(0)end
end
for _,pc in ipairs({0x86f621,0x86f624,0x86f626,0x86f629,0x86f62c,0x86f62e,0x86f630,0x86f633,0x86f635,0x86f637,0x86f63a,0x86f63d,0x86f63f,0x86f641,0x86f644,0x86f646})do
 hook(pc,function()if pending then pending.pcs[#pending.pcs+1]=pc end end)
end
hook(0x86f648,function()finish(0x86f648)end)
hook(0x86f653,function()finish(0x86f653)end)
emu.addEventCallback(function()frame=frame+1 end,emu.eventType.endFrame)
-- Reuse the verified Exhibition menu driver; its own unrelated empty vector
-- file is only a driver artifact and is not part of this gate fixture.
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
