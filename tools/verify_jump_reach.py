"""Same-entry native decision replay, NOT full WRAM/child/game equivalence."""
import argparse,hashlib,json,subprocess
from pathlib import Path
from differential_compare import compare,load

FIELDS='actor_x actor_y actor_z lower_state distance direction movement subject_x subject_y subject_z subject_vz subject_direction paired_direction ball_x ball_z ball_vz activity owner receiver live_state block_mode raw_0046 velocity_x velocity_y velocity_z rng rating_3c rating_3d'.split()

def cases():
    base=dict.fromkeys(FIELDS,0)
    base.update(actor_x=8,actor_y=3,subject_z=100,ball_z=100,subject_vz=-100,
                ball_vz=-100,owner=-1,receiver=-1,live_state=0x81,rating_3c=65,rating_3d=75,
                distance=10,rng=1,velocity_x=123,velocity_y=456,velocity_z=789)
    result=[]
    def add(**changes):
        v=base|changes
        # The controlled native subject is the actual ball record.
        for suffix in ('x','z','vz'):
            if 'ball_'+suffix in changes:v['subject_'+suffix]=v['ball_'+suffix]
            else:v['ball_'+suffix]=v['subject_'+suffix]
        result.append([v[k]&65535 for k in FIELDS])
    for key,values in dict(actor_z=[0,1,65535],lower_state=[0,0x32],distance=[0,31,32,33,39,40,65535],
        ball_z=[72,73,80,81,82,83,100,112,113],receiver=[-1,0,9],
        ball_vz=[-600,-64,-63,0,63,64,600],rating_3c=[49,50,65,66,75,255],raw_0046=[0,32768]).items():
        for v in values:add(**{key:v})
    for rng in [0,1,2,4,0x8000,0x8001,0xc000,0xffff]:
        for live in [0x81,0]:
            for block in [0,1]:
                for rating in [65,66]:add(rng=rng,live_state=live,block_mode=block,rating_3c=rating)
    for x,y in [(0,0),(8,3),(-8,3),(8,-3),(-8,-3),(1,2),(2,1),(5,2),(2,5),(1,6),(6,1)]:
        for direction in [0,4,8,12]:add(activity=1,owner=0,actor_x=x,actor_y=y,subject_direction=direction)
        for rating in [65,66]:add(actor_x=x,actor_y=y,live_state=0,rng=1,rating_3c=rating,block_mode=1)
    for owner in [-1,0]:
        for distance in [39,40]:add(activity=1,owner=owner,distance=distance)
    for distance in [32,33]:
        for paired in [0,4]:
            for movement in [63,64]:
                for rng in [0,8]:add(ball_z=72,distance=distance,paired_direction=paired,movement=movement,rng=rng)
    for raw in [0,32768]:
        for x in [-10,10]:
            for rating in [65,66]:add(raw_0046=raw,ball_x=x,live_state=0,block_mode=1,rng=1,rating_3d=rating)
    return result

