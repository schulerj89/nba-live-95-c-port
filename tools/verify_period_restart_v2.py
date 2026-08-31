"""Typed isolated parent-segment differential; never a complete restart oracle."""
import argparse, hashlib, json, re, struct, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
BUILD={'include/nba_period_restart_v2.h','src/nba_period_restart_v2.c','tools/period_restart_probe_v2.c','tools/period_restart_probe_fields.inc','tools/build_period_restart_probe_v2.ps1'}
REVISIONS={'capture.lua':'d58b956c73f5b079d014c005f94f494b5800dbf604a4dc9d57a4e62832a5fbee','capture.py':'6c5d9982c5731277100c30b0a648bfa70bd9c6365f7edc614ea51da7f8179774','mesen_portable.py':'1bc6db2d68d836c7c6af180137a3d5e8e4ea454d7cb8a97e9e95cc6312ddc3bb'}
TAGS={'court.entry':0x87a47a,'seed.before':0x85edc6,'seed.after':0x85edc6,'period.scene':0x8795e9,'restart.predecessor':0x878c86,'formation.entry':0x86dca6,'clock.select':0x86dd2d,'clock.ready':0x86dd47,'formation.table':0x86dd97,'appearance.first.before':0x86dfcb,'appearance.first.after':0x86dfcf,'appearance.second.before':0x86dfd8,'appearance.second.after':0x86dfdc,'ball.initialize':0x86e056,'assignment.before':0x86e0ac,'assignment.after':0x86e0b0,'cancel.before':0x86e0b4,'cancel.after':0x86e0b8,'inbound.entry':0x86e0fa,'target.before':0x86e102,'target.after':0x86e106,'possession.before':0x86e17f,'possession.after':0x86e183,'upper.cancel.before':0x86e187,'lower.cancel.before':0x86e18b,'pose.before':0x86e198,'attachment.before':0x86e19c,'direction.before':0x86e1a0,'inbound.after':0x86e1a4,'roles.before':0x86e1e5,'roles.after':0x86e1f7,'formation.return':0x86e207,'opening.or.inbound.after':0x86e1ac}
FIELDS='0926 0928 092c 092e 0930 0932 0936 093a 093c 093e 0940 0942 0944 0946 0948 094a 0952 0954 0956 0958 095a 095c 0968 09b0 09b2 09b4 09b8 09ba 09c0 09f6 13e7 4711 4791'.split()

def check(ok,message):
    if not ok:raise ValueError(message)
def integer(v,lo,hi):return type(v)is int and lo<=v<=hi
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def pairs(items):
    out={}
    for k,v in items:check(k not in out,'duplicate JSON key '+k);out[k]=v
    return out
def bad_constant(value):raise ValueError('nonfinite JSON constant '+value)
def loads(text):return json.loads(text,object_pairs_hook=pairs,parse_constant=bad_constant)
def typed(g,w):
    check(type(g)is type(w),'evidence type')
    if isinstance(w,dict):
        check(set(g)==set(w),'evidence nested keys')
        for k in w:typed(g[k],w[k])
    elif isinstance(w,list):
        check(len(g)==len(w),'evidence list length')
        for a,b in zip(g,w):typed(a,b)
    else:check(g==w,'evidence value')
def word(raw,address):return struct.unpack_from('<H',raw,address)[0]
def mapping():
    fields=re.findall(r'^(ACTOR|BALL|GLOBAL)\((\w+),(0x[0-9a-f]+)\)$',(ROOT/'tools/period_restart_probe_fields.inc').read_text(),re.M)
    check(len(fields)==79,'typed field closure')
    result=[]
    for actor in range(10):result.extend((f'actor{actor}.{name}',0x34eb+actor*256+int(offset,16))for kind,name,offset in fields if kind=='ACTOR')
    result.extend((('ball.'if kind=='BALL'else'')+name,(0x3eeb if kind=='BALL'else 0)+int(offset,16))for kind,name,offset in fields if kind!='ACTOR')
    result.extend((f'object_list.{i}',0x34d3+2*i)for i in range(12))
    check(len(result)==406,'typed state word count');return result
def typed_words(raw):return [word(raw,addr)for _,addr in mapping()]
def binary_input(period,tip,anchors,words,returns=()):
    out=bytearray(struct.pack('<7H',0x5250,1,period,tip,*[v&65535 for v in anchors],len(returns)))
    out.extend(struct.pack('<'+'H'*len(words),*words))
    for pc,actor,values in returns:out.extend(struct.pack('<2H',pc&65535,actor));out.extend(struct.pack('<'+'H'*len(values),*values))
    return bytes(out)
