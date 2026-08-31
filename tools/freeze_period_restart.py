"""Freeze the completed bounded helper packet; never modifies native evidence."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
    data=p.read_bytes()
    return {'path':str(p.resolve()),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
def main():
    out=ROOT/'.analysis/period-restart-freeze-v1.json'
    if out.exists():raise ValueError('freeze already exists')
    native=ROOT.parent/'completion-owner/build/period-restart-native-freeze-v1.json'
    assert identity(native)['sha256']=='04e4c13a1b7298b97fd72fac004e73f58cf6f2eb5bcddf0eaf389eeb404f3d2b'
    original=json.loads(native.read_text());assert len(original['files'])==573
    files={}
    for key,want in original['files'].items():
        assert identity(Path(want['path']))==want,('native identity changed',key)
        files['native/'+key]=want
    files['native/freeze.json']=identity(native)
    for name in ('include/nba_period_restart.h','src/nba_period_restart.c','tools/period_restart_probe_fields.inc','tools/period_restart_probe.c','tools/build_period_restart_probe.ps1','tools/verify_period_restart.py','tools/test_period_restart.py','tools/freeze_period_restart.py','docs/period-restart-source-helper.md'):
        files[name]=identity(ROOT/name)
    for directory in ('.analysis/period-restart-build-v4','.analysis/period-restart-native-v2','.analysis/period-restart-tests-v3'):
        for path in sorted((ROOT/directory).rglob('*')):
            if path.is_file():files[path.relative_to(ROOT).as_posix()]=identity(path)
    result={'schema':1,'scope':'typed period formation/restart parent segments; four native component differentials and twenty source-only combinations; explicit excluded children; no production or timing acceptance; independent audit pending','files':files}
    out.write_text(json.dumps(result,indent=2)+'\n')
    print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
