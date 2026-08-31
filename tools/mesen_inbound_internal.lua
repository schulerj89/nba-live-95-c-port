-- Read-only per-dispatch inbound evidence. No runtime/CPU/ROM injection.
-- The imported generic driver explicitly forces Exhibition while navigating;
-- its legacy CPU-clear mode MUST remain disabled. Controller words are recorded,
-- not presumed CPU-only. Use NBA95_VEC_MAX=1000000 for the imported sidecar,
-- NBA95_INBOUND_INTERNAL_MAX (default 500) for this capture's bounded stop.
assert(os.getenv("NBA95_CPU_VS_CPU") ~= "1",
       "legacy controller clearing is prohibited for this capture")
local out = assert(os.getenv("NBA95_CAPTURE_DIR"))
local tool_dir = assert(os.getenv("NBA95_TOOL_DIR"))
local limit = tonumber(os.getenv("NBA95_INBOUND_INTERNAL_MAX")) or 500
assert(limit > 0)
local file = assert(io.open(out .. "/inbound-internal.jsonl", "wb"))
local frame, count, pending, done = 0, 0, nil, false
local function word(a)
    return emu.read(a, emu.memType.snesWorkRam, false) |
           (emu.read(a + 1, emu.memType.snesWorkRam, false) << 8)
end
local function encode(value)
    if type(value) == "number" then return string.format("%d", value) end
    if type(value) == "boolean" then return value and "true" or "false" end
    if type(value) == "string" then return string.format("%q", value) end
    local keys, parts = {}, {}
    for k in pairs(value) do keys[#keys + 1] = k end
    table.sort(keys)
    for _, k in ipairs(keys) do
        parts[#parts + 1] = string.format("%q:%s", k, encode(value[k]))
    end
    return "{" .. table.concat(parts, ",") .. "}"
end
local function snapshot(pc, actor)
    local state = emu.getState()
    local s = {pc=pc, frame=frame, actor_ptr=actor, current_ptr=word(0x96),
        actor_id=word(actor), cpu_x=state["cpu.x"], cpu_ps=state["cpu.ps"],
        cpu_sp=state["cpu.sp"], cycle=state["cpu.cycleCount"]}
    for name, a in pairs({x_fraction=actor+2,x=actor+4,y_fraction=actor+6,
        y=actor+8,z=actor+0x0c,vx=actor+0x0e,vy=actor+0x10,
        controller=actor+0x16,boost=actor+0x72,flags=actor+0x7e,
        draw_direction=actor+0x4e,target_x=0x958,target_y=0x95a,
        direction=0x95c,dp_aa=0xaa,dp_ac=0xac,dp_ae=0xae,dp_b0=0xb0,
        steering_direction=0xba,dispatch_dt=0xc6,rng=0x7f6,owner=0x93e,
        receiver=0x946,live=0x936,dead=0x968,attachment=0x9f6,
        ready=0x9ba,whistle=0x9b6,event=0x964,transfer=0x9b8,timer=0x92e}) do
        s[name]=word(a)
    end
    local profile_ptr=word(0xe0) | (emu.read(0xe2,emu.memType.snesWorkRam,false)<<16)
    s.profile_42=emu.read((profile_ptr+0x42)&0xffffff,emu.memType.snesMemory,false)
    return s
end
local function hook(pc, callback)
    emu.addMemoryCallback(callback, emu.callbackType.exec, pc, pc,
                         emu.cpuType.snes, emu.memType.snesMemory)
end
emu.addEventCallback(function() frame=frame+1 end, emu.eventType.endFrame)
hook(0x86f43a,function()
    if done then return end
    assert(pending == nil, "nested F43A dispatch: boundary pairing invalid")
    local actor=word(0x96)
    assert(actor>=0x34eb and actor<=0x3deb and (actor-0x34eb)%0x100==0,
           "invalid inbound actor pointer")
    pending={schema="nba95-inbound-internal-v1",call=count+1,
             entry=snapshot(0x86f43a,actor)}
end)
for name,pc in pairs({pre_motion=0x86f4e2,post_motion=0x86f4e6,
                     restored=0x86f4f2,arrived=0x86f520,
                     prepared=0x86f58f,reload=0x86f654}) do
    hook(pc,function()
        if pending then
            assert(pending[name]==nil,"duplicate inbound stage "..name)
            pending[name]=snapshot(pc,pending.entry.actor_ptr)
        end
    end)
end
-- B3C9 passes its final direction in DP AA to A82C. Capture it before A82C
-- reuses scratch words; an end-of-dispatch scratch value is not this input.
hook(0x85a82c,function()
    if pending and pending.pre_motion and not pending.post_motion then
        assert(pending.velocity_entry==nil,"duplicate inbound velocity child")
        pending.velocity_entry=snapshot(0x85a82c,pending.entry.actor_ptr)
    end
end)
for _,pc in ipairs({0x86f439,0x86f653}) do
    hook(pc,function()
        if not pending or done then return end
        pending.exit=snapshot(pc,pending.entry.actor_ptr)
        file:write(encode(pending).."\n"); file:flush()
        pending=nil;count=count+1
        if count>=limit then
            done=true;file:close()
            local finish=assert(io.open(out.."/inbound-internal-complete.txt","wb"))
            finish:write("complete_calls="..count.."\n");finish:close()
            emu.stop(0)
        end
    end)
end
local meta=assert(io.open(out.."/inbound-internal.meta.json","wb"))
meta:write(encode({schema="nba95-inbound-internal-v1",requested_calls=limit,
    native_execution=true,runtime_state_injection=false,
    cpu_state_injection=false,rom_patch=false,
    setup_intervention="generic driver writes Exhibition Mode before Start",
    controller_context="native default; every dispatch records controller word",
    boundaries="F43A,F4E2,F4E6,F4F2,F520,F58F,F654,F439/F653"}).."\n")
meta:close()
dofile(tool_dir.."/mesen_func_vectors.lua")