def check_build(exe):
    m=loads((exe.parent/'build-manifest.json').read_text(encoding='utf-8-sig'))
    check(set(m)=={'schema','compiler_exit','sources','executable'}and integer(m['schema'],1,1)and integer(m['compiler_exit'],0,0)and set(m['sources'])==BUILD,'build contract')
    for name,value in m['sources'].items():
        check(set(value)=={'path','sha256'}and sha(value['path'])==value['sha256']==sha(ROOT/name),'build source identity')
    e=m['executable'];check(set(e)=={'path','sha256'}and Path(e['path']).resolve()==exe.resolve()and sha(exe)==e['sha256'],'executable identity')
    return m
def check_source(rom):
    data=rom.read_bytes();check(sha(rom)==ROM_SHA,'original ROM identity')
    body=(ROOT/'src/nba_period_restart_v2.c').read_text().split('formation[3][5]={',1)[1].split('};',1)[0]
    rows=[tuple(int(x)for x in match.split(','))for match in re.findall(r'\{(-?\d+,-?\d+,\d+,\d+)\}',body)]
    check(len(rows)==15,'formation table closure')
    table=b''.join(struct.pack('<hhHH',*row)for row in rows)
    check(table==data[0x506a:0x50e2],'original formation table bytes')
    return hashlib.sha256(table).hexdigest()
