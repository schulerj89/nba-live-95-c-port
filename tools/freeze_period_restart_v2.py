"""Freeze corrected v2 separately; retain every rejected v1 identity."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
    data=p.read_bytes();return {'path':str(p.resolve()),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
def main():
    out=ROOT/'.analysis/period-restart-freeze-v2.json'
    if out.exists():raise ValueError('freeze already exists')
    old=ROOT/'.analysis/period-restart-freeze-v1.json'
    assert identity(old)['sha256']=='bbb9fddb581a17a43fda57b2647fd149d5480a84f5b179bada7ae802deb233fb'
    files=json.loads(old.read_text())['files']
    for name,want in files.items():assert identity(Path(want['path']))==want,('v1 identity changed',name)
    files[old.relative_to(ROOT).as_posix()]=identity(old)
    for name in ('include/nba_period_restart_v2.h','src/nba_period_restart_v2.c','tools/period_restart_probe_v2.c','tools/build_period_restart_v2.ps1','tools/verify_period_restart_v2.py','tools/period_restart_source_reference_v2.py','tools/test_period_restart_v2.py','tools/freeze_period_restart_v2.py','.analysis/period-restart-v2-preservation.json'):
        files[name]=identity(ROOT/name)
    for name in ('period-restart-v2-build-v1','period-restart-v2-native-v1','period-restart-v2-tests-v1','period-restart-v2-independent-ROM-v1','period-restart-v2-independent-domain-v1'):
        for p in sorted((ROOT/'.analysis'/name).rglob('*')):
            if p.is_file():files[p.relative_to(ROOT).as_posix()]=identity(p)
    auditor=ROOT.parent/'completion-auditor'
    for name in ('tools/test_period_formation_rom_audit.py','tools/test_period_native_domain_audit.py'):
        files['auditor/'+name]=identity(auditor/name)
    out.write_text(json.dumps({'schema':1,'scope':'v2 corrects original opening positive-anchor Y transform and native caller domain; all v1 files retained unchanged as rejected evidence; bounded parent segments only; independent v2 audit pending; no production or timing acceptance','files':files},indent=2)+'\n')
    print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
