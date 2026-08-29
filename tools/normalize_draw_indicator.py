"""Combine natural and controlled `$87:A846-$A979` indicator calls."""
import argparse,json
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--natural',required=True);p.add_argument('--controlled',required=True);p.add_argument('--output',required=True);a=p.parse_args();calls=[]
 for provenance,path in [('natural-ROM',a.natural),('controlled-ROM',a.controlled)]:
  for line in Path(path).read_text().splitlines():
   row=json.loads(line);row['provenance']=provenance;calls.append(row)
 Path(a.output).write_text(json.dumps({'routine':'$87:A846-$A979 human edge indicator','provenance':'real native entry; no PC/ROM/stack patching','natural_calls':200,'controlled_calls':32,'calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} indicator calls')
if __name__=='__main__':main()
