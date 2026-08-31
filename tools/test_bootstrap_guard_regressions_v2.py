"""Run an unchanged frozen mutation suite against an explicitly chosen reader.

Only the suite's verifier module import is rebound. Source/C/native data and
each original mutation, assertion and output remain unchanged. The receipt
prints both exact input hashes; this is not a new independent audit.
"""
import argparse,hashlib,importlib.util,json,runpy,sys
from pathlib import Path
from unittest.mock import patch

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--suite',type=Path,required=True)
    p.add_argument('--verifier',type=Path,required=True)
    a,rest=p.parse_known_args();suite=a.suite.resolve();verifier=a.verifier.resolve()
    def identity(path):return dict(path=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    print(json.dumps(dict(suite=identity(suite),verifier=identity(verifier))),flush=True)
    original=importlib.util.spec_from_file_location;hits=[]
    def select(name,location,*args,**kwargs):
        if Path(location).name.startswith('verify_bootstrap'):
            hits.append(str(location));location=verifier
        return original(name,location,*args,**kwargs)
    with patch.object(sys,'argv',[str(suite),*rest]),patch.object(importlib.util,'spec_from_file_location',side_effect=select):
        runpy.run_path(str(suite),run_name='__main__')
    if len(hits)!=1:raise ValueError('Expected exactly one verifier import')

if __name__=='__main__':main()
