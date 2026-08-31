"""Retain v3 identity closure and add only separately named v4 evidence."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
    data=p.read_bytes();return {'path':str(p.resolve()),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
def main():
    for kind in ('init','control'):
        out=ROOT/f'.analysis/spc-{kind}-freeze-v4.json'
        if out.exists():raise ValueError('freeze already exists: '+str(out))
    for kind in ('init','control'):
        prior=ROOT/f'.analysis/spc-{kind}-freeze-v3.json';files=json.loads(prior.read_text())['files']
        for name,want in files.items():
            assert identity(Path(want['path']))==want,('old frozen identity changed',name)
        files[prior.relative_to(ROOT).as_posix()]=identity(prior)
        names=[f'tools/verify_setup_spc_{kind}_v4.py','tools/setup_spc_state_contract_v4.py','tools/test_setup_spc_state_contract_v4.py','tools/freeze_setup_spc_boundaries_v4.py','docs/setup-spc-boundary-verifier-v4.md','.analysis/spc-v4-preservation-final.json','.analysis/spc-state-contract-v4-tests-final/report.json']
        for n in names:files[n]=identity(ROOT/n)
        for path in (ROOT/'.analysis/spc-state-schema-reference-v1').iterdir():
            if path.is_file():files[path.relative_to(ROOT).as_posix()]=identity(path)
        for mode in ('init','control'):
            p=ROOT/f'.analysis/native-spc-{mode}-v1'
            for path in [p/'manifest.json',*p.glob('*.state')]:files[path.relative_to(ROOT).as_posix()]=identity(path)
        for p in (ROOT/f'.analysis/spc-{kind}-build-v4').iterdir():
            if p.is_file():files[p.relative_to(ROOT).as_posix()]=identity(p)
        for category in ('independent','evidence','protocol','regression'):
            p=ROOT/f'.analysis/spc-{kind}-v4-{category}-final'
            for path in [p/'report.json',*sorted((p/'baseline').iterdir())]:
                if path.is_file():files[path.relative_to(ROOT).as_posix()]=identity(path)
        auditor=ROOT.parent/'completion-auditor'
        for name in ('tools/test_spc_init_control_boundary_audit.py','docs/completion-spc-init-control-independent-audit.md',f'build/spc-{kind}-audit-v3/independent-boundaries/report.json'):
            files['auditor/'+name]=identity(auditor/name)
        out=ROOT/f'.analysis/spc-{kind}-freeze-v4.json'
        out.write_text(json.dumps({'schema':1,'scope':'verifier-only exact SPC scalar state and source callback relation repair; previous source/native/v3 untouched; independent v4 audit pending; no normal DSP/timer/phase acceptance','files':files},indent=2)+'\n')
        print(kind,len(files),identity(out)['sha256'])
if __name__=='__main__':main()
