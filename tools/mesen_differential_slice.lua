-- Capture an unmodified native ball-init prefix before the sweep harness.
-- Full WRAM snapshots support same-input replay and unexpected-write checks.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local active=false;local seen={};local count=0;local frame=0;local first_frame=0;local entry_cpu
local controlled=os.getenv('NBA95_DIFF_INIT_POISON')=='1'
emu.addEventCallback(function()frame=frame+1 end,emu.eventType.endFrame)
local function dump(name)
    local b={};for a=0,0x1ffff do b[#b+1]=string.char(emu.read(a,emu.memType.snesWorkRam))end
    local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(b));f:close()
end
local function hook(pc,fn)
    emu.addMemoryCallback(fn,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
hook(0x86e056,function()
    assert(count==0 and not active,'unexpected repeated ball initializer')
    if controlled then
        local f=assert(io.open(out..'/ball-init-addresses.txt','rb'))
        for line in f:lines()do
            local a=assert(tonumber(line,16))
            if a~=0x9a then
                local value=(a~0xa55a)&0xffff
                emu.write(a,value&255,emu.memType.snesWorkRam)
                emu.write(a+1,value>>8,emu.memType.snesWorkRam)
            end
        end
        f:close()
    end
    entry_cpu=emu.getState();first_frame=frame
    dump('ball-init-entry.wram');active=true
end)
emu.addMemoryCallback(function(pc)if active then seen[pc]=true end end,
    emu.callbackType.exec,0x86e056,0x86e0ab,emu.cpuType.snes,emu.memType.snesMemory)
hook(0x86e0ac,function()
    assert(active,'initializer exit without entry');active=false;count=count+1
    dump('ball-init-exit.wram')
    local pcs={};for pc in pairs(seen)do pcs[#pcs+1]=pc end;table.sort(pcs)
    local f=assert(io.open(out..'/ball-init-pcs.json','wb'))
    f:write('['..table.concat(pcs,',')..']');f:close()
    f=assert(io.open(out..'/ball-init-meta.json','wb'))
    f:write(string.format('{"frames":%d,"d":%d,"dbr":%d,"ps":%d,"controlled":%s}',
        frame-first_frame,entry_cpu['cpu.d'],entry_cpu['cpu.dbr'],entry_cpu['cpu.ps'],tostring(controlled)));f:close()
end)
dofile(assert(os.getenv('NBA95_DIFF_DRIVER')))
