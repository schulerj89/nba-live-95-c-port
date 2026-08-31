"""Independently demonstrate WIP HUD section-boundary reads; no production edits."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

HARNESS = r'''
#define _CRT_SECURE_NO_WARNINGS
#include "nba_gameplay_hud.h"
#include <stdio.h>
#include <string.h>
const NbaAssetItem *nba_assets_get(const NbaAssetPack *p,NbaAssetId id) {
    return id==NBA_ASSET_GAMEPLAY_HUD ? &p->items[0] : NULL;
}
int main(int argc,char **argv) {
    unsigned char storage[4096];
    NbaAssetPack pack={0}; NbaGameplayHud hud; NbaGameplayHudInput in={0};
    if(argc!=3)return 2;
    FILE *f=fopen(argv[1],"rb"); if(!f)return 3;
    memset(storage,0xA5,sizeof(storage));
    if(fread(storage,1,3500,f)!=3500 || fclose(f))return 4;
    /* The pack item ends at3500; the padded backing allocation safely
     * records whether a read crossed that declared item boundary. */
    pack.items[0].id=NBA_ASSET_GAMEPLAY_HUD;
    pack.items[0].data=storage; pack.items[0].size=3500;
    if(argv[2][0]=='1') { storage[3462]=30; storage[3463]=0;
                         storage[3464]=8; storage[3465]=0; }
    if(!nba_gameplay_hud_init(&hud,&pack)) {puts("INIT_REJECT");return 0;}
    in.presentation_timer_raw_08de=0xFFFF;
    int result=nba_gameplay_hud_publish(&hud,&pack,0x87BACB,&in);
    unsigned leaked=0;
    for(unsigned i=0;i<sizeof(hud.visible_map);++i)
        if(hud.visible_map[i]==0xA5)++leaked;
    printf("accepted=%d beyond_item_marker_bytes=%u\n",result,leaked);
    return 0;
}
'''

def digest(path):
    b=path.read_bytes()
    return dict(size=len(b),sha256=hashlib.sha256(b).hexdigest())

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--source',required=True,type=Path)
    p.add_argument('--resource',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    if a.output.exists():raise ValueError('refusing reused output')
    a.output.mkdir(parents=True)
    identities={}
    names=['src/nba_gameplay_hud.c','src/nba_rom_font.c',
           'include/nba_gameplay_hud.h','include/nba_rom_font.h',
           'include/nba_assets.h','include/nba_types.h']
    for name in names:
        dst=a.output/name;dst.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(a.source/name,dst);identities[name]=digest(dst)
    shutil.copyfile(a.resource,a.output/'hud-resource.bin')
    (a.output/'probe.c').write_text(HARNESS)
    rsp=['/nologo','/W4','/WX','/O2','/MD','/utf-8',
         '/I"'+str((a.output/'include').resolve())+'"',
         '"'+str((a.output/'probe.c').resolve())+'"',
         '"'+str((a.output/'src/nba_gameplay_hud.c').resolve())+'"',
         '"'+str((a.output/'src/nba_rom_font.c').resolve())+'"',
         '/Fe:probe.exe']
    (a.output/'compile.rsp').write_text('\n'.join(rsp)+'\n')
    vc=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat')
    (a.output/'compile.cmd').write_text('@echo off\ncall "'+str(vc)+'" >nul\ncl @compile.rsp\nexit /b %errorlevel%\n')
    run=subprocess.run(['cmd.exe','/c','compile.cmd'],cwd=a.output,capture_output=True,text=True)
    (a.output/'compile.log').write_text(run.stdout+run.stderr)
    if run.returncode:raise RuntimeError('private compile failed')
    reports=[]
    for mode in ['0','1']:
        run=subprocess.run([str((a.output/'probe.exe').resolve()),str((a.output/'hud-resource.bin').resolve()),mode],capture_output=True,text=True)
        reports.append(dict(mode=mode,code=run.returncode,stdout=run.stdout,stderr=run.stderr))
    report=dict(source=str(a.source.resolve()),sources=identities,
                resource=digest(a.output/'hud-resource.bin'),probe=digest(a.output/'probe.exe'),
                observations=reports,scope='Declared asset boundary only; backing storage deliberately padded, no undefined allocation access or production asset edits.')
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(reports))

if __name__=='__main__':main()
