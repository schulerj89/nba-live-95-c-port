-- Controlled genuine `$87:92A5` parent calls. No ROM/CPU/register edits.
-- Each fresh entry receives one owned/free OOB boundary vector; capture ends
-- at `$87:94A2`, after native 93BB/93BE and the nested native reset helper.
local cases={}
local points={{378,0},{-378,0},{-379,0},{0,208},{0,-208},{0,-209}}
for _,owner in ipairs({2,7,-1})do for _,p in ipairs(points)do
 for _,v in ipairs({-256,256})do cases[#cases+1]={owner=owner,x=p[1],y=p[2],vx=v,vy=v,live=0,z=0}end
end end
for _,p in ipairs({{378,0},{-379,0},{0,208},{0,-209}})do
 cases[#cases+1]={owner=-1,x=p[1],y=p[2],vx=0,vy=0,live=0,z=0}
end
for _,owner in ipairs({2,7})do for _,guard in ipairs({'airborne','inbound'})do
 cases[#cases+1]={owner=owner,x=400,y=220,vx=256,vy=256,live=guard=='inbound' and 0x82 or 0,z=guard=='airborne' and 1 or 0}
end end
-- Ownerless X checks dominate: an inward X edge skips even when Y is out.
cases[#cases+1]={owner=-1,x=378,y=208,vx=-256,vy=256,live=0,z=0}
cases[#cases+1]={owner=-1,x=-379,y=-209,vx=256,vy=-256,live=0,z=0}
local index=0
local function put(a,v)emu.write(a,v&255,emu.memType.snesWorkRam);emu.write(a+1,(v>>8)&255,emu.memType.snesWorkRam)end
local file=assert(io.open(assert(os.getenv('NBA95_CAPTURE_DIR'))..'/violation-oob-cases.jsonl','wb'))
emu.addMemoryCallback(function()
 if index>=#cases then return end
 index=index+1;local c=cases[index]
 for _,a in ipairs({0x9b4,0x9b6,0x964,0x9bc,0x978,0xa02,0x948})do put(a,0)end
 put(0x17d5,1);put(0x93a,c.owner==7 and 5 or 0);put(0x93e,c.owner);put(0x936,c.live)
 put(0x492d,2);put(0x492f,7);put(0x472a,2);put(0x47aa,7)
 put(0x3eef,c.owner<0 and c.x or 13);put(0x3ef3,c.owner<0 and c.y or -17)
 put(0x3ef7,80);put(0x3ef9,c.vx);put(0x3efb,c.vy)
 if c.owner>=0 then local a=0x34eb+c.owner*0x100
  put(a+4,c.x);put(a+8,c.y);put(a+0xc,c.z);put(a+0xe,c.vx);put(a+0x10,c.vy);put(a+0x5e,11)
 end
 file:write(string.format('{"case":%d,"controlled":true,"owner":%d,"x":%d,"y":%d,"vx":%d,"vy":%d,"live":%d,"z":%d}\n',index,c.owner,c.x,c.y,c.vx,c.vy,c.live,c.z));file:flush()
end,emu.callbackType.exec,0x8792a5,0x8792a5,emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv('NBA95_TOOL_DIR'))..'/mesen_func_vectors.lua')
