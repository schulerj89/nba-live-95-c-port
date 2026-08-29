import argparse,json
from pathlib import Path
from verify_close_finish_vectors import coherent,memory,row,word
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--output',required=True);a=p.parse_args()
 raw=[json.loads(x) for x in Path(a.vectors).read_text().splitlines() if x.strip()]
 vs=[v for v in raw if coherent(v)]
 # Retain every natural call: the corpus is small and contains six distinct
 # entry/exit pairs, including expiry, hold, jump and terminal-shot exits.
 calls=[{'call':v['call'],'entry_pc':v['entry_pc'],'exit_pc':v['exit_pc'],
         'input':memory(v['entry']).hex(),
         'expected':row(memory(v['exit']),v['entry_pc'],word(memory(v['entry']),0x96))} for v in vs]
 Path(a.output).write_text(json.dumps({'routine':'$86:B0F7-$B624 close-finish modes','provenance':'natural CPU-vs-CPU real-entry calls; no PC/ROM/stack patching','quarantined_incoherent_snapshots':len(raw)-len(vs),'calls':calls},separators=(',',':'))+'\n')
 print(f'normalized {len(calls)} close-finish calls; quarantined {len(raw)-len(vs)} incoherent snapshots')
if __name__=='__main__':main()
