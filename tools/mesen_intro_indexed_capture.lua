-- Investigation capture: untouched cold boot, no input or machine writes.
local out=assert(os.getenv('NBA95_CAPTURE_DIR'))
local home=assert(io.open(out..'/observed-script-data-folder.txt','wb'))
home:write(emu.getScriptDataFolder()..'\n');home:close()
local frames=assert(io.open(out..'/frames.jsonl','wb'))
local marks=assert(io.open(out..'/marks.jsonl','wb'))
local global,license,motion=0,-1,-1
local limit=tonumber(os.getenv('NBA95_INTRO_LIMIT')) or 1000
local function hook(pc,label)
 emu.addMemoryCallback(function()
  marks:write(string.format('{"global":%d,"license":%d,"motion":%d,"pc":%d,"label":"%s"}\n',global,license,motion,pc,label));marks:flush()
  if pc==0x80FD9E and license<0 then license=0 end
  if pc==0x82F2EA and motion<0 then motion=0 end
 end,emu.callbackType.exec,pc,pc,emu.cpuType.snes,emu.memType.snesMemory)
end
local function memory_snapshot(name,state)
 for _,memory in ipairs({{'vram',emu.memType.snesVideoRam,0x10000},{'cgram',emu.memType.snesCgRam,0x200},{'oam',emu.memType.snesSpriteRam,0x220}})do
  local bytes={};for a=0,memory[3]-1 do bytes[#bytes+1]=string.char(emu.read(a,memory[2],false))end
  local f=assert(io.open(out..'/'..name..'.'..memory[1],'wb'));f:write(table.concat(bytes));f:close()
 end
 local f=assert(io.open(out..'/'..name..'-state.txt','wb'))
 local keys={};for k,v in pairs(state)do if k:sub(1,4)=='ppu.' and type(v)~='table'then keys[#keys+1]=k end end
 table.sort(keys);for _,k in ipairs(keys)do f:write(k..'='..tostring(state[k])..'\n')end;f:close()
end
hook(0x80FD9E,'license-builder');hook(0x80FE7B,'license-hold');hook(0x80FEE6,'legal-hold')
hook(0x80FF03,'legal-start-poll-hold');hook(0x80FF28,'legal-fade-out')
hook(0x82F2EA,'ea-motion');hook(0x82F2FE,'e-complete')
hook(0x82F37E,'a-complete');hook(0x82F43A,'sports-complete')
hook(0x82F492,'ea-hold');hook(0x80E1B1,'title-entry')
emu.addEventCallback(function()
 if license>=0 then
  local p=emu.getScreenBuffer();assert(#p==256*239)
  local b={};for i,c in ipairs(p)do b[i]=string.char((c>>16)&255,(c>>8)&255,c&255)end
  local name=string.format('frame_%04d.rgb',license)
  local f=assert(io.open(out..'/'..name,'wb'));f:write(table.concat(b));f:close()
  if motion>=0 and motion<=134 then
   for _,memory in ipairs({{'cgram',emu.memType.snesCgRam,0x200},{'oam',emu.memType.snesSpriteRam,0x220}})do
    local bytes={};for a=0,memory[3]-1 do bytes[#bytes+1]=string.char(emu.read(a,memory[2],false))end
    local f=assert(io.open(out..string.format('/ea_%03d.%s',motion,memory[1]),'wb'));f:write(table.concat(bytes));f:close()
   end
  end
  local s=emu.getState()
  if license==8 then memory_snapshot('license',s)end
  if license==169 then memory_snapshot('legal',s)end
  if motion==0 or motion==23 or motion==33 or motion==34 or motion==55 or motion==56 or motion==66 or motion==67 or motion==90 or motion==100 or motion==130 then memory_snapshot(string.format('ea_%03d',motion),s)end
  frames:write(string.format('{"global":%d,"license":%d,"motion":%d,"brightness":%d,"blank":%s,"main":%d,"sub":%d,"m7a":%d,"m7d":%d,"m7x":%d,"m7y":%d,"m7h":%d,"m7v":%d}\n',global,license,motion,s['ppu.screenBrightness'],s['ppu.forcedBlank'] and 'true' or 'false',s['ppu.mainScreenLayers'],s['ppu.subScreenLayers'],s['ppu.mode7.matrix[0]'],s['ppu.mode7.matrix[3]'],s['ppu.mode7.centerX'],s['ppu.mode7.centerY'],s['ppu.mode7.hscroll'],s['ppu.mode7.vscroll']));frames:flush()
  license=license+1
  if motion>=0 then motion=motion+1 end
  if license>=limit then
   frames:close();marks:close()
   local f=assert(io.open(out..'/complete.txt','wb'));f:write(tostring(limit)..'\n');f:close()
   emu.stop(0)
  end
 end
 global=global+1
 if global>4000 then emu.stop(1);error('license landmark missing')end
end,emu.eventType.endFrame)
