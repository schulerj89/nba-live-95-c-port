"""Read-only canonical ROM listings and minimal draw-input table inventory."""
import argparse,hashlib,importlib.util,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);rom=a.rom.read_bytes()
 assert hashlib.sha256(rom).hexdigest()=='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
 decoder=Path('C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler/snes65816.py')
 spec=importlib.util.spec_from_file_location('sprite_source_decode',decoder);d=importlib.util.module_from_spec(spec);sys.modules[spec.name]=d;spec.loader.exec_module(d)
 for bank,start,end in[(0x80,0xad92,0xb0ff),(0x80,0xb348,0xb52f),(0x87,0xa4e1,0xa6b9),(0x87,0xab48,0xab51),(0x87,0xac13,0xac26)]:
  pc=start;lines=[];m=x=0
  while pc<end:
   offset=d.lorom_offset(bank,pc);i=d.decode_insn(rom,offset,pc,bank,m=m,x=x)
   lines.append(f'{bank:02X}:{pc:04X} {rom[offset:offset+i.length].hex(" "):12s} {i.mnem} {i.operand:06X}')
   if i.mnem=='SEP':m=1 if i.operand&32 else m;x=1 if i.operand&16 else x
   if i.mnem=='REP':m=0 if i.operand&32 else m;x=0 if i.operand&16 else x
   pc+=i.length
  (out/f'rom-{bank:02x}-{start:04x}.txt').write_text('\n'.join(lines)+'\n')
 tables={}
 for name,pc,size in[('head_order_51',0xacb6b3,0x830),('number_by_direction',0x87a98e,16),('head_by_direction',0x84c36e,16)]:
  offset=d.lorom_offset(pc>>16,pc&65535);raw=rom[offset:offset+size];assert len(raw)==size
  target=out/(name+'.bin');target.write_bytes(raw)
  tables[name]={'source_pc':hex(pc),'size':size,'sha256':sha(target),'existing_asset':name=='head_by_direction'}
  if size==16:tables[name]['words']=[int.from_bytes(raw[i:i+2],'little')for i in range(0,size,2)]
 (out/'inventory.json').write_text(json.dumps({'rom':{'path':str(a.rom.resolve()),'sha256':sha(a.rom)},'decoder':{'path':str(decoder),'sha256':sha(decoder)},'script_sha256':sha(Path(__file__)),'tables':tables,'new_literal_table_bytes':0x830+16,'limits':'Candidate evidence inventory only. No asset pack, resource ID, extractor or runtime producer changed.'},indent=2)+'\n')
 print(json.dumps(tables,indent=2))
if __name__=='__main__':main()
