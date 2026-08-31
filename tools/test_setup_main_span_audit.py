"""Controlled old/new Main span checks; native canvas parity is a separate gate."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from PIL import Image


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('old-exe','new-exe','old-canvas','new-canvas','pack','poison','rom','output'):
        p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args(); a.output.mkdir(parents=True,exist_ok=False)
    records=[]
    def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
    def render(exe, pack, label, row, value):
        target=a.output/(label+'.bmp')
        direction,count=('--setup-main-left',3-value) if row==3 else ('--setup-main-right',value)
        command=[str(exe.resolve()),'--headless','--setup-only','--rom',str(a.rom.resolve()),
                 '--assets',str(pack.resolve()),'--frames','200','--setup-main-row',str(row),
                 direction,str(count),'--dump-frame',str(target.resolve())]
        run=subprocess.run(command,text=True,capture_output=True,check=True,timeout=30)
        (a.output/(label+'.log')).write_text(run.stdout+run.stderr)
        # Verify the requested value was actually reached, including all four
        # quarter choices descending from factory12min. Main values wrap;
        # ascending Right journeys also cover all choices in a different order.
        field=('mode','style','level','quarter')[row]
        summary=next(line for line in run.stdout.splitlines() if line.startswith('[SETUP MAIN TEST]'))
        if f'{field}={value}' not in summary: raise AssertionError('input journey did not reach requested value')
        rgb=Image.open(target).convert('RGB').tobytes()
        records.append(dict(label=label,command=command,file_sha256=sha(target),rgb_sha256=hashlib.sha256(rgb).hexdigest()))
        return rgb
    def canvas(exe,pack,label):
        target=a.output/(label+'.bin')
        command=[str(exe.resolve()),str(pack.resolve()),str(target.resolve())]
        run=subprocess.run(command,input='1 0 0 3\n',text=True,capture_output=True,check=True,timeout=30)
        (a.output/(label+'.log')).write_text(run.stdout+run.stderr)
        raw=target.read_bytes(); assert len(raw)==65536
        records.append(dict(label=label,command=command,input='1 0 0 3',file_sha256=sha(target)))
        return raw
    cases=[]
    for row,maximum in enumerate((3,2,2,3)):
        for value in range(maximum+1):
            before=render(a.old_exe,a.pack,f'old-{row}-{value}',row,value)
            after=render(a.new_exe,a.pack,f'new-{row}-{value}',row,value)
            cases.append(dict(name=f'valid Main row{row} value{value}',passed=before==after))
    before=render(a.old_exe,a.pack,'old-clean',0,1)
    poisoned=render(a.old_exe,a.poison,'old-poison',0,1)
    old_changed=sum(before[i:i+3]!=poisoned[i:i+3] for i in range(0,len(before),3))
    cases.append(dict(name='RGB negative control is effective',passed=old_changed>0,changed_pixels=old_changed))
    after=render(a.new_exe,a.pack,'new-clean',0,1)
    poisoned=render(a.new_exe,a.poison,'new-poison',0,1)
    new_changed=sum(after[i:i+3]!=poisoned[i:i+3] for i in range(0,len(after),3))
    cases.append(dict(name='RGB tail ignores source alias',passed=new_changed==0,changed_pixels=new_changed))
    for version,exe in [('old',a.old_canvas),('new',a.new_canvas)]:
        clean=canvas(exe,a.pack,version+'-canvas-clean')
        poisoned=canvas(exe,a.poison,version+'-canvas-poison')
        differences=[i for i,(x,y) in enumerate(zip(clean,poisoned)) if x!=y]
        cases.append(dict(name=version+' raw canvas tail ignores source alias',passed=not differences,differing_addresses=differences))
    report=dict(scope=__doc__,passed=sum(c['passed'] for c in cases),total=len(cases),cases=cases,records=records,
                identities={name:dict(path=str(value.resolve()),sha256=sha(value)) for name,value in vars(a).items() if name!='output'},
                tool_sha256=sha(Path(__file__)))
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','total','cases')}))
    if report['passed']!=report['total']:raise SystemExit(1)


if __name__=='__main__':main()
