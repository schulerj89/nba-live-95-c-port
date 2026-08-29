"""Create permanent `$87:92A5-$949E` parent-dispatch witnesses."""
import argparse,json
from pathlib import Path
from verify_violation_parent_vectors import memory,projection
def main():
 p=argparse.ArgumentParser();p.add_argument('--source',action='append',nargs=2,metavar=('LABEL','PATH'),required=True);p.add_argument('--output',required=True);a=p.parse_args();calls=[]
 for label,path in a.source:
  for line in Path(path).open():
   if not line.strip():continue
   row=json.loads(line)
   if row['entry_frame']!=row['exit_frame']:continue
   calls.append({'source':label,'input':memory(row['entry']).hex(),'expected':projection(memory(row['exit']))})
 Path(a.output).write_text(json.dumps({'schema':'nba95-violation-parent-v1','calls':calls},separators=(',',':'))+'\n');print(f'[VIOLATION NORMALIZE] wrote {len(calls)} calls')
if __name__=='__main__':main()