def verify(rows,probe,pack,channels=False):
    if not rows:raise ValueError('empty native capture')
    for r in rows:
        if len(r['input'])!=28 or len(r['output'])!=4:raise ValueError('invalid field count')
        if any(type(v)!=int or not 0<=v<=65535 for v in r['input']+r['output']):raise ValueError('invalid native word')
        if r['abi'][0:2]!=[0,126] or r['abi'][2]&0x30:raise ValueError('unsupported CPU ABI')
        if any(len(c)!=2 for c in r['calls']) or len(r['calls'])>2:raise ValueError('invalid child list')
        if not r['pcs'] or r['pcs']!=sorted(set(r['pcs'])) or any(type(pc)!=int or not 0x86ec32<=pc<=0x86ee75 for pc in r['pcs']):
            raise ValueError('invalid parent instruction trace')
        if r['input'][26]>255 or r['input'][27]>255:raise ValueError('invalid rating')
        if channels:
            for name,size in [('channels_in',18),('channels_out',18),('animation_options',2)]:
                if len(r[name])!=size or any(type(v)!=int or not 0<=v<=65535 for v in r[name]):raise ValueError('invalid animation state')
    text=''.join(' '.join(map(str,r['input']+(r['channels_in']+r['animation_options'] if channels else [])))+'\n' for r in rows)
    run=subprocess.run([str(probe),str(pack)]+(['--channels'] if channels else []),input=text,text=True,capture_output=True,check=True)
    output=[line for line in run.stdout.splitlines() if not line.startswith('[ASSETS] Loaded asset pack:')]
    if len(output)!=len(rows):raise ValueError('probe record count differs')
    for i,(r,line) in enumerate(zip(rows,output)):
        fields=dict(zip([f'{r["actor"]+offset:04x}' for offset in (14,16,18)]+['07f6'],['vx','vy','vz','rng']))
        v=list(map(int,line.split()));count=v[4]
        end=5+count*2
        if len(v)!=end+(18 if channels else 0):raise ValueError('probe request length')
        initial=dict(zip(fields,r['input'][22:26]))
        def records(final):
            return [dict(sequence=j,checkpoint=p,outer_frame=0,inputs=[0]*5,
                         state=s,writers={}) for j,(p,s) in enumerate([
                             ('jump.entry',initial),('jump.decision',dict(zip(fields,final)))])]
        comparison=compare(records(r['output']),records(v[:4]),0,fields=fields,
                           checkpoint_plan=['jump.entry','jump.decision'])
        commands=[v[j:j+2] for j in range(5,end,2)]
        if comparison['status']!='PROJECTION_MATCH' or commands!=r['calls']:
            raise ValueError(json.dumps(dict(first_mismatch=i,native=r,port=v,comparison=comparison)))
        if channels and not any(c[0]<0x870000 for c in commands) and v[end:]!=r['channels_out']:
            raise ValueError(json.dumps(dict(first_animation_mismatch=i,native=r,port_channels=v[end:])))
    return sorted({p for r in rows for p in r['pcs']})

def main():
    p=argparse.ArgumentParser();p.add_argument('--cases');p.add_argument('--native',action='append',default=[])
    p.add_argument('--probe');p.add_argument('--pack');p.add_argument('--export');p.add_argument('--report')
    p.add_argument('--channels',action='store_true');p.add_argument('--census')
    a=p.parse_args()
    if a.cases:
        rows=cases();Path(a.cases).write_text(''.join(' '.join(map(str,r))+'\n' for r in rows));print('controlled cases:',len(rows));return
    rows=[]
    for f in a.native:rows.extend(load(f))
    pcs=verify(rows,a.probe,a.pack,a.channels)
    if a.census:
        expected=json.loads(Path(a.census).read_text())['pcs']
        if pcs!=expected:raise ValueError('native instruction set differs from Ghidra census')
    report=dict(status='DECISION_PROJECTION_MATCH',calls=len(rows),instruction_starts=len(pcs),pcs=pcs,
                limitations='Not full WRAM replay: animation/EAA8/BD1F children, CPU cycles, caller cadence and whole-game equivalence excluded.',
                sources={f:hashlib.sha256(Path(f).read_bytes()).hexdigest() for f in a.native+[a.probe,a.pack]})
    for path in [Path(__file__),Path(__file__).parents[1]/'src/nba_jump_reach.c']:
        report['sources'][str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    if a.export:
        # Native-only witnesses, retaining every distinct input/output/branch set.
        unique={json.dumps([r['input'],r['output'],r['calls'],r['pcs']]+
                          ([r['channels_in'],r['channels_out'],r['animation_options']] if a.channels else [])):r for r in rows}
        Path(a.export).write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r in unique.values()))
    if a.channels:
        report['animation_channel_calls']=sum(bool(r['calls']) and all(c[0]>=0x870000 for c in r['calls']) for r in rows)
        report['limitations']='18 animation-channel words also replayed using existing production command API. EAA8/BD1F child effects, resolver resources, scratch RAM, cycles, caller cadence and whole-game equivalence excluded.'
    if a.report:Path(a.report).write_text(json.dumps(report,indent=2)+'\n')
    print(report['status'],len(rows),'calls,',len(pcs),'native parent starts; child effects/caller excluded')
if __name__=='__main__':main()
