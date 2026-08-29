-- Nested branch evidence for `$85:B678-$B8CA` captures.
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local trace = assert(io.open(out .. "/mode11_parent.children.jsonl", "wb"))
local active, calls, lane = false, {}, {}
local function word(a)
    local lo=emu.read(a,emu.memType.snesWorkRam,false) or 0
    local hi=emu.read(a+1,emu.memType.snesWorkRam,false) or 0
    return lo | (hi << 8)
end
emu.addMemoryCallback(function() active=true end,emu.callbackType.exec,
    0x87A47A,0x87A47A,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
    if active then calls[#calls+1]={slot=word(0x00C2),lanes={},events={}} end
end,emu.callbackType.exec,0x85B678,0x85B678,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
    if #calls>0 then lane[#lane+1]=calls[#calls] end
end,emu.callbackType.exec,0x85F5E4,0x85F5E4,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
    local call=table.remove(lane)
    if call then call.lanes[#call.lanes+1]=word(0x00AA) end
end,emu.callbackType.exec,0x85F727,0x85F727,emu.cpuType.snes,emu.memType.snesMemory)
for _,pc in ipairs({0x85B734,0x85B74C,0x85B790,0x85B7A6,0x85B80A,0x85B812}) do
    emu.addMemoryCallback(function()
        if #calls>0 then
            local c=calls[#calls]
            c.events[#c.events+1]=string.format('"%06x:%04x"',pc,word(0x07F6))
        end
    end,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
local function finish(exit)
    local call=table.remove(calls);if not call then return end
    local parts={};for _,v in ipairs(call.lanes) do parts[#parts+1]=tostring(v) end
    trace:write(string.format('{"slot":%d,"exit":"%s","lanes":[%s],"events":[%s]}\n',
        call.slot,exit,table.concat(parts,","),table.concat(call.events,",")));trace:flush()
end
emu.addMemoryCallback(function()finish("b837")end,emu.callbackType.exec,
    0x85B837,0x85B837,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()finish("b88c")end,emu.callbackType.exec,
    0x85B88C,0x85B88C,emu.cpuType.snes,emu.memType.snesMemory)
dofile(assert(os.getenv("NBA95_VECTOR_DRIVER")))
