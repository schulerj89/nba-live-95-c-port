"""Source-derived direct-page flag domain, only for native00F1 callbacks."""
import argparse,json,re
from pathlib import Path
import setup_spc_control_contract_v5 as c

def main():
 p=argparse.ArgumentParser();p.add_argument('--native',type=Path,required=True);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 types=(a.reference/'SpcTypes.h').read_text();flags=int(re.search(r'DirectPage = (0x[0-9A-Fa-f]+)',types).group(1),16);assert flags==32
 src=(a.reference/'Spc.cpp').read_text();assert 'return (CheckFlag(SpcFlags::DirectPage) ? 0x100 : 0) + offset;'in src
 instructions=(a.reference/'Spc.Instructions.cpp').read_text();assert 'case 0x8F: Addr_DirImm(); MOV_Imm(); break;'in instructions
 assert '_operandB = GetDirectAddress(ReadOperandByte()); EndAddr();'in instructions
 assert 'Write(_operandB, (uint8_t)_operandA); EndOp();'in instructions
 cases=0
 for index,pc,value in ((1,0x384,0x30),(2,0x3ec,1)):
  state=dict(line.split('=',1)for line in(a.native/f'spc_control_{index}_before.state').read_text().splitlines())
  for ps in range(256):
   changed=dict(state);changed['spc.ps']=str(ps)
   try:c.control_boundary(changed,pc,value)
   except ValueError:accepted=False
   else:accepted=True
   assert accepted==((ps&flags)==0),(index,ps);cases+=1
 (a.output/'report.json').write_text(json.dumps(dict(passed=True,cases=cases,scope='all256PSvalues per source callback; onlyDirectPage flag restricts this native address gate; hardwarecommitAPIunchanged'),indent=2)+'\n');print('PASS',cases)
if __name__=='__main__':main()
