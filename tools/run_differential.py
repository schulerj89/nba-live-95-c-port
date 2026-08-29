"""Fresh configured-launch baseline + strict native/C sweep comparison.

Exit1 is a state mismatch, not a successful equivalence run. Exit2 means the
capture/contract is invalid or its context remains unproven. This v1 does not
import a RAM image.
"""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from differential_compare import CONTRACT,SCHEMA,compare,digest,load,schema

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser();p.add_argument('--rom',required=True);p.add_argument('--pack',required=True)
    p.add_argument('--output',required=True);p.add_argument('--sweeps',type=int,default=12)
    p.add_argument('--controllers',choices=('cpu-vs-human','cpu-vs-cpu'),default='cpu-vs-human',
                   help='native launch; port currently remains CPU-only (default: preserve human setup)')
    p.add_argument('--capture-ball-init',action='store_true',help='also capture the native E056-E0AB entry/exit prefix')
    p.add_argument('--capture-jump-reach',action='store_true',help='capture EC32 decision inputs and native child requests')
    p.add_argument('--jump-runtime',action='store_true',help='also observe jump caller/scratch/timer boundaries')
    p.add_argument('--jump-cases',help='controlled native EC32 input cases; requires --capture-jump-reach')
    p.add_argument('--poison-ball-init',action='store_true',help='controlled native test of prefix output clearing; requires --capture-ball-init')
    p.add_argument('--mesen',default=shutil.which('Mesen.exe'));a=p.parse_args()
    if not 1<=a.sweeps<=1000:p.error('sweeps must be1..1000')
    if a.poison_ball_init and not a.capture_ball_init:p.error('--poison-ball-init requires --capture-ball-init')
    if a.capture_ball_init and a.capture_jump_reach:p.error('choose one bounded capture wrapper')
    if a.jump_cases and not a.capture_jump_reach:p.error('--jump-cases requires --capture-jump-reach')
    if a.jump_runtime and not a.capture_jump_reach:p.error('--jump-runtime requires --capture-jump-reach')
    out=Path(a.output).resolve()
    if out.exists():p.error('output must be a new directory; no stale captures may be reused')
    out.mkdir(parents=True)
    manifest=dict(contract=CONTRACT,scope='partial-state baseline gate, no snapshot import',sweeps=a.sweeps,
        launch=dict(teams=[3,18],native_controllers=a.controllers,port_controllers='cpu-vs-cpu',
                    controller_mode_matches=a.controllers=='cpu-vs-cpu',input_masks=[0]*5,
                    configuration='pre-game team/Exhibition writes; optional CPU-only selection writes; other setup defaults not synchronized'),
        schema_sha256=digest(SCHEMA),sources={})
    result={};code=2
    try:
        rom=Path(a.rom).resolve();pack=Path(a.pack).resolve();probe=ROOT/'build/differential_runtime_probe.exe'
        for name,path in dict(rom=rom,pack=pack,probe=probe,port=ROOT/'build/nba95_port.exe',
                              capture=ROOT/'tools/mesen_differential_capture.lua',
                              comparator=ROOT/'tools/differential_compare.py',runner=Path(__file__)).items():
            manifest['sources'][name]=dict(path=str(path),sha256=digest(path))
        if not a.mesen:raise ValueError('Mesen.exe not found')
        manifest['sources']['mesen']=dict(path=str(Path(a.mesen).resolve()),sha256=digest(a.mesen))
        manifest['git_head']=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
        manifest['git_status']=subprocess.check_output(['git','status','--short'],cwd=ROOT,text=True)
        (out/'addresses.txt').write_text('\n'.join(schema())+'\n')
        # Existing ROM/pack loader rejects mismatched identity before either run.
        with (out/'preflight.log').open('w') as log:
            subprocess.run([str(ROOT/'build/nba95_port.exe'),'--headless','--rom',str(rom),
                            '--assets',str(pack),'--frames','0'],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
        with (out/'port.log').open('w') as log:
            subprocess.run([str(probe),str(pack),str(out/'port.jsonl'),str(a.sweeps)],
                            stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
        # Clean inherited capture options; no prior controlled-vector switches.
        env={k:v for k,v in os.environ.items() if not k.startswith(('NBA95_VEC_','NBA95_TIP_','NBA95_DIFF_'))}
        capture=ROOT/'tools/mesen_differential_capture.lua'
        env.update(NBA95_CAPTURE_DIR=str(out),NBA95_DIFF_SWEEPS=str(a.sweeps),NBA95_DIFF_CONTROLLERS=a.controllers,
                   NBA95_DIFF_DRIVER=str(capture))
        if a.capture_ball_init:
            from verify_ball_init_differential import fields
            (out/'ball-init-addresses.txt').write_text('\n'.join(fields(0x34e7))+'\n')
            env['NBA95_DIFF_INIT_POISON']='1' if a.poison_ball_init else '0'
            manifest['ball_init_controlled']=a.poison_ball_init
            capture=ROOT/'tools/mesen_differential_slice.lua'
            manifest['sources']['slice_capture']=dict(path=str(capture),sha256=digest(capture))
        if a.capture_jump_reach:
            capture=ROOT/'tools/mesen_jump_reach.lua'
            manifest['sources']['slice_capture']=dict(path=str(capture),sha256=digest(capture))
            if a.jump_runtime:
                env['NBA95_DIFF_JUMP_DRIVER']=str(capture)
                capture=ROOT/'tools/mesen_jump_runtime.lua'
                manifest['sources']['runtime_capture']=dict(path=str(capture),sha256=digest(capture))
            if a.jump_cases:
                cases=Path(a.jump_cases).resolve()
                env['NBA95_DIFF_JUMP_CASES']=str(cases)
                manifest['sources']['jump_cases']=dict(path=str(cases),sha256=digest(cases))
        hidden={}
        if os.name=='nt':
            startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
            hidden['startupinfo']=startup
        with (out/'mesen.log').open('w') as log:
            subprocess.run([a.mesen,'--testrunner','--timeout=180',str(rom),str(capture)],
                           env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=200,**hidden)
        if not (out/'differential_complete.txt').is_file():raise ValueError('Mesen did not complete differential checkpoints')
        if a.capture_jump_reach:
            progress=json.loads((out/'jump-reach-progress.json').read_text())
            calls=load(out/'jump-reach.jsonl')
            if not calls or progress['started']!=progress['completed'] or len(calls)!=progress['completed']:
                raise ValueError('incomplete jump/reach capture')
            if a.jump_cases and len(calls)!=progress['expected_controlled']:
                raise ValueError('not every controlled jump/reach case executed')
            for name in ('jump-reach.jsonl','jump-reach-progress.json'):
                manifest['sources'][name]=dict(sha256=digest(out/name))
        result=compare(load(out/'rom.jsonl'),load(out/'port.jsonl'),a.sweeps)
        # A projection match cannot certify unsynchronized configuration/state.
        if result['status']=='PROJECTION_MATCH':
            result['status']='PROJECTION_MATCH_CONTEXT_UNPROVEN'
            result['note']='No full starting-state/configuration import exists yet; not a gameplay-equivalence PASS.'
            code=2
        else:code=1
        for name in ('rom.jsonl','port.jsonl','baseline.wram'):
            manifest['sources'][name]=dict(sha256=digest(out/name))
        if a.capture_ball_init:
            for name in ('ball-init-entry.wram','ball-init-exit.wram','ball-init-pcs.json','ball-init-meta.json'):
                manifest['sources'][name]=dict(sha256=digest(out/name))
    except (ValueError,TypeError,KeyError,OSError,subprocess.SubprocessError) as error:
        result=dict(status='INVALID_CAPTURE',error=str(error));code=2
        if (out/'capture_error.txt').is_file():
            result['capture_error']=(out/'capture_error.txt').read_text()
    result.update(contract=CONTRACT,schema_sha256=digest(SCHEMA))
    (out/'run.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (out/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'])
    for d in result.get('differences',[])[:12]:print(d)
    print('Report:',out/'report.json');return code
if __name__=='__main__':raise SystemExit(main())
