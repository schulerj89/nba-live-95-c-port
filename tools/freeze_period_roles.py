"""Freeze bounded role-prefix source/proof while rechecking earlier packets."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
    data=p.read_bytes();return {'path':str(p.resolve()),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
def main():
    out=ROOT/'.analysis/period-roles-freeze-v1.json'
    if out.exists():raise ValueError('freeze already exists')
    unchanged=[]
    for name in ('period-restart-freeze-v2.json','spc-init-freeze-v4.json','spc-control-freeze-v4.json'):
        p=ROOT/'.analysis'/name;m=json.loads(p.read_text())
        for key,want in m['files'].items():assert identity(Path(want['path']))==want,('earlier identity changed',key)
        unchanged.append({'freeze':name,'sha256':identity(p)['sha256'],'unchanged_identities':len(m['files'])})
    preservation=ROOT/'.analysis/period-roles-preservation-v1.json'
    if preservation.exists():raise ValueError('preservation record already exists')
    preservation.write_text(json.dumps({'passed':True,'previous':unchanged},indent=2)+'\n')
    native=ROOT.parent/'completion-owner/build/period-restart-native-freeze-v1.json'
    assert identity(native)['sha256']=='04e4c13a1b7298b97fd72fac004e73f58cf6f2eb5bcddf0eaf389eeb404f3d2b'
    files={}
    for key,want in json.loads(native.read_text())['files'].items():
        assert identity(Path(want['path']))==want,('native identity changed',key);files['native/'+key]=want
    files['native/freeze.json']=identity(native)
    for name in ('include/nba_period_roles.h','src/nba_period_roles.c','tools/period_roles_probe.c','tools/period_roles_probe_fields.inc','tools/build_period_roles_probe.ps1','tools/verify_period_roles.py','tools/test_period_roles.py','tools/freeze_period_roles.py','tools/verify_period_restart_v2.py','docs/period-role-prefix-source-helper.md','.analysis/period-roles-preservation-v1.json','.analysis/period-restart-freeze-v2.json'):
        files[name]=identity(ROOT/name)
    for directory in ('period-roles-build-v3','period-roles-native-v1','period-roles-tests-v1'):
        for p in sorted((ROOT/'.analysis'/directory).rglob('*')):
            if p.is_file():files[p.relative_to(ROOT).as_posix()]=identity(p)
    p=ROOT.parent/'completion-auditor/docs/completion-period-restart-v2-independent-audit.md'
    files['auditor/accepted-native-guard-review.md']=identity(p)
    out.write_text(json.dumps({'schema':1,'scope':'paired BC07 initial scan/F34F/cadence only; typed before-state native differential; explicit BD0D/BE06 stops; no full planner, production or timing acceptance; independent audit pending','files':files},indent=2)+'\n')
    print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
