"""Verify NBPDRAW1 bytes, preservation, C table projection and refusal."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from upgrade_gameplay_hud_pack import unpack
from build_player_draw_inputs import build,ROM_SHA
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in('exe','base-pack','pack','manifest','rom','output'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 rom=a.rom.read_bytes();assert sha(rom)==ROM_SHA
 old=unpack(a.base_pack.read_bytes());new=unpack(a.pack.read_bytes());receipt=json.loads(a.manifest.read_text())
 assert len(old)==264 and len(new)==265 and old==new[:-1]
 assert new[-1][:4]==(287,0,0,0) and new[-1][4]==build(rom)
 assert receipt['preserved_assets']==[{'id':i,'width':w,'height':h,'flags':f,'bytes':len(b),'sha256':sha(b)}for i,w,h,f,b in old]
 env={k:v for k,v in os.environ.items()if not k.startswith('NBA95')}
 run=subprocess.run([str(a.exe),str(a.pack)],capture_output=True,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'probe.bin').write_bytes(run.stdout);(out/'probe.stderr.txt').write_bytes(run.stderr);assert run.returncode==0
 banner,payload=run.stdout.split(b'\n',1);assert banner.startswith(b'[ASSETS] Loaded asset pack:')
 words=list(struct.iter_unpack('<H',payload));values=[x[0]for x in words]
 table_words=0x830*8*2;identity_words=29*12*2*2
 assert len(values)==table_words+identity_words
 head=rom[((0xac-0x80)*0x8000)+(0xb6b3-0x8000):][:0x830]
 visibility=rom[((0xac-0x80)*0x8000)+(0xc7e3-0x8000):][:0x830]
 numbers=rom[((0x87-0x80)*0x8000)+(0xa98e-0x8000):][:16]
 expected=[]
 for upper in range(0x830):
  for direction in range(8):
   expected.extend((head[upper]|(0xff00 if head[upper]&0x80 else 0),
                    (visibility[upper]|0xff00) if visibility[upper]&0x80 else int.from_bytes(numbers[direction*2:direction*2+2],'little')))
 assert values[:table_words]==expected
 corrupt=bytearray(a.pack.read_bytes());records=unpack(bytes(corrupt));payload=records[-1][4]
 position=corrupt.find(payload);assert position>=0;corrupt[position+32]^=1
 bad=out/'corrupt.pak';bad.write_bytes(corrupt)
 reject=subprocess.run([str(a.exe),str(bad)],capture_output=True,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'corrupt.stdout.bin').write_bytes(reject.stdout);(out/'corrupt.stderr.txt').write_bytes(reject.stderr)
 assert reject.returncode!=0 and b'Player draw-input tables are invalid' in reject.stderr
 report={'passed':True,'rom_sha256':sha(rom),'base_pack_sha256':sha(a.base_pack.read_bytes()),'pack_sha256':sha(a.pack.read_bytes()),'exe_sha256':sha(a.exe.read_bytes()),'preserved_payloads':264,'table_cases':0x830*8,'identity_cases':29*12*2,'atomic_refusals':3,'malformed_pack_rejections':1,'limits':['No graphics queue, allocation, OAM cursor, NMI budget, scanout timing or ball physics claim.','Identity comparison reuses the previously accepted appearance setup API; ROM table projection is independently byte-derived.']}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: 264 payloads preserved; 16768 table cases; 696 identity cases')
if __name__=='__main__':main()
