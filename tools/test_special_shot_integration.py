"""Controlled inputs on both baskets through the real C gameplay dispatcher."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

def main():
    p=argparse.ArgumentParser()
    for name in ('exe','rom','pack'):p.add_argument('--'+name,required=True)
    args=p.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        for actor in (0,5):
            trace=Path(tmp)/f'special_{actor}.jsonl'
            run=subprocess.run([args.exe,'--headless','--rom',args.rom,'--assets',args.pack,
                '--tipoff-only','--frames','3500','--gameplay-special-shot-at','3420',
                '--gameplay-actor',str(actor),'--gameplay-trace',str(trace)],capture_output=True,text=True,check=True)
            if 'CONTROLLED INPUT' not in run.stdout:raise AssertionError('fixture not labeled')
            rows=[json.loads(l) for l in trace.read_text().splitlines()][3419:]
            first=rows[0]['actors'][actor]
            if first['raw']['control_mode']!=17 or first['animation'] not in (0x14,0x15):
                raise AssertionError('selector did not enter special mode')
            release=next((i for i,r in enumerate(rows) if r['ball']['owner']==-1),None)
            if release is None or not 4<=release<60:raise AssertionError('special shot did not release')
            if any(r['ball']['owner']!=actor for r in rows[:release]):raise AssertionError('ownership lost before release')
            if not any(r['actors'][actor]['z']>0 for r in rows[:release]):raise AssertionError('jump missing')
            before,after=rows[release-1],rows[release]
            a=after['actors'][actor]
            if before['actors'][actor]['raw']['animation_rom']['upper_phase_3a']<3 or \
                a['animation']!=first['animation'] or after['match']['shot_value_raw']!=2 or \
                after['match']['shot_actor_raw_09c8']!=actor or after['match']['shot_inner_veto_raw'] not in (0,1):
                raise AssertionError('special release used ordinary pose or wrong shot state')
            if after['ball']['state']!=5 or not any(after['ball'][k] for k in ('vx','vy','vz')):
                raise AssertionError('missing physical ball launch')
            print(f'[SPECIAL INTEGRATION] actor={actor} basket={"left" if actor==0 else "right"} '
                  f'upper={first["animation"]:02x} released={after["frame"]} retained special pose')

if __name__=='__main__':main()
