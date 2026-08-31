"""New callback-address guard freeze, preserving all previous source/evidence."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
 b=p.read_bytes();return dict(path=str(p.resolve()),bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def main():
 out=ROOT/'.analysis/spc-control-freeze-v5.json';assert not out.exists()
 prior=ROOT/'.analysis/spc-control-freeze-v4.json';assert identity(prior)['sha256']=='0de81cb230b32f96eee417d98829e4bdbe187b33adc042bb1f33ad89c6500c2c'
 files=json.loads(prior.read_text())['files']
 for k,w in files.items():assert identity(Path(w['path']))==w,k
 files['.analysis/spc-control-freeze-v4.json']=identity(prior)
 for n in ['tools/verify_setup_spc_control_v5.py','tools/setup_spc_control_contract_v5.py','tools/test_setup_spc_control_contract_v5.py','tools/freeze_setup_spc_control_v5.py','docs/setup-spc-control-verifier-v5.md','.analysis/spc-control-v5-preservation-v1.json']:
  files[n]=identity(ROOT/n)
 for directory in ['spc-control-build-v5','spc-control-v5-direct-page-v1','spc-control-v5-independent-v1','spc-control-v5-regression-v1','spc-control-v5-evidence-v1','spc-control-v5-protocol-v1','spc-control-v5-domain-v1']:
  folder=ROOT/'.analysis'/directory
  for path in folder.rglob('*'):
   if path.is_file():files[path.relative_to(ROOT).as_posix()]=identity(path)
 auditor=ROOT.parent/'completion-auditor'
 for n in ['tools/test_spc_control_direct_page_audit.py','docs/completion-spc-init-control-v4-independent-audit.md','build/spc-control-audit-v4/independent-directpage-v1/report.json']:
  files['auditor/'+n]=identity(auditor/n)
 out.write_text(json.dumps(dict(schema=1,scope='native callback PS.P address guard only; C/native/v4 unchanged; independent v5 audit pending',files=files),indent=2)+'\n')
 print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
