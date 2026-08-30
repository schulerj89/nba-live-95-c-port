-- Controlled receiver-cancellation witnesses at genuine, already-arrived
-- F43A entries. Only $09B8/$0946 are replaced; restore them after recording
-- F58F, before the native CPU selector. No ROM/CPU/stack edits.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local file=assert(io.open(out..'/inbound-cancel-recovery.jsonl','wb'))
local cases={{1,0xffff},{1,0},{0xa5a5,0xffff},{0,0xffff}}
local index,frame,pending=0,0,nil
local function w(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local function signed(v)return v>=32768 and v-65536 or v end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
local function snapshot()
 local cpu=emu.getState();local registers={}
 for _,key in ipairs({'a','x','y','ps','sp','d','dbr','k','pc'})do
  registers[#registers+1]='"'..key..'":'..tostring(cpu['cpu.'..key])
 end
 local memory={};for a=0,0x4aff do memory[#memory+1]=string.format('%02x',emu.read(a,emu.memType.snesWorkRam))end
 return '{"cpu":{'..table.concat(registers,',')..'},"mem":{"0000":"'..table.concat(memory)..'"}}'
end
hook(0x86f43a,function()
 if pending or index>=#cases then return end
 local actor=w(0x96)
 local dx=signed((w(0x958)-w(actor+4))&0xffff)
 local dy=signed((w(0x95a)-w(actor+8))&0xffff)
 if dx< -9 or dx>8 or dy< -9 or dy>8 then return end
 assert(w(0x936)==0x82 and w(actor+0x5e)==11,'expected live dead-ball owner')
 index=index+1;local c=cases[index]
 pending={frame=frame,transfer=w(0x9b8),receiver=w(0x946),pcs={}}
 put(0x9b8,c[1]);put(0x946,c[2]);pending.entry=snapshot()
end)
emu.addMemoryCallback(function()
 if pending then local s=emu.getState();pending.pcs[s['cpu.k']*65536+s['cpu.pc']]=true end
end,emu.callbackType.exec,0x86f43a,0x86f58f,emu.cpuType.snes,emu.memType.snesMemory)
hook(0x86f58f,function()
 if not pending then return end
 local p=pending;local pcs={};for pc in pairs(p.pcs)do pcs[#pcs+1]=pc end;table.sort(pcs)
 file:write('{"case":'..index..',"controlled":true,"entry_frame":'..p.frame..',"exit_frame":'..frame..
  ',"entry_pc":"86f43a","exit_pc":"86f58f","entry":'..p.entry..',"exit":'..snapshot()..
  ',"executed":['..table.concat(pcs,',')..']}\n');file:flush()
 put(0x9b8,p.transfer);put(0x946,p.receiver);pending=nil
 if index==#cases then
  file:close();local f=assert(io.open(out..'/inbound-cancel-recovery-complete.txt','wb'));f:write('controlled_cases='..index..'\n');f:close();emu.stop(0)
 end
end)
emu.addEventCallback(function()frame=frame+1 end,emu.eventType.endFrame)
-- This supplies the established Exhibition menu driver only; its own sparse
-- trace is a driver artifact, not part of these independently recorded cases.
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
