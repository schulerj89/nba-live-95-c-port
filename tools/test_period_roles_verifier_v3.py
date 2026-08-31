"""Run auditor's unchanged v2-importing cases against the new v3 verifier."""
import argparse,hashlib,runpy,sys
from pathlib import Path
import verify_period_roles_v3 as repaired

def main():
 p=argparse.ArgumentParser()
 for k in ('audit_tool','rom','exe','native','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();sys.modules['verify_period_roles_v2']=repaired
 sys.argv=[str(a.audit_tool),'--source',str(Path(__file__).resolve().parents[1]),'--rom',str(a.rom),'--exe',str(a.exe),'--native',str(a.native),'--output',str(a.output)]
 runpy.run_path(str(a.audit_tool),run_name='__main__')
if __name__=='__main__':main()
