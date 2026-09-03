"""Freeze new period continuation files, preserving every v1 identity."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
 b=p.read_bytes();return dict(path=str(p.resolve()),bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def main():
 out=ROOT/'.analysis/period-roles-freeze-v2.json'
 if out.exists():raise ValueError('freeze exists')
 previous=ROOT/'.analysis/period-roles-freeze-v1.json'
 assert identity(previous)['sha256']=='b0926909ac925598d0e4a0216cb2437d29c3919e634ec919125eb5abbd55f790'
 old=json.loads(previous.read_text());files={}
 for key,want in old['files'].items():
  assert identity(Path(want['path']))==want,('prior identity changed',key);files['v1/'+key]=want
 files['v1/freeze.json']=identity(previous)
 names=['include/nba_period_roles_v2.h','src/nba_period_roles_v2.c','tools/period_roles_probe_v2.c','tools/period_roles_probe_fields_v2.inc','tools/build_period_roles_probe_v2.ps1','tools/verify_period_roles_v2.py','tools/test_period_roles_v2.py','tools/period_roles_rom_reference_v2.py','tools/test_period_roles_protocol_v2.py','tools/freeze_period_roles_v2.py']
 for name in names:files[name]=identity(ROOT/name)
 for directory in ['period-roles-v2-build-v1','period-roles-v2-native-v1','period-roles-v2-tests-v1','period-roles-v2-protocol-v1']:
  for p in sorted((ROOT/'.analysis'/directory).rglob('*')):
   if p.is_file():files[p.relative_to(ROOT).as_posix()]=identity(p)
 out.write_text(json.dumps(dict(schema=1,scope='bounded live81/82 period role continuation; explicit record/assignment stops; no production/timing/normal-state acceptance; independent audit pending',previous_unchanged=714,files=files),indent=2)+'\n')
 print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
