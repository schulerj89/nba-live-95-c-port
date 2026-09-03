"""Freeze only the new standalone draw packet; recheck prior formation intact."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def identity(p):
 data=p.read_bytes();return dict(path=str(p.resolve()),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
def main():
 out=ROOT/'.analysis/draw-order-freeze-v1.json';assert not out.exists()
 prior=ROOT/'.analysis/period-formation-freeze-v1.json'
 assert identity(prior)['sha256']=='8265eb8e8e71e6c59186b2a0526d9ea75c02fe96a80ccf18bed5507dde41e244'
 old=json.loads(prior.read_text())['files']
 for name,item in old.items():assert identity(Path(item['path']))==item,('prior changed',name)
 preservation=ROOT/'.analysis/draw-order-preservation-v1.json';assert not preservation.exists()
 preservation.write_text(json.dumps(dict(prior_freeze=identity(prior),identities_rechecked=len(old),unchanged=True),indent=2)+'\n')
 files={}
 def add(name,p):files[name]=identity(p)
 for name in ['include/nba_draw_order.h','src/nba_draw_order.c','tools/draw_order_probe.c','tools/build_draw_order_probe.ps1','tools/capture_draw_order.py','tools/capture_draw_order.lua','tools/mesen_portable.py','tools/draw_order_rom_reference.py','tools/verify_draw_order.py','tools/test_draw_order.py','tools/test_draw_order_protocol.py','tools/freeze_draw_order.py','.analysis/draw-order-preservation-v1.json']:
  add(name,ROOT/name)
 for directory in ['native-draw-order-v1','draw-order-build-v1','draw-order-build-v2','draw-order-native-v1','draw-order-native-v2','draw-order-source-v1','draw-order-protocol-v1']:
  for p in sorted((ROOT/'.analysis'/directory).rglob('*')):
   if p.is_file():add(p.relative_to(ROOT).as_posix(),p)
 add('original/rom.sfc',Path('F:/Games/SNES/NBA Live 95 (USA).sfc'))
 out.write_text(json.dumps(dict(schema=1,scope='standalone12record initialization/depth/singlepass state; normal-input component observations only; no production/OAM/scheduling/CPUtime claim; independent audit pending',files=files),indent=2)+'\n')
 print(len(files),identity(out)['sha256'])
if __name__=='__main__':main()
