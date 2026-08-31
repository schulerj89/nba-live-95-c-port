"""Diagnostic named-field projection, not production WRAM storage."""
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 fields=[]
 def add(name,addr,width=2):fields.append((name,addr,width))
 parent=re.findall(r'^(ACTOR|BALL|GLOBAL)\((\w+),(0x[0-9a-f]+)\)$',(ROOT/'tools/period_restart_probe_fields.inc').read_text(),re.M)
 for i in range(10):
  for kind,n,o in parent:
   if kind=='ACTOR':add(f'parent.actors[{i}].{n}',0x34eb+i*256+int(o,16))
 for kind,n,o in parent:
  if kind=='BALL':add('parent.ball.'+n,0x3eeb+int(o,16))
  if kind=='GLOBAL':add('parent.'+n,int(o,16))
 for i in range(12):add(f'parent.object_list[{i}]',0x34d3+2*i)
 for n,a in [('input.period',0x926),('input.tip_winner',0x932),('input.anchor_x[0]',0x46f5),('input.anchor_x[1]',0x4775)]:add(n,a)
 extra={'upper_queue_cursor':0x18,'lower_queue_cursor':0x1a,'upper_state':0x30,'lower_state':0x32,'base_state':0x38,'upper_phase_target':0xb0,'mirror_flags':0x28,'upper_resource':0x2a,'lower_resource':0x2c,'resolved_upper_state':0x34,'resolved_lower_state':0x36,'resolved_upper_phase':0x3e,'resolved_lower_phase':0x40,'alternate_lower':0xa8,'variant':0x6c,'catcher_latch':0xae,'current_assignment':0x74,'base_assignment':0x76,'alternate_assignment':0x78,'help':0x80,'role':0x92,'saved_mode':0x84,'pair_direction':0x86,'pair_distance':0x8a,'anchor_distance':0x8c,'depth':0x68}
 for j in range(3):extra[f'upper_queue[{j}]']=0x1c+j*2;extra[f'lower_queue[{j}]']=0x22+j*2
 for i in range(10):
  for n,o in extra.items():add(f'actors[{i}].{n}',0x34eb+i*256+o)
 for side in range(2):
  for n,o in [('team',0),('opponent_pointer',2),('first_actor_pointer',4)]:add(f'contexts[{side}].{n}',0x46eb+128*side+o)
  for i in range(5):
   add(f'contexts[{side}].roster[{i}]',0x46f9+128*side+2*i)
   add(f'contexts[{side}].selector[{i}]',0x159a+side*5+i,1)
   add(f'contexts[{side}].order[{i}]',0x4734+128*side+i,1)
  for i in range(12):add(f'roster_table[{side}][{i}]',0x3471+48*side+4*i,4)
  add(f'controllers.count[{side}]',0x4726+128*side);add(f'controllers.cursor[{side}]',0x4728+128*side)
 for i in range(5):
  names=['group','actor','processed','direction','held','previous','changed','pressed','alternate_direction']+[f'reserved[{j}]'for j in range(23)]
  for j,n in enumerate(names):add(f'controllers.record[{i}].{n}',0x47eb+64*i+2*j)
  add(f'controllers.previous_selection[{i}]',0x1677+2*i)
 for i in range(10):
  add(f'controllers.actor_assignment[{i}]',0x3501+256*i)
  add(f'active_roster_pointer[{i}]',0x3449+4*i,4);add(f'statistic_pointer[{i}]',0x3435+2*i);add(f'assignment_sort_slots[{i}]',0x9da+2*i)
 for n,a in [('ball_assignment',0x3f5f),('ball_alternate_assignment',0x3f63),('ball_anchor_distance',0x3f77),('ball_depth',0x3f53),('extra_draw_x',0x3fef),('extra_draw_y',0x3ff3),('extra_draw_depth',0x4053),('leading_sentinel',0x34d1),('camera_y',0x860),('frame_low',0x84a),('frame_high',0x84c),('delta',0xc6),('rng',0x7f6),('previous_ball_x',0x922),('predicted_x',0x918),('predicted_y',0x91a),('role_cadence',0x9d2),('role_rebuild',0x9d6)]:add(n,a)
 for i in range(12):add(f'draw_order[{i}]',0x7e44+2*i)
 seen={}
 for name,addr,width in fields:
  for a in range(addr,addr+width):assert a not in seen,(name,seen.get(a),hex(a));seen[a]=name
 text=''.join(f'FIELD({n},0x{a:04x},{w})\n'for n,a,w in fields)
 (ROOT/'tools/period_formation_fields.inc').write_text(text)
 print(len(fields),'unique fields;',sum(w for _,_,w in fields),'serialized bytes')
if __name__=='__main__':main()
