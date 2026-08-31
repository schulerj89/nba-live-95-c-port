"""Compare composed Setup value canvases against captured native VRAM.

This is a controlled canvas-construction comparison, not a timing/whole-frame
gate or proof that the current captured glyph assets meet production policy.
Inputs are the four Main or thirteen Rules working words. The C process never
receives expected VRAM bytes or hashes. Every 65,536-byte output is compared.
Main covers initial edits before the first submenu; Rules covers independently
identified stable Rules snapshots. Neither mode verifies upload scheduling.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from normalize_setup_config import read_compact,sha
from verify_setup_config_runtime import native_projections


def witnesses(journey,page='main'):
    first=min(e['action'] for e in journey['events'] if e['pc']==0x81bed5)
    if first<1:raise ValueError('missing first native submenu boundary')
    states=[journey['states'][0]]+journey['states'][2::2]
    result=[]
    actions=range(first) if page=='main' else [row['action'] for row in native_projections(journey) if row['page']==1]
    for action in actions:
        row=states[action]
        source=journey['native_manifest']['sources'][f'visual_{action}_vram.bin']
        digest=source['sha256']
        if type(digest) is not str or len(digest)!=64 or any(c not in '0123456789abcdef' for c in digest):
            raise ValueError('invalid native canvas identity')
        result.append({'action':action,'input':row['working'][:4 if page=='main' else 13],
                       'native_sha256':digest,'native_path':source['path']})
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('fixture','probe','pack','output-dir'):parser.add_argument('--'+name,required=True)
    parser.add_argument('--journey',required=True)
    parser.add_argument('--page',choices=('main','rules'),default='main')
    args=parser.parse_args()
    journeys=[j for j in read_compact(args.fixture) if j['name']==args.journey]
    if len(journeys)!=1:raise ValueError('missing/duplicate requested native journey')
    output=Path(args.output_dir).resolve()
    if output.exists():raise ValueError('output directory must be new')
    output.mkdir(parents=True)
    sources={name:{'path':str(Path(getattr(args,name)).resolve()),'sha256':sha(getattr(args,name))}
             for name in ('fixture','probe','pack')}
    results=[]
    for witness in witnesses(journeys[0],args.page):
        target=output/f"canvas-{witness['action']:03d}.bin"
        run=subprocess.run([args.probe,args.pack,str(target)],
            input=' '.join(map(str,witness['input']))+'\n',text=True,
            capture_output=True,check=True,timeout=30)
        if target.stat().st_size!=65536:raise ValueError('incomplete C canvas')
        actual=sha(target)
        results.append(witness|{'actual_path':str(target),'C_sha256':actual,
                                'result':'PASS' if actual==witness['native_sha256'] else 'FAIL'})
    for key,source in sources.items():
        if sha(getattr(args,key))!=source['sha256']:raise ValueError('source changed during replay')
    passed=bool(results) and all(row['result']=='PASS' for row in results)
    report={'result':'PASS' if passed else 'FAIL','scope':__doc__,'page':args.page,'sources':sources,
            'bytes_per_case':65536,'cases':results}
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':report['result'],'cases':len(results),
        'passed':sum(r['result']=='PASS' for r in results),
        'failed_actions':[r['action'] for r in results if r['result']=='FAIL']}))
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
