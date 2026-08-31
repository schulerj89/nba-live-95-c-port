import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
 b=p.read_bytes();return dict(path=str(p.resolve()),bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def main():
 out=ROOT/'.analysis/period-roles-freeze-v3.json';assert not out.exists();prior=ROOT/'.analysis/period-roles-freeze-v2.json'
 assert identity(prior)['sha256']=='c8ba8e82c18518f788871656a4410a3f75ffbe3ac2394e7680b6adedcb19b2c6'
 files=json.loads(prior.read_text())['files']
 for n,w in files.items():assert identity(Path(w['path']))==w,n
 for p in (ROOT/'.analysis/period-roles-v2-native-v1').iterdir():
  if p.suffix in ('.input','.jsonl'):assert (ROOT/'.analysis/period-roles-v3-native-v1'/p.name).read_bytes()==p.read_bytes()
 files['.analysis/period-roles-freeze-v2.json']=identity(prior)
 for n in ['tools/verify_period_roles_v3.py','tools/test_period_roles_verifier_v3.py','tools/freeze_period_roles_v3.py','docs/period-role-verifier-v3.md']:files[n]=identity(ROOT/n)
 for d in ['period-roles-v3-independent-v1','period-roles-v3-native-v1']:
  for p in (ROOT/'.analysis'/d).rglob('*'):
   if p.is_file():files[p.relative_to(ROOT).as_posix()]=identity(p)
 p=ROOT.parent/'completion-auditor/tools/test_period_roles_protocol_audit.py';files['auditor/protocol_tool.py']=identity(p)
 p=ROOT.parent/'completion-auditor/build/period-roles-audit-v2/independent-protocol-v1/report.json';files['auditor/original_rejection.json']=identity(p)
 out.write_text(json.dumps(dict(schema=1,scope='every-row projection-width verifier fix only; original C/native/v2 unchanged; independent review pending',files=files),indent=2)+'\n');print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
