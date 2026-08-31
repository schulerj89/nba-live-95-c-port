"""Append resource287 while preserving all264 prior payloads and metadata."""
import argparse,hashlib,json,struct
from pathlib import Path
from build_player_draw_inputs import build,HEAD_SHA,NUMBER_SHA,ROM_SHA
from upgrade_gameplay_hud_pack import unpack
def sha(raw):return hashlib.sha256(raw).hexdigest()
def upgrade(raw,rom):
 records=unpack(raw)
 if len(records)!=264 or not any(r[0]==286 for r in records) or any(r[0]>=287 for r in records):
  raise ValueError('expected complete264-resource version31 base without resource287')
 payload=build(rom);updated=records+[(287,0,0,0,payload)]
 cursor=16+len(updated)*24;directory=bytearray(b'NBA95PAK'+struct.pack('<II',31,len(updated)));data=bytearray()
 for key,width,height,flags,body in updated:
  directory+=struct.pack('<6I',key,cursor,len(body),width,height,flags);data+=body;cursor+=len(body)
 result=bytes(directory+data)
 if unpack(result)!=updated:raise ValueError('serialization changed existing asset')
 receipt={'schema':1,'base_sha256':sha(raw),'output_sha256':sha(result),'rom_sha256':ROM_SHA,'source_head_sha256':HEAD_SHA,'source_numbers_sha256':NUMBER_SHA,'preserved_assets':[{'id':key,'width':width,'height':height,'flags':flags,'bytes':len(body),'sha256':sha(body)}for key,width,height,flags,body in records],'new_asset':{'id':287,'bytes':len(payload),'sha256':sha(payload)}}
 return result,receipt
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('base-pack','rom','output','manifest'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();paths=[v.resolve()for v in vars(a).values()]
 if len(set(paths))!=4 or a.output.exists()or a.manifest.exists():raise ValueError('distinct input/output paths and new output/receipt required')
 raw=a.base_pack.read_bytes();rom=a.rom.read_bytes()
 if len(rom)%1024==512:rom=rom[512:]
 result,receipt=upgrade(raw,rom)
 receipt.update(base_pack=str(a.base_pack.resolve()),output=str(a.output.resolve()),script_sha256=sha(Path(__file__).read_bytes()))
 a.output.parent.mkdir(parents=True,exist_ok=True);a.manifest.parent.mkdir(parents=True,exist_ok=True)
 with a.output.open('xb')as f:f.write(result)
 with a.manifest.open('x')as f:json.dump(receipt,f,indent=2);f.write('\n')
 print(f'Preserved264 payloads; added287; {len(result)} bytes; SHA256 {sha(result)}')
if __name__=='__main__':main()
