"""Classify existing menu goldens using native brightness samples, without edits.

Each PASS requires the pre-change binary to reproduce its committed golden,
unchanged brightness/scroll telemetry, and exact native-table mapping of every
pixel. A changed resource, raster boundary or scroll phase must FAIL this gate.
This is conversion-delta evidence, not native whole-transition parity.
"""
import argparse
import ast
import json
from pathlib import Path
import re
import subprocess

import numpy as np
from PIL import Image

from audit_brightness_golden_delta import read_native, sha

SECTIONS = {
    'options-open': ('EXPECTED_OPTIONS_OPEN_TRANSITION_SHA256', 'options', False,
                    [198,219,259,263,267,271,275,279,283,287,299,300,301]),
    'options-return': ('EXPECTED_OPTIONS_RETURN_TRANSITION_SHA256', 'options', True,
                      [315,321,322,323,324,343,355,367,423,431]),
    'rules-return': ('EXPECTED_SUBMENU_TRANSITION_SHA256', 'rules', True,
                    [319,320,329,345,350,351,352,353,382,424,450]),
    'rules-outgoing': ('EXPECTED_SUBMENU_TRANSITION_SHA256', 'rules', False,
                      [198,219,234,242]),
}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('before','after','rom','before-pack','pack','native','baseline-test','output'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--section',choices=SECTIONS,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    inputs={name:dict(path=str(getattr(a,name).resolve()),sha256=sha(getattr(a,name).read_bytes()))
            for name in ('before','after','rom','before_pack','pack','baseline_test')}
    tables,manifest,native_hash=read_native(a.native)
    if inputs['rom']['sha256']!=manifest['sources']['rom']['sha256']:
        raise ValueError('ROM identity mismatch')
    constant,menu,confirm,frames=SECTIONS[a.section]
    values=[ast.literal_eval(n.value) for n in ast.parse(a.baseline_test.read_text(encoding='utf-8-sig')).body
            if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==constant for t in n.targets)]
    if len(values)!=1:raise ValueError('missing/duplicate baseline golden constant')
    goldens=values[0]
    if a.section=='rules-return':goldens={f:h for (direction,f),h in goldens.items() if direction=='close'}
    if a.section=='rules-outgoing':
        goldens={f:h for (direction,f),h in goldens.items() if direction=='open' and f<243}
    if list(goldens)!=frames or any(not isinstance(h,str) or not re.fullmatch('[0-9a-f]{64}',h)
                                    for h in goldens.values()):
        raise ValueError('incomplete/malformed original golden set')
    results=[]
    for frame in frames:
        images=[];states=[]
        for label in ('before','after'):
            destination=a.output/f'{label}-{frame:04d}.bmp'
            pack=a.before_pack if label=='before' else a.pack
            command=[str(getattr(a,label).resolve()),'--headless','--setup-only','--setup-menu',menu,
                     '--rom',str(a.rom.resolve()),'--assets',str(pack.resolve()),'--frames',str(frame),
                     '--dump-frame',str(destination),'--debug-state']
            if confirm:command.append('--setup-menu-confirm')
            run=subprocess.run(command,check=True,text=True,capture_output=True,timeout=60)
            (a.output/f'{label}-{frame:04d}.log').write_text(run.stdout,encoding='utf-8')
            state=re.findall(r'PPU B:(\d+) X1:(-?\d+) X2:(-?\d+) Y2:(-?\d+) Y3:(-?\d+)',run.stdout)
            if len(state)!=1:raise ValueError('missing/duplicate state telemetry')
            states.append(tuple(map(int,state[0])))
            image=np.asarray(Image.open(destination).convert('RGB'))
            if image.shape!=(224,256,3):raise ValueError('invalid framebuffer geometry')
            images.append(image)
        before,after=images
        if sha(before.tobytes())!=goldens[frame]:
            raise ValueError(f'pre-change executable does not reproduce committed frame{frame}')
        issues=[]
        if states[0]!=states[1]:issues.append('brightness/scroll state changed')
        mapped=np.stack([tables[states[0][0],channel,before[:,:,channel]] for channel in range(3)],axis=2)
        if np.any(mapped<0):issues.append('old RGB outside native mapping')
        bad=int(np.count_nonzero(np.any(mapped!=after,axis=2)))
        if bad:issues.append('pixels differ from independently observed conversion')
        row=dict(frame=frame,result='FAIL' if issues else 'PASS',issues=issues,
                 before_state=states[0],after_state=states[1],before_sha256=goldens[frame],
                 changed_pixels=int(np.count_nonzero(np.any(before!=after,axis=2))),unauthorized_pixels=bad)
        if not issues:row['after_sha256']=sha(after.tobytes())
        results.append(row)
    for name,source in inputs.items():
        if sha(getattr(a,name).read_bytes())!=source['sha256']:raise ValueError('source changed during audit')
    passed=sum(row['result']=='PASS' for row in results)
    report=dict(result='PASS' if passed==len(results) else 'FAIL',section=a.section,
                scope='conversion-only pixel mapping; no native transition timing claim',
                sources=inputs,native_raw_sha256=native_hash,rows=results)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f'{a.section}: {passed}/{len(results)} independently authorized conversion-only frames')
    return 0 if passed==len(results) else 1


if __name__=='__main__':raise SystemExit(main())
