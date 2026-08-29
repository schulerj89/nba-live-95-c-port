-- Boundary-only branch controller for `$87:92A5-$949E`. It alters WRAM
-- inputs at the real entry and never patches ROM, PC, stack, flags, or RNG.
local mode = assert(os.getenv("NBA95_VIOLATION_CONTROL"))
local function word(a)
    local lo=emu.read(a,emu.memType.snesWorkRam,false) or 0
    local hi=emu.read(a+1,emu.memType.snesWorkRam,false) or 0
    return lo | (hi << 8)
end
local function write_word(a,v)
    emu.write(a,v & 0xff,emu.memType.snesWorkRam)
    emu.write(a+1,(v >> 8) & 0xff,emu.memType.snesWorkRam)
end
emu.addMemoryCallback(function()
    write_word(0x09B4,0);write_word(0x09B6,0)
    write_word(0x0964,0);write_word(0x09BC,0);write_word(0x0978,0)
    write_word(0x093A,0);write_word(0x093E,2);write_word(0x0936,0)
    write_word(0x492D,2);write_word(0x492F,7)
    write_word(0x3EEF,0);write_word(0x3EF3,0);write_word(0x3EF7,0x0050)
    write_word(0x3EF9,0);write_word(0x3EFB,0)
    if mode=="none" then write_word(0x093E,0xFFFF);return
    elseif mode=="busy" then write_word(0x09B4,1)
    elseif mode=="interference" then
        write_word(0x0964,6);write_word(0x17DB,1)
    elseif mode=="interference-off" then
        write_word(0x0964,6);write_word(0x17DB,0)
    elseif mode=="boundary" then
        write_word(0x17D5,1);write_word(0x093E,0xFFFF)
        write_word(0x3EEF,0x017A);write_word(0x3EF3,0)
        write_word(0x3EF9,1);write_word(0x3EFB,0)
    elseif mode=="code5" then write_word(0x0964,5)
    elseif mode=="code7" then write_word(0x0964,7)
    elseif mode=="charging" then write_word(0x0964,2)
    elseif mode=="offensive" then write_word(0x0964,13)
    elseif mode=="defensive" then write_word(0x0964,1)
    elseif mode=="deferred" then
        write_word(0x09BC,1);write_word(0x0A02,2);write_word(0x0948,1)
    elseif mode=="deferred-wait" then
        write_word(0x09BC,1);write_word(0x0A02,1);write_word(0x0948,1)
    else error("unknown NBA95_VIOLATION_CONTROL="..mode) end
end,emu.callbackType.exec,0x8792A5,0x8792A5,
    emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv("NBA95_VECTOR_DRIVER")))
