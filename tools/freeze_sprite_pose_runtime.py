"""Freeze the reviewed sprite-pose runtime source and bounded evidence."""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES=['.gitattributes','include/nba_assets.h','include/nba_player_lab.h','src/nba_assets.c','src/nba_player_lab.c','src/nba_tipoff.c','tools/extract_assets.py','tools/build_player_draw_inputs.py','tools/build_sprite_pose_guards.py','tools/build_sprite_pose_probe.py','tools/build_sprite_pose_runtime_probe.py','tools/freeze_sprite_pose_runtime.py','tools/inspect_sprite_pose_source.py','tools/sprite_pose_guard_probe.c','tools/sprite_pose_probe.c','tools/sprite_pose_rom_oracle.py','tools/sprite_pose_runtime_probe.c','tools/test_sprite_pose.py','tools/test_sprite_pose_protocol.py','tools/test_sprite_pose_runtime_inputs.py','tools/test_sprite_pose_runtime_source.py','tools/upgrade_player_draw_pack.py','tools/verify_sprite_pose_live_runtime.py','tools/verify_sprite_pose_runtime.py']
ARTIFACTS=['build/sprite-pose-runtime-pack-v1/manifest.json','build/sprite-pose-runtime-pack-v1/nba95_assets.pak','build/sprite-pose-runtime-cli-v3/manifest.json','build/sprite-pose-runtime-probe-v2/manifest.json','build/sprite-pose-runtime-input-tests-v2/report.json','build/sprite-pose-runtime-source-v1/report.json','build/sprite-pose-runtime-live-v1/report.json','build/sprite-pose-runtime-broad-v1/report.json','build/runtime-compositor-tests-v3/report.json','build/runtime-compositor-protocol-v1/report.json','build/runtime-compositor-guards-v1/manifest.json','../ball-pass-alignment-20260831/build/ball-native-v1/manifest.json']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def identity(p):p=p.resolve();return {'path':str(p),'size':p.stat().st_size,'sha256':sha(p)}
def staged_identity(name):
 raw=subprocess.check_output(['git','show',':'+name],cwd=ROOT)
 return {'git_index':name,'size':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 files=[ROOT/f for f in FILES];artifacts=[ROOT/f for f in ARTIFACTS]
 assert all(q.is_file()for q in files+artifacts)
 patch=subprocess.check_output(['git','diff','--binary','744809a','--',*FILES],cwd=ROOT)
 (out/'sprite-pose-runtime.patch').write_bytes(patch)
 names=[str(q.relative_to(ROOT)).replace('\\','/') for q in files]
 freeze={'schema':1,'base':'744809a9d2ad548f83dedd9dffabce09e3cbda11','files':{name:staged_identity(name) for name in names},'artifacts':{str(q):identity(q) for q in artifacts},'patch':identity(out/'sprite-pose-runtime.patch'),'scope':'Optional NBPDRAW1 asset, literal player-pose visual adapter, AB48-AC22 body mirror and cull-owned A609 head bit. No ball state, graphics queue, NbaGame/session/release/manifest or shared-pack ownership.'}
 (out/'freeze.json').write_text(json.dumps(freeze,indent=2)+'\n');print(f"Frozen {len(files)} source files and {len(artifacts)} evidence files; {sum(v['size'] for v in freeze['artifacts'].values())} evidence bytes")
if __name__=='__main__':main()
