"""C host-disabled and malformed-input regressions, separate from ROM replay."""
import argparse,json,subprocess
from pathlib import Path
from normalize_audio_events import read_fixture,require
from verify_audio_events import cases,input_text

def main():
    p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--fixture',required=True);a=p.parse_args()
    rows=cases(read_fixture(a.fixture))
    result=subprocess.run([a.probe,'--discard-operations'],input=input_text(rows),text=True,capture_output=True)
    require(result.returncode==0,'discard regression probe failed')
    output=[json.loads(s) for s in result.stdout.splitlines()]
    require(len(output)==len(rows),'discard population changed')
    for i,(got,(_,native)) in enumerate(zip(output,rows)):
        require(got==dict(id=i,output=native['output'],returns_consumed=0,operations=[]),'disabled host changed logical event/RNG state')
    for bad in ('','0 1 0 0 1 0 0\n','0 1 0 0 1 0 1\n','0 -1 0 0 1 0 0\n',
                '0 0 0 0 1 65536 0\n','0 0 0 0 1 0 0 extra\n','0 0 0 0 1 0 15\n'):
        if not bad:continue # An empty process has no accepted cases; the verifier rejects its output.
        run=subprocess.run([a.probe],input=bad,text=True,capture_output=True)
        require(run.returncode!=0,'malformed/missing callee input accepted')
    print('PASS C discard/malformed-input regressions; NMI/runtime caller wiring is not tested')
if __name__=='__main__':main()
