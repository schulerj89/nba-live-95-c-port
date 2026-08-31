"""Adapted prior independent parsed-view endpoint (terminalPC8184 only changed) and process-output guards. No frozen-file edits."""
import argparse,contextlib,importlib.util,io,json,sys
from pathlib import Path
from unittest.mock import patch
def main():
 p=argparse.ArgumentParser()
 for key in ('verifier','native','rom','exe','decoder-root','output'):p.add_argument('--'+key,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);sys.path.insert(0,str(a.verifier.parent))
 spec=importlib.util.spec_from_file_location('fill_audit_v',a.verifier);v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
 old_rows=v.rows;old_run=v.subprocess.run
 cases=['baseline','native_terminal_a','native_terminal_x','native_terminal_y','native_terminal_ps','native_terminal_db','native_terminal_dp_bool','native_terminal_emulation','stdout_error_prefix','stdout_duplicate_boundary','stdout_missing_boundary','stdout_changed_boundary']
 result=[]
 for case in cases:
  hit=[False]
  def rows(path):
   d=old_rows(path)
   if Path(path).name=='cpu.jsonl'and case.startswith('native_terminal_'):
    e=d[-1];assert e['pc']==0x808184;key=case[len('native_terminal_'):]
    if key=='dp_bool':e['dp']=False
    elif key=='emulation':e[key]=not e[key]
    else:e[key]^=1
    hit[0]=True
   return d
  def run(*args,**kwargs):
   r=old_run(*args,**kwargs)
   if case.startswith('stdout_'):
    lines=r.stdout.splitlines();indices=[i for i,s in enumerate(lines)if s.startswith('BOUNDARY ')];assert len(indices)==2
    if case=='stdout_error_prefix':lines.insert(0,'ERROR: incomplete source work')
    elif case=='stdout_duplicate_boundary':lines.insert(indices[0],lines[indices[0]])
    elif case=='stdout_missing_boundary':lines.pop(indices[0])
    else:lines[indices[0]]=lines[indices[0]].replace('upload=1264','upload=0')
    r.stdout='\n'.join(lines)+'\n';hit[0]=True
   return r
  argv=['verify','--native',str(a.native.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve()),'--decoder-root',str(a.decoder_root.resolve()),'--output',str((a.output/case).resolve())]
  outcome='accepted';reason=''
  try:
   with patch.object(sys,'argv',argv),patch.object(v,'rows',side_effect=rows),patch.object(v.subprocess,'run',side_effect=run),contextlib.redirect_stdout(io.StringIO()):v.main()
  except(ValueError,AssertionError)as e:outcome='rejected';reason=str(e)
  passed=outcome==('accepted'if case=='baseline'else'rejected')and(case=='baseline'or hit[0])
  result.append(dict(case=case,passed=passed,mutation_reached=hit[0],outcome=outcome,reason=reason));print(case,passed,flush=True)
 (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');return 0 if all(r['passed']for r in result)else 1
if __name__=='__main__':raise SystemExit(main())
