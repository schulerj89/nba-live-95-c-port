-- Native scheduler/scratch producer evidence. Observational only.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local file=assert(io.open(out..'/runtime-events.jsonl','wb'))
local frames=assert(io.open(out..'/runtime-frames.jsonl','wb'))
local ready=false;local frame=0;local writer={0,0,0,0};local seen={}
local reaches=assert(io.open(out..'/reach-children.jsonl','wb'));local reach=nil
local graphics=assert(io.open(out..'/graphics-scratch.jsonl','wb'));local graphics_in=nil
local duplicates=assert(io.open(out..'/graphics-duplicate-reads.jsonl','wb'))
local function w(a)return emu.readWord(a,emu.memType.snesWorkRam)end
local function arr(v)local s={};for _,n in ipairs(v)do s[#s+1]=tostring(n)end;return '['..table.concat(s,',')..']'end
local function hook(pc,fn)emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)end
emu.addMemoryCallback(function()
    if not ready then return end
    local s=emu.getState()
    file:write(string.format('{"timer_writer":%d,"frame":%d,"value":%d}\n',s['cpu.k']*65536+s['cpu.pc'],frame,w(0x9f2)));file:flush()
end,emu.callbackType.write,0x9f2,0x9f3,emu.cpuType.snes,emu.memType.snesWorkRam)
hook(0x87a47a,function()ready=true end)
hook(0x86eaa8,function()
    if not ready then return end
    local a=w(0x96);local b=w(0x910);local context=w(0x9e)
    reach={actor=a,input={w(a+4),w(a+8),w(b+4),w(b+8),w(b+0xe),w(b+0x10),
        w(b+0x8c),w(context+0xa),w(a+0x4e),w(a+0x50),w(a+0xe),w(a+0x10),w(a+0x12),w(0x91c)}}
end)
hook(0x86ec31,function()
    if not reach then return end
    local v=reach.input;local a=reach.actor
    v={v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],w(a+0x4e),w(a+0x50),w(a+0xe),w(a+0x10),w(a+0x12),w(0x91c)}
    reaches:write(string.format('{"frame":%d,"input":%s,"output":%s}\n',frame,arr(reach.input),arr(v)));reaches:flush();reach=nil
end)
emu.addMemoryCallback(function()
    if not ready then return end
    local s=emu.getState();writer={s['cpu.k']*65536+s['cpu.pc'],w(0x96),s['cpu.x'],frame}
end,emu.callbackType.write,0x46,0x48,emu.cpuType.snes,emu.memType.snesWorkRam)
local function record(pc)
    if not ready or frame>280 then return end
    file:write(string.format('{"frame":%d,"pc":%d,"actor":%d,"scratch":%d,"writer":%s,"words":%s}\n',
        frame,pc,w(0x96),w(0x46),arr(writer),arr({w(0x9f2),w(0x9f6),w(0x936),w(0x920),w(0x564),w(0xc6),w(0xc8),w(0x3ef7),w(0x3efd)})));file:flush()
end
for _,pc in ipairs({0x878efb,0x878f95,0x879244,0x86ec32,0x859a24,0x86e1a6})do hook(pc,function()record(pc)end)end
hook(0x82f12d,function()
    if not ready or frame>220 then return end
    local s=emu.getState();file:write(string.format('{"graphics_writer":%d,"frame":%d,"a":%d,"x":%d,"y":%d,"ee":%d,"ef":%d,"49":%d,"0c":%d}\n',
      0x82f12d,frame,s['cpu.a'],s['cpu.x'],s['cpu.y'],w(0xee),emu.read(0xef,emu.memType.snesWorkRam),w(0x49),w(0x0c)));file:flush()
end)
local function graphics_state()
    local v={w(0x7f6),w(0x46)}
    for base=0x8e24,0x8e34,8 do for off=0,6,2 do v[#v+1]=w(base+off)end end
    return v
end
hook(0x82f02f,function()
    if not ready then return end
    if not graphics_in then graphics_in=graphics_state();graphics_in[#graphics_in+1]=emu.read(0x90,emu.memType.snesWorkRam);graphics_in[#graphics_in+1]=emu.read(0x94,emu.memType.snesWorkRam) end
end)
hook(0x82f115,function()if ready and graphics_in then
    local output=graphics_state();output[#output+1]=emu.read(0x90,emu.memType.snesWorkRam);output[#output+1]=emu.read(0x94,emu.memType.snesWorkRam)
    graphics:write(string.format('{"frame":%d,"input":%s,"output":%s}\n',frame,arr(graphics_in),arr(output)));graphics:flush();graphics_in=nil
end end)
-- F06A reads each queue descriptor key through a bank-$82 long pointer.  An
-- empty queue record ($FFFF) deliberately wraps to $82:0005; capture the
-- actual accumulator at F06C so the C scheduler can model that bus-visible
-- comparison without borrowing the recompiler runtime's address fallback.
local duplicate_read=nil
hook(0x82f06a,function()
    if not ready or frame>220 then return end
    local s=emu.getState()
    duplicate_read={frame,s['cpu.y'],w(0x14),w(0x92),w(0x8e),w(0x7f6)}
end)
hook(0x82f06c,function()
    if not duplicate_read then return end
    local s=emu.getState();duplicate_read[#duplicate_read+1]=s['cpu.a']
    duplicates:write(string.format('{"values":%s}\n',arr(duplicate_read)));duplicates:flush()
    duplicate_read=nil
end)
emu.addMemoryCallback(function(pc)
    if ready and frame<150 then seen[pc]=true end
end,emu.callbackType.exec,0x878c00,0x878efa,emu.cpuType.snes,emu.memType.snesMemory)
emu.addEventCallback(function()
    if not ready then return end
    frame=frame+1
    if frame==114 or frame==160 or frame==190 or frame==210 then
        local shot=assert(io.open(out..'/native-'..frame..'.png','wb'));shot:write(emu.takeScreenshot());shot:close()
    end
    if frame<=280 then
        local values={}
        for _,a in ipairs({0x9f2,0x9f6,0x7f6,0x936,0x93e,0x942,0x946,0x948,0x3ef5,0x3ef7,0x3efd})do values[#values+1]=w(a)end
        for slot=0,9 do for _,offset in ipairs({4,8,0xa,0xc,0xe,0x10,0x12,0x28,0x2a,0x2c,0x30,0x32,0x3a,0x3c,0x46,0x48,0x4e,0x50,0x52,0x5a,0x5e,0x60,0x8e})do
            values[#values+1]=w(0x34eb+slot*256+offset)
        end end
        frames:write(string.format('{"frame":%d,"words":%s}\n',frame,arr(values)));frames:flush()
    end
    if frame==280 then
        local pcs={};for p in pairs(seen)do pcs[#pcs+1]=p end;table.sort(pcs)
        local f=assert(io.open(out..'/scheduler-pcs.json','wb'));f:write(arr(pcs));f:close()
    end
end,emu.eventType.endFrame)
dofile(assert(os.getenv('NBA95_DIFF_JUMP_DRIVER')))
