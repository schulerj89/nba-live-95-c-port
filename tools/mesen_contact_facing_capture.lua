-- Observe both native callers of F02D. All words are real ROM inputs;
-- no forced contact, PC jump, or WRAM modification in this witness.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local f=assert(io.open(out..'/contact_facing.jsonl','wb'))
local function word(a)return emu.read(a,emu.memType.snesWorkRam)|(emu.read(a+1,emu.memType.snesWorkRam)<<8)end
for _,spec in ipairs({{0x86c217,0x86c223,'x'},{0x86cb5e,0x86cb6a,'y'}})do
    local pending
    emu.addMemoryCallback(function()
        local base=emu.getState()['cpu.'..spec[3]]
        pending={base=base,x=word(0xaa),y=word(0xae),vx=word(base+0xe),vy=word(base+0x10)}
    end,emu.callbackType.exec,spec[1],spec[1],emu.cpuType.snes,emu.memType.snesMemory)
    emu.addMemoryCallback(function()
        if not pending then return end
        local p=pending
        f:write(string.format('{"entry":%d,"dx":%d,"dy":%d,"vx":%d,"vy":%d,"facing":%d}\n',
            spec[1],p.x,p.y,p.vx,p.vy,word(p.base+0x4e)));f:flush();pending=nil
    end,emu.callbackType.exec,spec[2],spec[2],emu.cpuType.snes,emu.memType.snesMemory)
end
dofile(assert(os.getenv('NBA95_VECTOR_DRIVER')))
