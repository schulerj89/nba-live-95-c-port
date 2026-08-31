"""Read-only original-ROM disassembly for ball/pass source attribution."""
import argparse,hashlib,importlib.util,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
decoder=Path('C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler/snes65816.py')
rompath=Path('F:/Games/SNES/NBA Live 95 (USA).sfc');rom=rompath.read_bytes()
assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
s=importlib.util.spec_from_file_location('ball_source_decode',decoder);d=importlib.util.module_from_spec(s);sys.modules[s.name]=d;s.loader.exec_module(d)
for bank,start,end in [(0x80,0xad92,0xb16e),(0x87,0xa47a,0xa733),(0x87,0xb649,0xb692),(0x87,0xb832,0xb996),(0x85,0xa4f2,0xa5f2)]:
 pc=start;lines=[];m=x=0
 while pc<end:
  o=d.lorom_offset(bank,pc);i=d.decode_insn(rom,o,pc,bank,m=m,x=x)
  lines.append(f'{bank:02X}:{pc:04X} {rom[o:o+i.length].hex(" "):12s} {i.mnem} {i.operand:06X}')
  if i.mnem=='SEP':m=1 if i.operand&32 else m;x=1 if i.operand&16 else x
  if i.mnem=='REP':m=0 if i.operand&32 else m;x=0 if i.operand&16 else x
  pc+=i.length
 (out/f'rom-{bank:02x}-{start:04x}.txt').write_text('\n'.join(lines)+'\n')
(out/'identities.json').write_text(json.dumps({str(q):{'size':q.stat().st_size,'sha256':hashlib.sha256(q.read_bytes()).hexdigest()}for q in [decoder,rompath,Path(__file__)]},indent=2)+'\n')
