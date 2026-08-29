-- Force the play-request dispatcher at its real callable boundary while
-- preserving every other live CPU-vs-CPU input and the native RNG stream.
local function force_request()
    emu.writeWord(0x0994, 1, emu.memType.snesWorkRam)
    if os.getenv('NBA95_PLAY_REQUEST_LIVE') == '0' then
        emu.writeWord(0x0936, 0, emu.memType.snesWorkRam)
    end
end
for _, pc in ipairs({0x85B116}) do
    emu.addMemoryCallback(force_request, emu.callbackType.exec, pc, pc,
        emu.cpuType.snes, emu.memType.snesMemory)
end
dofile(assert(os.getenv('NBA95_TOOL_DIR')) .. '/mesen_func_vectors.lua')
