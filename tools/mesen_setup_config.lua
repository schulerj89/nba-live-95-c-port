-- Natural controller-only configuration journeys, with full boundary state.
-- No ROM, CPU register, WRAM, SRAM, or PPU writes. Screens are evidence only.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local journey=assert(os.getenv('NBA95_CONFIG_JOURNEY'))
assert(journey=='presets' or journey=='rules' or journey=='options' or journey=='load' or journey=='held' or journey=='main' or journey=='input' or journey=='faces')
local function json(value)
    if type(value)=='string'then return '"'..value:gsub('\\','\\\\'):gsub('"','\\"')..'"'end
    if type(value)~='table'then return tostring(value)end
    local entries={}
    if #value>0 then
        for _,v in ipairs(value)do entries[#entries+1]=json(v)end
        return '['..table.concat(entries,',')..']'
    end
    local keys={};for k in pairs(value)do keys[#keys+1]=k end;table.sort(keys)
    for _,k in ipairs(keys)do entries[#entries+1]=json(k)..':'..json(value[k])end
    return '{'..table.concat(entries,',')..'}'
end
local events=assert(io.open(out..'/events.jsonl','wb'))
local stages=assert(io.open(out..'/action-states.jsonl','wb'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder());home:close()
local frame,title_frame,setup_frame=0,-1,-1
local action_index,action_frame=1,-1
local actions={}
local function add(key,label,wait,hold)
    actions[#actions+1]={key=key,label=label,wait=wait or 60,hold=hold or 3}
end
local function down(count)for i=1,count do add('down','move_down')end end
if journey=='faces'then
    for _,key in ipairs({'b','y','x','l','r','a+start','b+right'})do
        down(4);add(key,'face_mask_opens_rules_'..key,260)
        add('start','return_from_rules',260)
    end
    down(4) -- the final exact Start confirms a match from the Rules row.
elseif journey=='input'then
    -- These holds deliberately have no release between adjacent actions.
    -- Native repeat preserves its fast flag across nonzero word changes.
    down(4);add('a','open_rules',260)
    add('left','rule_bar_hold_then_change_without_release',180,180)
    add('right','rule_bar_fast_preserved_on_right',120,120)
    add('left+right','opposing_horizontal_ignored',70,70)
    add('right','single_right_after_combination',210,180)
    add('up+down','opposing_vertical_ignored',110,80)
    add('left+start','adjust_confirm_combination_ignored',110,80)
    add('a+start','two_confirm_buttons_ignored',110,80)
    add('b+right','cancel_adjust_combination_ignored',110,80)
    down(2);add('right','boolean_repeat_without_acceleration',210,180)
    add('left+right','boolean_combination_ignored',110,80)
    add('left','boolean_reverse_repeat',210,180)
    add('start','commit_rules',260)
    down(5);add('a','open_options',260)
    add('right','option_fast_hold_without_release',120,120)
    add('down','option_row_change_without_release',1,1)
    add('left','option_fast_flag_with_acceleration_cleared',130,100)
    add('left+right','options_opposing_ignored',110,80)
    add('a+start','options_two_confirm_buttons_ignored',110,80)
    add('start','commit_options',260)
    add('a+start','main_two_confirm_buttons_ignored',110,80)
elseif journey=='load'then
    add('none','observe_loaded_setup',60,0)
elseif journey=='main'then
    add('up','main_top_boundary')
    add('down','main_bottom_boundary')
    for row,count in ipairs({4,3,3,4})do
        for _,key in ipairs({'right','left'})do
            for edge=1,count do add(key,'main'..(row-1)..'_cycle_'..key..'_'..edge)end
        end
        if row<4 then down(1)end
    end
    -- Leave nonfactory Level/Quarter values in working state, then prove
    -- the normal Options entry commits Main before replacing its buffer.
    add('right','quarter_12_to_3');add('up','level_row')
    add('right','level_rookie_to_starter');down(3)
    add('a','open_options',260);add('start','normal_options_start',260)
    add('up','main_up_after_return');add('down','main_down_after_return')
elseif journey=='held'then
    down(4);add('a','open_rules',260)
    for row=0,1 do
        add('left','held_rule'..row..'_left_180',210,180)
        add('right','held_rule'..row..'_right_180',210,180)
        add('left','held_rule'..row..'_left_180',210,180)
        if row==0 then down(1)end
    end
    add('start','commit_rules',260)
    down(5);add('a','open_options',260)
    for row=0,1 do
        add('right','held_option'..row..'_right_180',210,180)
        add('left','held_option'..row..'_left_180',210,180)
        if row==0 then down(1)end
    end
    add('start','commit_options',260)
elseif journey=='presets'then
    down(1)
    for _,key in ipairs({'right','right','right','left','left','left','right'})do
        add(key,'style_cycle')
    end
    down(3);add('a','open_rules',260)
    add('right','clamp_simulation_max_marks_custom')
    add('b','ignored_B')
    add('left','edit_defensive_fouls')
    add('start','commit_rules',260)
    down(1)
    for _,key in ipairs({'left','left','left','right','right','right'})do
        add(key,'style_cycle_saved_custom')
    end
    down(3);add('a','reenter_rules',260);add('start','commit_unchanged_rules',260)
elseif journey=='rules'then
    down(4);add('a','open_rules',260)
    add('up','cursor_top_boundary')
    for row=0,1 do
        add('left','rule'..row..'_clamp_min')
        for value=1,45 do add('right','rule'..row..'_increase_'..value)end
        add('right','rule'..row..'_clamp_max')
        for value=44,0,-1 do add('left','rule'..row..'_decrease_'..value)end
        down(1)
    end
    for row=2,12 do
        for _,key in ipairs({'right','right','left','left'})do
            add(key,'rule'..row..'_cycle_'..key)
        end
        if row<12 then down(1)end
    end
    add('down','cursor_bottom_boundary')
    add('b','ignored_B');add('start','commit_rules',260)
    down(4);add('a','reenter_rules',260);add('start','commit_unchanged_rules',260)
else
    down(5);add('a','open_options',260)
    for row=0,1 do
        for n=1,31 do add('left','option'..row..'_decrease_to_clamp_'..n)end
        for n=1,46 do add('right','option'..row..'_increase_to_clamp_'..n)end
        for n=1,45 do add('left','option'..row..'_decrease_'..n)end
        down(1)
    end
    for row=2,6 do
        local count=row==2 and 3 or 2
        for _,key in ipairs({'right','left'})do
            for n=1,count do add(key,'option'..row..'_cycle_'..key..'_'..n)end
        end
        if row<6 then down(1)end
    end
    add('b','ignored_B');add('start','commit_options',260)
    down(5);add('a','reenter_options',260);add('start','commit_unchanged_options',260)
end
if journey~='load'then add('start','confirm_exhibition_and_serialize',260)end
local f=assert(io.open(out..'/actions.json','wb'));f:write(json(actions));f:close()
local function word(address)
    return emu.read(address,emu.memType.snesWorkRam)|
           emu.read(address+1,emu.memType.snesWorkRam)<<8
end
local function words(address,count)
    local result={};for i=0,count-1 do result[#result+1]=word(address+2*i)end
    return result
end
local function snapshot(kind,pc)
    local state=emu.getState();local sram={}
    for address=0x48,0x56 do
        sram[#sram+1]=emu.read(0x700000+address,emu.memType.snesMemory,false)
    end
    local controller=word(0x1615)
    return {kind=kind,pc=pc or 0,frame=frame,setup_frame=setup_frame,
        action=action_frame>=0 and action_index or 0,
        main=words(0x17ab,4),working=words(0x16fb,13),
        rules=words(0x17d1,13),options=words(0x17b5,7),sram48_56=sram,
        sram_marker=emu.read(0x700004,emu.memType.snesMemory,false),
        row=word(0x1693),value=word(0x1695),maximum=word(0x1697),
        controller=controller,repeat_input=word(0x15e3+controller),
        held_input=word(0x576+controller),previous_input=word(0x15cf+controller),
        pending_input=word(0x15d9+controller),repeat_delay=word(0x15ed+controller),
        repeat_speed=word(0x1601+controller),
        repeat_flag=word(0x1639),dirty=word(0x1645),original_style=word(0x140b),
        a=state['cpu.a'],x=state['cpu.x'],y=state['cpu.y'],p=state['cpu.ps'],
        brightness=state['ppu.screenBrightness'],forced_blank=state['ppu.forcedBlank']}
end
local function event(kind,pc)
    events:write(json(snapshot(kind,pc))..'\n');events:flush()
end
for _,pc in ipairs({0x81c19a,0x81c1a9,0x81c232,0x81c24a,0x81c24b,
                   0x81bfaa,0x81c00b,0x81c398,0x81c3d3,0x81c3d5,0x81c41d,
                   0x81bed5,0x81bee6,0x81bf59,0x81bf6a,
                   0x81d446,0x81d4c0,0x81d47a,0x81d494,0x81d4a9,0x81d4fa,0x81d52f,0x81d53b,
                   0x828d92,0x828e5f,0x828dc6,0x828dda,0x828d0a,
                   0x828eb3,0x828ecb,0x828ed4,0x828ee4})do
    emu.addMemoryCallback(function()event('boundary',pc)end,
        emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
emu.addMemoryCallback(function()
    if title_frame<0 then title_frame=0 end
end,emu.callbackType.exec,0x80e1b1,0x80e1b1,emu.cpuType.snes,emu.memType.snesMemory)
emu.addMemoryCallback(function()
    if title_frame>=850 and setup_frame<0 then setup_frame=0 end
end,emu.callbackType.exec,0x80a2bf,0x80a2bf,emu.cpuType.snes,emu.memType.snesMemory)
emu.addEventCallback(function()
    local input={}
    if setup_frame<0 then
        if title_frame>=850 and title_frame<853 then input.start=true end
    elseif action_frame>=0 and actions[action_index]then
        local action=actions[action_index]
        if action_frame<action.hold then
            for key in action.key:gmatch('[^+]+')do
                if key~='none'then input[key]=true end
            end
        end
    end
    emu.setInput(input,0)
end,emu.eventType.inputPolled)
local function record_stage(kind)
    stages:write(json(snapshot(kind))..'\n');stages:flush()
    if (journey=='faces' or journey=='main') and kind~='before_action'then
        local prefix=out..'/visual_'..(kind=='initial_setup' and 0 or action_index)
        local screen=emu.getScreenBuffer()
        assert(#screen==256*239,'unexpected native screen geometry')
        local pixels={}
        for y=7,230 do for x=0,255 do
            local c=screen[y*256+x+1]
            pixels[#pixels+1]=string.char((c>>16)&255,(c>>8)&255,c&255)
        end end
        local f=assert(io.open(prefix..'.rgb','wb'));f:write(table.concat(pixels));f:close()
        for _,spec in ipairs({{'vram',emu.memType.snesVideoRam,65536},
                              {'cgram',emu.memType.snesCgRam,512}})do
            local bytes={}
            for i=0,spec[3]-1 do bytes[#bytes+1]=string.char(emu.read(i,spec[2],false))end
            f=assert(io.open(prefix..'_'..spec[1]..'.bin','wb'))
            f:write(table.concat(bytes));f:close()
        end
    end
end
emu.addEventCallback(function()
    frame=frame+1
    if setup_frame<0 then
        if title_frame>=0 then title_frame=title_frame+1 end
        return
    end
    setup_frame=setup_frame+1
    if setup_frame==400 then
        record_stage('initial_setup');action_frame=0;record_stage('before_action')
        return
    end
    if action_frame<0 then return end
    action_frame=action_frame+1
    local action=actions[action_index]
    if action_frame==action.wait then
        record_stage('after_action')
        action_index=action_index+1
        if not actions[action_index]then
            events:close();stages:close()
            local done=assert(io.open(out..'/capture_complete.txt','wb'))
            done:write('actions='..#actions..'\n');done:close();emu.stop(0)
        else action_frame=0;record_stage('before_action')end
    end
end,emu.eventType.endFrame)