def read_native(p,rom):
    m=loads((p/'manifest.json').read_text())
    check(set(m)=={'schema','kind','period_seed','require_naturally_ready','injected_words','state_injection','rom_patch','formation_target_ready_injection','arguments','environment','isolation','sources','exit_code','completion','artifacts'},'capture manifest keys')
    check(integer(m['schema'],1,1)and integer(m['exit_code'],0,0)and integer(m['period_seed'],0,3),'capture numeric contract')
    check(m['kind']=='controlled expiry with normal cold boot/input and original period formation'and m['require_naturally_ready']is True and m['state_injection']is True and m['rom_patch']is False and m['formation_target_ready_injection']is False,'controlled capture scope')
    typed(m['injected_words'],['0926','0928','4711','4791','09b4','13e7'])
    revisions=dict(REVISIONS)
    if m['period_seed']==3:revisions['capture.lua']='bc931180e7827e12cb6955eb5d99d8d677b8555c59fc75c44536ddd993dcaa98'
    expected={str(rom.resolve()):ROM_SHA,str(p/'portable-mesen/Mesen.exe'):MESEN_SHA,**{str(p/n):h for n,h in revisions.items()}}
    typed(m['sources'],expected)
    for path,digest in expected.items():check(sha(path)==digest,'capture source identity')
    typed(m['arguments'],[str(p/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=240',str(rom.resolve()),str(p/'capture.lua')])
    typed(m['environment'],{'NBA95_CAPTURE_DIR':p.as_posix(),'NBA95_PERIOD_SEED':str(m['period_seed']),'NBA95_REQUIRE_READY':'1'})
    names={q.name for q in p.iterdir()if q.is_file()and q.name!='manifest.json'}
    check(set(m['artifacts'])==names,'capture artifact inventory')
    required={'capture.lua','capture.py','mesen_portable.py','boundaries.jsonl','capture_complete.txt','initial-mesen-settings.json','observed-script-data-folder.txt','stdout.log','stderr.log'}
    check(required<=names,'required capture artifacts')
    for name,v in m['artifacts'].items():check(set(v)=={'bytes','sha256'}and integer(v['bytes'],0,2**31)and (p/name).stat().st_size==v['bytes']and sha(p/name)==v['sha256'],'capture artifact identity')
    check((p/'stderr.log').read_bytes()==b''and (p/'capture_complete.txt').read_text()==m['completion'],'capture completion/stderr')
    iso=m['isolation'];check(set(iso)=={'method','home','save_folder','initial_saves','settings','initial_settings_sha256','post_settings_verified','observed_script_data_folder','post_settings_sha256','final_saves'},'isolation fields')
    check(iso['initial_saves']==[]and type(iso['final_saves'])is dict and iso['post_settings_verified']is True,'save isolation')
    check(iso['final_saves']=={q.name:sha(q)for q in (p/'isolated-saves').iterdir()if q.is_file()},'generated save identities')
    check(Path(iso['home']).resolve()==p/'portable-mesen'and Path(iso['save_folder']).resolve()==p/'isolated-saves','isolation paths')
    initial=loads((p/'initial-mesen-settings.json').read_text());typed(initial,iso['settings'])
    settings={'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60,'SaveScriptBeforeRun':False}},'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':str(p/'isolated-saves')},'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'DisableFrameSkipping':True,'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','ForceFixedResolution':False,'Overscan':{'Top':7,'Bottom':8,'Left':0,'Right':0}},'Video':{'VideoFilter':'None','AspectRatio':'NoStretching','Brightness':0,'Contrast':0,'Hue':0,'Saturation':0,'ScanlineIntensity':0,'UseBilinearInterpolation':False,'ScreenRotation':'None'}}
    typed(initial,settings)
    check(iso['method']=='private portable executable/settings','isolation method')
    observed=(p/'observed-script-data-folder.txt').read_text().strip()
    check(observed==iso['observed_script_data_folder']and Path(observed).resolve()==p/'portable-mesen/LuaScriptData/capture','observed script directory')
    check(sha(p/'initial-mesen-settings.json')==iso['initial_settings_sha256']and sha(p/'portable-mesen/settings.json')==iso['post_settings_sha256'],'settings identities')
    post=loads((p/'portable-mesen/settings.json').read_text(encoding='utf-8-sig'))
    def subset(g,w):
        for k,v in w.items():
            if isinstance(v,dict):subset(g[k],v)
            else:typed(g[k],v)
    subset(post,initial)
    check(initial['Snes']['RamPowerOnState']=='AllZeros'and initial['Snes']['EnableRandomPowerOnState']is False and initial['Preferences']['AutoLoadPatches']is False,'cold boot settings')
    rows=[loads(line)for line in (p/'boundaries.jsonl').read_text().splitlines()]
    for index,row in enumerate(rows,1):
        check(set(row)=={'index','frame','court','tag','pc','raw','a','x','y','sp','d','ps','fields'},'boundary schema')
        check(row['tag']in TAGS and integer(row['pc'],0,0xffffff)and row['pc']==TAGS[row['tag']]and integer(row['index'],index,index),'boundary identity')
        for k in ('frame','court','a','x','y','sp','d','ps'):check(integer(row[k],0,255 if k=='ps'else 2**31 if k in ('frame','court')else 65535),'boundary numeric '+k)
        if index>1:check(row['frame']>=rows[index-2]['frame']and row['court']>=rows[index-2]['court'],'boundary chronology')
        check(row['raw']==f'raw_{index:04d}.bin'and set(row['fields'])==set(FIELDS),'boundary raw/fields')
        raw=(p/row['raw']).read_bytes();check(len(raw)==131072,'full WRAM size')
        for key,value in row['fields'].items():check(integer(value,0,65535)and value==word(raw,int(key,16)),'boundary field/WRAM divergence')
        row['memory']=raw
    before=next(r for r in rows if r['tag']=='seed.before')['memory'];after=next(r for r in rows if r['tag']=='seed.after')['memory']
    allowed={int(k,16)+b for k in m['injected_words']for b in (0,1)}
    check(all(a==b or index in allowed for index,(a,b)in enumerate(zip(before,after))),'undeclared capture injection')
    check(word(before,0x9ba)==word(after,0x9ba)==1,'ready must be naturally retained')
    check(word(after,0x926)==m['period_seed']and word(after,0x928)==1 and word(after,0x4711)==10 and word(after,0x4791)==(10 if m['period_seed']==3 else 8)and word(after,0x9b4)==0 and word(after,0x13e7)==word(before,0x13e7)&0xf7ff,'declared expiry seed differs')
    prefix=['court.entry','seed.before','seed.after','period.scene','restart.predecessor','formation.entry','clock.select','clock.ready','formation.table']
    appearances=['appearance.first.before','appearance.first.after','appearance.second.before','appearance.second.after']*5
    common=['ball.initialize','assignment.before','assignment.after','cancel.before','cancel.after']
    rest=['opening.or.inbound.after']if m['period_seed']==3 else ['inbound.entry','target.before','target.after','possession.before','possession.after','upper.cancel.before','lower.cancel.before','pose.before','attachment.before','direction.before','inbound.after']
    typed([r['tag']for r in rows],prefix+appearances+common+rest+['roles.before','roles.after','formation.return'])
    check(names==required|{f'raw_{i:04d}.bin'for i in range(1,len(rows)+1)},'exact capture artifact closure')
    check(m['completion']==f"period_seed={m['period_seed']}\nnext_period={m['period_seed']+1}\nframes={rows[-1]['frame']}\nboundaries={len(rows)}\n",'completion values')
    start=next(r for r in rows if r['tag']=='formation.table')
    check(start['x']==0 and start['y']==0x34eb,'DD91/DD94 original formation entry X/Y')
    check(word(start['memory'],0xb6)==word(start['memory'],0x46f5),'original carried B6/context0 anchor')
    check(word(start['memory'],0x9a)==0x34d3,'DD89 original formation entry cursor')
    for row in rows[start['index']-1:]:
        check(row['d']==0 and not(row['ps']&0x38),'native M/X/decimal D/direct-page preconditions')
        check(word(row['memory'],0x926)==m['period_seed']+1,'native period')
        for address in (0x932,0x46f5,0x4775):check(word(row['memory'],address)==word(start['memory'],address),'carried helper inputs may not change in excluded children')
        for address in (0x9ba,0x9b0,0x9b2):check(word(row['memory'],address)==word(start['memory'],address),'native preserved latch/coordinates')
    return m,rows
def run_probe(exe,out,name,data):
    src=out/(name+'.input');trace=out/(name+'.jsonl');src.write_bytes(data)
    r=subprocess.run([str(exe.resolve()),str(src)],capture_output=True,text=True)
    check(type(r.returncode)is int and r.returncode==0 and type(r.stdout)is str and type(r.stderr)is str and r.stderr=='','probe status/stderr')
    trace.write_text(r.stdout);rows=[loads(line)for line in r.stdout.splitlines()]
    for row in rows:
        check(set(row)=={'kind','pc','child','actor','words'}and integer(row['kind'],1,11)and integer(row['pc'],0,0xffffff)and integer(row['child'],0,0xffffff)and integer(row['actor'],0,65535),'probe boundary schema')
        check(type(row['words'])is list and len(row['words'])==len(mapping())and all(integer(x,0,65535)for x in row['words']),'probe typed word schema')
    return rows
def verify_case(p,rom,exe,out):
    manifest,native=read_native(p,rom);start=next(r for r in native if r['tag']=='formation.table')
    names=mapping();raw=start['memory'];period=word(raw,0x926);tip=word(raw,0x932);anchors=[word(raw,a)for a in (0x46f5,0x4775)]
    expected=[];returns=[];external=[]
    for r in native:
        pc=r['pc']
        if pc in (0x86dfcb,0x86dfd8,0x86e056,0x86e0ac,0x86e0b0,0x86e0b4,0x86e0b8,0x86e102,0x86e106,0x86e183,0x86e1ac):expected.append(r)
        if pc in (0x86dfcf,0x86dfdc,0x86e0b0,0x86e0b4):
            caller={0x86dfcf:0x86dfcb,0x86dfdc:0x86dfd8,0x86e0b0:0x86e0ac,0x86e0b4:0x86e0b0}[pc]
            prev=native[r['index']-2];check(prev['pc']==caller,'external child pairing')
            actor=(word(prev['memory'],0x96)-0x34eb)//256 if caller in (0x86dfcb,0x86dfd8)else 65535
            values=typed_words(r['memory']);returns.append((caller,actor,values))
            prior=typed_words(prev['memory']);external.append({'caller':caller,'actor':actor,'changed':[name for (name,_),a,b in zip(names,prior,values)if a!=b]})
    got=run_probe(exe,out,p.name,binary_input(period,tip,anchors,typed_words(raw),returns))
    check(len(got)==len(expected)==(16 if period==4 else 18)and len(returns)==12,'exact parent/child boundary count')
    for c,n in zip(got,expected):
        metadata={0x86dfcb:(1,0x87aab2,(word(n['memory'],0x96)-0x34eb)//256),0x86dfd8:(1,0x87aab2,(word(n['memory'],0x96)-0x34eb)//256),0x86e056:(2,0,65535),0x86e0ac:(3,0x86d85e,65535),0x86e0b0:(4,0x86d5db,65535),0x86e0b4:(5,0x86a60d,65535),0x86e0b8:(6,0,65535),0x86e102:(7,0x85c37d,word(n['memory'],0x952)+2),0x86e106:(8,0,65535),0x86e183:(9,0x86bc9b,word(n['memory'],0x954)),0x86e1ac:(10,0,65535)}
        kind,child,actor=metadata[n['pc']]
        typed({k:c[k]for k in ('kind','pc','child','actor')},{'kind':kind,'pc':n['pc'],'child':child,'actor':actor})
        wanted=typed_words(n['memory'])
        differences=[(name,actual,want)for (name,_),actual,want in zip(names,c['words'],wanted)if actual!=want]
        check(not differences,f'parent fields at {n["pc"]:06X}: {differences[:12]}')
    return {'capture':str(p),'manifest_sha256':sha(p/'manifest.json'),'period':period,'tip_winner':tip,'parent_boundaries':len(got),'typed_comparisons':len(got)*len(names),'external_children':external,'scope':'isolated parent segments; typed native child returns are external test inputs, not child emulation'}
def main(a):
    a.rom=a.rom.resolve();a.exe=a.exe.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    check_build(a.exe);table=check_source(a.rom)
    cases=[verify_case(p.resolve(),a.rom,a.exe,out)for p in a.native]
    report={'passed':True,'cases':cases,'formation_rom_sha256':table,'source_sha256':sha(ROOT/'src/nba_period_restart_v2.c'),'verifier_sha256':sha(__file__),'build_manifest_sha256':sha(a.exe.parent/'build-manifest.json')}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(cases),'captures');return report
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--native',type=Path,nargs='+',required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);main(p.parse_args())
