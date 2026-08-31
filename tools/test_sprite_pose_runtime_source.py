"""Bound the runtime adapter to pose publication and reject ball-state edits."""
import argparse,json,re,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--output',type=Path,required=True);a=p.parse_args();root=a.root.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 source=(root/'src/nba_tipoff.c').read_text()
 body=re.search(r'static void actor_publish_body_mirror\([^}]+\}',source,re.S);assert body
 normalized=' '.join(body.group(0).split())
 assert 'actor->actor_status_raw_28 &= 0x7fffu;' in normalized and 'actor->direction < 3u' in normalized and '0x8000u' in normalized
 assert source.count('actor_publish_body_mirror(')==6
 latch=re.search(r'static void latch_player_screen_origins\(.+?^}',source,re.S|re.M).group(0)
 assert 'if (tipoff->player_screen_visible[actor])' in latch and 'actor_status_raw_28 &= 0xfffbu' in latch and 'head_direction < 3u' in latch
 render=re.search(r'void nba_tipoff_render\(.+?^}',source,re.S|re.M).group(0)
 for token in ('nba_player_sprite_pose_table_inputs','movement_c0 = tipoff->actors[actor].movement_direction','tipoff->actors[actor].direction, &pose','nba_player_sprite_render_pose') : assert token in render
 diff=subprocess.check_output(['git','diff','--unified=0','744809a','--','src/nba_tipoff.c'],cwd=root,text=True)
 changed=[line[1:]for line in diff.splitlines()if line.startswith(('+','-'))and not line.startswith(('+++','---'))]
 forbidden=re.compile(r'\bball\.(?:x_fp|y_fp|z_fp|velocity_[xyz]|owner_actor|state)\b')
 assert not [line for line in changed if forbidden.search(line)]
 report={'passed':True,'mirror_publish_call_sites':5,'cull_owned_head_bit':True,'live_tokens':4,'changed_ball_state_lines':0,'base':'744809a','limits':'Textual ownership guard supplements compiled probes; it is not semantic equivalence proof.'}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: source ownership and zero ball-state edit lines')
if __name__=='__main__':main()
