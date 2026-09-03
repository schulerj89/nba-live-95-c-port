"""Immutable typed composition packet; no original file or capture is modified."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
 b=p.read_bytes();return dict(path=str(p.resolve()),bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def main():
 out=ROOT/'.analysis/period-formation-freeze-v1.json';assert not out.exists();files={};by_path={}
 def add(name,path):
  item=identity(path)
  if item['path']in by_path:assert files[by_path[item['path']]]==item
  else:files[name]=item;by_path[item['path']]=name
 for name,digest in [('period-restart-freeze-v2.json','02b6365e8e8e06d65e009fb4c6115e644392796e89fd2cb3a3d4eafbeb38a19f'),('period-roles-freeze-v3.json','40f6762fa9310fa4ac83f2f8fc427e689594591952c45ce8e71bd1048f928667')]:
  p=ROOT/'.analysis'/name;assert identity(p)['sha256']==digest
  for n,w in json.loads(p.read_text())['files'].items():
   assert identity(Path(w['path']))==w,('prior frozen file changed',n);add('prior/'+name+'/'+n,Path(w['path']))
  add('prior/'+name,p)
 own=['include/nba_period_formation.h','src/nba_period_formation.c','tools/period_formation_probe.c','tools/period_formation_fields.inc','tools/generate_period_formation_fields.py','tools/build_period_formation_probe.ps1','tools/verify_period_formation.py','tools/test_period_formation.py','tools/test_period_formation_protocol.py','tools/freeze_period_formation.py','.analysis/period-formation-role-alias-map-v1.json','.analysis/period-formation-preservation-v1.json']
 for n in own:add(n,ROOT/n)
 for d in ['period-formation-dependencies-v1','period-formation-build-v1','period-formation-native-v1','period-formation-tests-v2','period-formation-protocol-v1']:
  for p in sorted((ROOT/'.analysis'/d).rglob('*')):
   if p.is_file():add(p.relative_to(ROOT).as_posix(),p)
 owner=ROOT.parent/'completion-owner';auditor=ROOT.parent/'completion-auditor'
 add('original/rom.sfc',Path('F:/Games/SNES/NBA Live 95 (USA).sfc'));add('original/assets.pak',owner/'build/full-extraction-v1/nba95_assets.pak')
 for n in ['build/period-render-tail-freeze-v2.json','build/period-composition-freeze-v1.json','build/period-composition-v1/driver.c','build/period-composition-v1/README.md']:add('owner/'+n,owner/n)
 for n in ['tools/test_period_formation_rom_audit.py']:add('auditor/'+n,auditor/n)
 out.write_text(json.dumps(dict(schema=1,scope='typed DD97/E207 current-state child composition only; explicit unsupported domains; no native after inputs or normal-init/timing/production acceptance; independent audit pending',files=files),indent=2)+'\n');print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
