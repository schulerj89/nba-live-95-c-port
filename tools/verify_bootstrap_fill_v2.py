"""Strict normal-bootstrap checkpoint comparison. Does not accept full S1/03DB.

Native master clocks on SPC callbacks identify lazy emulator catch-up, not the
SPC oscillator deadline. Compare SPC cycle positions, CPU callback clocks and
the declared projections separately; never inject any native value into C.
"""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from bootstrap_fill_trace_protocol import validate as validate_trace
from bootstrap_boundary_protocol_v2 import bind_cpu_hook,validate_stdout
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
MESEN_SHA='d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b'
SCRIPT_SHA='4de96d94cb2444a0456ddf6213edd61eb629fec9e337a307111c5f62b27d9123'
RUNNER_SHA='a1a0a496e0fb9fdb395dd4c3d74fce8cbb028998f69d686f77ed952362808dbb'
DECODER_SHA='b1864664d3ac0abcd439055e88bf7220cf66be7a6ae9dfc5d59b3186f1469a46'
SOURCES=set(['include/nba_bootstrap.h', 'include/nba_bootstrap_fill.h', 'include/nba_bootstrap_internal.h', 'include/nba_rom.h', 'include/nba_setup_codec_work.h', 'include/nba_setup_spc_control.h', 'include/nba_setup_spc_init.h', 'include/nba_setup_spc_resident.h', 'include/nba_types.h', 'src/nba_bootstrap.c', 'src/nba_bootstrap_cpu.c', 'src/nba_bootstrap_cpu_program.inc', 'src/nba_bootstrap_fill.c', 'src/nba_bootstrap_fill_cpu.c', 'src/nba_bootstrap_fill_cpu_program.inc', 'src/nba_bootstrap_ipl.c', 'src/nba_bootstrap_rom.c', 'src/nba_rom.c', 'src/nba_setup_spc_control.c', 'src/nba_setup_spc_init.c', 'tools/bootstrap_fill_probe.c', 'tools/build_bootstrap_fill_probe.ps1', 'tools/generate_bootstrap_cpu.py', 'tools/generate_bootstrap_fill_cpu.py'])
STREAMS=('cpu','spc','cpu_bus','spc_bus','boundaries')
TAGS=('power_entry','cpu_80bc','cpu_dma_request','resident_entry','post_f1','cpu_after_dma','cpu_fill_return')
ARTIFACTS={n+'.jsonl'for n in STREAMS}|{t+'.'+suffix for t in TAGS for suffix in ('state','aram','wram','vram')}|set('stdout.log stderr.log environment.txt complete.txt initial-settings.json capture.lua runner.py'.split())
def check(ok,message):
    if not ok:raise ValueError(message)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def pairs(items):
    d={}
    for k,v in items:
        check(k not in d,'duplicate JSON key '+k);d[k]=v
    return d
def loads(s):return json.loads(s,object_pairs_hook=pairs)
def load(p):return loads(Path(p).read_text(encoding='utf-8-sig'))
def rows(p):return [loads(s)for s in Path(p).read_text().splitlines()]
def integer(v,lo,hi):return type(v)is int and lo<=v<=hi
def subset(g,w):
    for k,v in w.items():
        check(k in g,'missing setting '+k)
        if type(v)is dict:subset(g[k],v)
        else:check(type(g[k])is type(v)and g[k]==v,'setting '+k)
def native(root,rom):
    m=load(root/'manifest.json')
    check(set(m)==set('schema kind state_injection rom_patch inputs accepted sources settings initial_saves arguments exit_code post_settings_sha256 observed_environment artifacts'.split()),'native manifest schema')
    check(type(m['schema'])is int and m['schema']==1,'native schema')
    check(m['kind']=='normal reset first DMA fill observation','native kind')
    for key,want in [('accepted',True),('state_injection',False),('rom_patch',False),('inputs',False)]:check(type(m[key])is bool and m[key]is want,key)
    check(type(m['exit_code'])is int and m['exit_code']==0,'native exit')
    check(m['initial_saves']==[],'initial saves')
    check(set(m['sources'])=={'rom','mesen','script','runner','settings'},'source closure')
    expected={'rom':ROM_SHA,'mesen':MESEN_SHA,'script':SCRIPT_SHA,'runner':RUNNER_SHA}
    for name,r in m['sources'].items():
        check(set(r)=={'path','sha256'},'source identity shape')
        check(sha(r['path'])==r['sha256'],'source identity '+name)
        if name in expected:check(r['sha256']==expected[name],'pinned '+name)
    check(Path(m['sources']['rom']['path']).resolve()==rom,'ROM source path')
    check(Path(m['sources']['mesen']['path']).resolve()==root/'portable-mesen/Mesen.exe','private emulator')
    check(Path(m['sources']['script']['path']).resolve()==root/'capture.lua','script path')
    check(Path(m['sources']['runner']['path']).resolve()==root/'runner.py','runner path')
    check(Path(m['sources']['settings']['path']).resolve()==root/'initial-settings.json','settings path')
    check(m['arguments']==[str(root/'portable-mesen/Mesen.exe'),'--testrunner','--timeout=180',str(rom),str(root/'capture.lua')],'native arguments')
    want={'Debug':{'ScriptWindow':{'AllowIoOsAccess':True,'ScriptTimeout':60}},'Preferences':{'SingleInstance':False,'PauseWhenInBackground':False,'AutoLoadPatches':False,'OverrideSaveDataFolder':True,'SaveDataFolder':str(root/'saves')},'Snes':{'Port1':{'Type':'SnesController'},'Port2':{'Type':'None'},'EnableRandomPowerOnState':False,'RamPowerOnState':'AllZeros','DisableFrameSkipping':True,'SpcClockSpeedAdjustment':40,'Region':'Ntsc'}}
    check(m['settings']==want,'declared settings');check(load(root/'initial-settings.json')==want,'initial settings')
    post=root/'portable-mesen/settings.json';check(sha(post)==m['post_settings_sha256'],'post settings identity');subset(load(post),want)
    env=dict(line.split('=',1)for line in(root/'environment.txt').read_text().splitlines())
    check(m['observed_environment']==env,'environment declaration');check(Path(env['output']).resolve()==root,'output home');check(Path(env['home']).resolve().is_relative_to(root/'portable-mesen'),'Lua home')
    check(set(m['artifacts'])==ARTIFACTS,'artifact closure')
    for name,r in m['artifacts'].items():
        check(set(r)=={'bytes','sha256'}and integer(r['bytes'],0,2**40),'artifact shape')
        check((root/name).stat().st_size==r['bytes']and sha(root/name)==r['sha256'],'artifact '+name)
    check((root/'complete.txt').read_text()=='ok; normal power-on through first reset fill return80C0\n','completion')
    check((root/'stderr.log').read_bytes()==b'','native stderr')
    data={n:rows(root/(n+'.jsonl'))for n in STREAMS}
    for name,stream in data.items():
        check(bool(stream),'empty native '+name);prev_master=-1;prev_cycles=-1
        for j,r in enumerate(stream):
            check(integer(r['event'],0,2**63)and r['event']==j,'native row ordinal')
            check(integer(r['master'],0,2**53)and r['master']>=prev_master,'native chronology');prev_master=r['master']
            if name=='boundaries':continue
            check(integer(r['pc'],0,65535 if name.startswith('spc')else 0xffffff),'native PC')
            check(integer(r['cycles'],0,2**53)and r['cycles']>=prev_cycles,'native processor chronology');prev_cycles=r['cycles']
            if name.endswith('_bus'):
                check(set(r)=={'event','master','cycles','pc','kind','address','value'},'native bus schema')
                check(integer(r['address'],0,65535 if name=='spc_bus'else 0xffffff)and integer(r['value'],0,255)and r['kind']in ('read','write'),'native bus domain')
            else:
                keys={'event','master','cycles','pc','a','x','y','sp','ps'}|({'db','dp','emulation'}if name=='cpu'else set())
                check(set(r)==keys,'native instruction schema')
                for k in ('a','x','y','sp','ps'):check(integer(r[k],0,255 if name=='spc'or k=='ps'else 65535),'native register '+k)
                if name=='cpu':check(integer(r['db'],0,255)and type(r['dp'])is int and r['dp']==0 and type(r['emulation'])is bool,'native CPU domain')
    check([r['tag']for r in data['boundaries']]==list(TAGS),'boundary tags')
    for r in data['boundaries']:
        check(set(r)=={'event','tag','master','cpu','spc','cpu_pc','spc_pc'},'boundary schema')
        for k in ('cpu','spc','cpu_pc','spc_pc'):check(integer(r[k],0,2**53),'boundary scalar')
    bind_native_boundaries(root,data)
    return data
def build(exe):
    m=load(exe.parent/'build-manifest.json')
    check(type(m['schema'])is int and m['schema']==1,'build schema');check(type(m['compiler_exit'])is int and m['compiler_exit']==0,'compiler status')
    check(set(m['sources'])==SOURCES,'build closure')
    for name,r in m['sources'].items():check(set(r)=={'path','sha256'}and sha(r['path'])==r['sha256'],'build source '+name)
    check(set(m['executable'])=={'path','sha256'}and Path(m['executable']['path']).resolve()==exe and sha(exe)==m['executable']['sha256'],'exe identity')
    return m
def state(p):
    d={}
    for line in p.read_text().splitlines():
        k,v=line.split('=',1);check(k not in d,'duplicate scalar state');d[k]=v
    return d
def bind_native_boundaries(root,data):
    # Callback PCs track the last EXEC observer and can differ from the CPU's
    # internal PC during DMA or a split SPC instruction. Bind tracking to
    # actual observed rows, while counters/master come from each raw snapshot.
    known_cpu={'power_entry':0x800d,'cpu_80bc':0x8080bc,'cpu_dma_request':0x808a8d,'cpu_after_dma':0x808a92,'cpu_fill_return':0x8080c0}
    known_spc={'resident_entry':0x380,'post_f1':0x387}
    for r in data['boundaries']:
        check(integer(r['cpu_pc'],0,0xffffff)and integer(r['spc_pc'],0,65535),'boundary PC widths')
        snapshot=state(root/(r['tag']+'.state'))
        def scalar(key):
            value=snapshot[key]
            check(re.fullmatch(r'0|[1-9][0-9]*',value)is not None,'snapshot scalar syntax '+key)
            return int(value)
        check((r['master'],r['cpu'],r['spc'])==(scalar('masterClock'),scalar('cpu.cycleCount'),scalar('spc.cycle')),'boundary snapshot counters')
        last_cpu=[x for x in data['cpu']if x['master']<=r['master']and x['cycles']<=r['cpu']]
        last_spc=[x for x in data['spc']if x['master']<=r['master']and x['cycles']<=r['spc']]
        check(r['cpu_pc']==(last_cpu[-1]['pc']if last_cpu else 0),'boundary CPU observer tracking')
        check(r['spc_pc']==(last_spc[-1]['pc']if last_spc else 0),'boundary SPC observer tracking')
        if r['tag']in known_cpu:
            pc=known_cpu[r['tag']]
            check(r['cpu_pc']==pc and (scalar('cpu.k')<<16)|scalar('cpu.pc')==pc,'CPU source hook PC')
            matching=[x for x in data['cpu']if (x['pc'],x['master'],x['cycles'])==(pc,r['master'],r['cpu'])]
            check(len(matching)==1,'one CPU source hook instruction');bind_cpu_hook(matching[0],snapshot)
        else:
            pc=known_spc[r['tag']]
            check(r['spc_pc']==pc and scalar('spc.pc')==pc,'SPC source hook PC')
            check(any((x['pc'],x['master'],x['cycles'])==(pc,r['master'],r['spc'])for x in data['spc']),'SPC source hook instruction')
def compare(c,n,keys,label):
    check(len(c)==len(n),label+' count')
    for i,(a,b)in enumerate(zip(c,n)):
        for k in keys:check(type(a[k])is type(b[k])and a[k]==b[k],f'{label}[{i}].{k}: {a[k]} != {b[k]}')
def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--decoder-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    root=a.native.resolve();rom=a.rom.resolve();exe=a.exe.resolve();out=a.output.resolve();check(sha(rom)==ROM_SHA,'ROM');n=native(root,rom);bm=build(exe)
    check(sha(a.decoder_root/'snes65816.py')==DECODER_SHA,'static decoder');sys.path.insert(0,str(a.decoder_root.resolve()));import snes65816 as decoder
    out.mkdir(parents=True,exist_ok=False)
    r=subprocess.run([str(exe),str(rom),str(out)],capture_output=True,text=True,timeout=60)
    (out/'stdout.log').write_text(r.stdout);(out/'stderr.log').write_text(r.stderr)
    check(type(r.returncode)is int and r.returncode==0 and r.stderr=='','C process protocol')
    summary=loads(r.stdout.splitlines()[-1]);check(set(summary)=={'status','boundary_pc','master','spc_ticks','cpu_cycles','spc_pc','spc_phase','upload','resident','f1','refresh','dma_bytes','vram_address','sync_counter','source_index'},'C summary schema')
    for k,v in summary.items():check(type(v)is(bool if k in ('resident','f1')else int),'C summary type')
    check(summary['status']==1 and summary['boundary_pc']==0x8080c0 and summary['upload']==1264 and summary['resident']and summary['f1'],'actual bounded stop')
    trace=rows(out/'events.jsonl');ccpu=[];cspc=[];cbus=[];sbus=[];last_master=-1;current=None;end_cpu=True;end_spc=True;raw_rom=rom.read_bytes();end_addr=0
    for t in trace:
        check(integer(t['kind'],0,8)and t['kind']!=4,'C event kind')
        check(integer(t['master'],0,2**53)and t['master']>=last_master,'C mixed chronology');last_master=t['master']
        kind=t['kind']
        if kind in (5,6):
            keys={'kind','master','pc','cycles','a','x','y','sp','ps'}|({'db','dp','emulation'}if kind==5 else set());check(set(t)==keys,'C instruction schema')
            for k in ('a','x','y','sp','ps'):check(integer(t[k],0,255 if kind==6 or k=='ps'else 65535),'C register domain')
            check(integer(t['pc'],0,65535 if kind==6 else 0xffffff)and integer(t['cycles'],0,2**53),'C instruction scalar')
            if kind==5:
                check(end_cpu,'incomplete CPU instruction');end_cpu=False;ccpu.append(t);current=t
                check(type(t['emulation'])is bool and integer(t['db'],0,255)and type(t['dp'])is int and t['dp']==0,'CPU mode')
                pc=t['pc'];i=decoder.decode_insn(raw_rom,((pc>>16)&127)*32768+(pc&32767),pc&65535,pc>>16,(t['ps']>>5)&1,(t['ps']>>4)&1);end_addr=pc+i.length
            else:check(end_spc,'incomplete SPC instruction');end_spc=False;cspc.append(t)
            continue
        check(set(t)=={'kind','master','sample_master','spc','pc','address','value','bus','end'},'C bus schema')
        for k in ('sample_master','spc','pc','address','value','bus'):check(integer(t[k],0,255 if k=='value'else 2**53),'C bus scalar')
        check(t['sample_master']<=t['master']and type(t['end'])is bool,'C sample/end domain')
        if kind==0:
            check(current is not None and t['pc']==current['pc']and not end_cpu and t['bus']in(0,1,2),'CPU bus association');end_cpu=t['end']
            fetch=t['bus']==0 and current['pc']<=t['address']<end_addr
            if fetch:
                off=((t['address']>>16)&127)*32768+(t['address']&32767);check(t['value']==raw_rom[off],'CPU fetch ROM byte')
            if t['bus']==2:check(t['address']==0 and t['value']==0,'canonical CPU idle')
            elif not fetch:cbus.append({'pc':t['pc'],'master':t['master'],'address':t['address'],'value':t['value'],'kind':'read'if t['bus']==0 else 'write'})
        elif kind==1:
            check(cspc and t['pc']==cspc[-1]['pc']and not end_spc and t['bus']in(1,2,3,4),'SPC bus association');end_spc=t['end']
            if t['bus']==4:check(t['address']==0 and t['value']==0,'canonical SPC idle')
            if t['bus']==1:
                if t['address']>=0xffc0:
                    bios=bytes.fromhex('cdefbde800c61dd0fc8faaf48fbbf578ccf4d0fb2f19ebf4d0fc7ef4d00be4f5cbf4d700fcd0f3ab0110ef7ef410ebbaf6da00baf4c4f4dd5dd0db1f0000c0ff');v=bios[t['address']&63]
                else:v=raw_rom[0x4687+t['address']-0x380]
                check(t['value']==v,'SPC fetch source byte')
            if t['bus']==3 or(t['bus']==2 and 0xf0<=t['address']<=0xff):sbus.append({'pc':t['pc'],'cycles':t['spc'],'address':t['address'],'value':t['value'],'kind':'write'if t['bus']==3 else 'read'})
    protocol,expected_vram=validate_trace(trace,summary,raw_rom,decoder)
    validate_stdout(r.stdout,trace,lambda name:state(out/name))
    boundary=n['boundaries'][-1];check(summary['master']==boundary['master']and summary['cpu_cycles']==boundary['cpu'],'CPU final clock')
    # Reconstruct every CPU cycle ordinal, including fetches/idles, and the
    # suspended current CPU cycle on DMA accesses. Keep mixed trace order.
    cbus=[];cpu_count=0;current=None;fetch_end=0
    for t in trace:
        if t['kind']==5:
            current=t;pc=t['pc'];i=decoder.decode_insn(raw_rom,((pc>>16)&127)*32768+(pc&32767),pc&65535,pc>>16,(t['ps']>>5)&1,(t['ps']>>4)&1);fetch_end=pc+i.length
        if t['kind']==0:
            cpu_count+=1
            if t['bus']==2 or(t['bus']==0 and current['pc']<=t['address']<fetch_end):continue
            cbus.append({'pc':t['pc'],'master':t['master'],'cycles':cpu_count,'address':t['address'],'value':t['value'],'kind':'read'if t['bus']==0 else'write'})
        if t['kind']==7:
            check(t['bus']in(0,1),'DMA bus kind')
            cbus.append({'pc':t['pc'],'master':t['master'],'cycles':cpu_count+1,'address':t['address'],'value':t['value'],'kind':'read'if t['bus']==0 else'write'})
    nn=[t for t in n['cpu']if t['master']<boundary['master']]
    compare(ccpu,nn,('pc','master','cycles','a','x','y','sp','ps','db','dp','emulation'),'CPU instruction')
    compare(cbus,[t for t in n['cpu_bus']if t['master']<=boundary['master']],('pc','master','cycles','address','value','kind'),'CPU and DMA data bus')
    lo=n['spc'][0]['cycles'];hi=n['spc'][-1]['cycles'];ss=[t for t in cspc if lo<=t['cycles']<=hi]
    compare(ss,n['spc'],('pc','cycles','a','x','y','sp','ps'),'SPC instruction')
    ns=[t for t in n['spc_bus']if t['kind']=='write'or 0xf0<=t['address']<=0xff]
    compare([t for t in sbus if ns[0]['cycles']<=t['cycles']<=boundary['spc']],ns,('pc','cycles','address','value','kind'),'SPC writes and IO')
    check((out/'final.wram').read_bytes()==(root/'cpu_fill_return.wram').read_bytes(),'full CPU-boundary WRAM')
    scalar_keys={'spc.'+k for k in ('a','x','y','sp','ps','pc','cycle','romEnabled','writeEnabled','dspReg')}|{f'spc.{p}[{i}]'for p in ('cpuRegs','outputReg')for i in range(4)}|{f'spc.timer{i}.{k}'for i in range(3)for k in ('stage0','stage1','prevStage1','stage2','output','target','enabled','timersEnabled')}
    for prefix,tag in [('entry','resident_entry'),('f1','post_f1')]:
        check((out/(prefix+'.aram')).read_bytes()==(root/(tag+'.aram')).read_bytes(),'full '+prefix+' ARAM')
        cstate=state(out/(prefix+'.state'));nstate=state(root/(tag+'.state'));check(set(cstate)==scalar_keys,'C boundary scalar closure')
        for k in scalar_keys:check(cstate[k]==nstate[k],'boundary scalar '+k)
    check((out/'cpu_80bc.wram').read_bytes()==(root/'cpu_80bc.wram').read_bytes(),'earlier checkpoint WRAM unchanged')
    check((out/'final.vram').read_bytes()==expected_vram==(root/'cpu_fill_return.vram').read_bytes(),'source and native full first-fill VRAM')
    final_keys=set('masterClock cpu.cycleCount cpu.a cpu.x cpu.y cpu.sp cpu.ps cpu.k cpu.pc cpu.dbr cpu.d cpu.emulationMode ppu.vramAddress ppu.vramReadBuffer dmaController.channel[1].srcAddress dmaController.channel[1].srcBank dmaController.channel[1].transferSize dmaController.channel[1].transferMode dmaController.channel[1].fixedTransfer dmaController.channel[1].invertDirection dmaController.channel[1].decrement dmaController.channel[1].dmaActive'.split())
    final_c=state(out/'final.state');final_n=state(root/'cpu_fill_return.state');check(set(final_c)==final_keys,'final typed field closure')
    for k in final_keys:check(final_c[k]==final_n[k],'final typed field '+k)
    report={'result':'PASS bounded normal reset/upload/F1/firstDMA; full S1/03DB remains OPEN','cpu_instruction_states':len(ccpu),'cpu_and_DMA_data_accesses':len(cbus),'cpu_data_accesses':len(cbus)-protocol['DMA_operations'],'spc_instruction_states':len(ss),'spc_write_io_accesses':len(ns),'scalar_fields':2*len(scalar_keys),'wram_bytes':131072,'aram_bytes':131072,'vram_bytes':65536,'final_typed_fields':len(final_keys),'profile':'pinned software NTSC zero RAM +40Hz SPC','SPC_master_callback_parity':False,'excluded':'SPC DSP RAM reads/full DSP state, concurrent CPU after80C0, otherDMA/HDMA, NMI and03DB','executable_sha256':sha(exe),'native_manifest_sha256':sha(root/'manifest.json'),'trace_sha256':sha(out/'events.jsonl'),'summary':summary,'protocol_v2':protocol,'native_boundary_bindings':len(n['boundaries']),'verifier_sources':{p.name:sha(p) for p in (Path(__file__),Path(__file__).with_name('bootstrap_boundary_protocol_v2.py'),Path(__file__).with_name('bootstrap_fill_trace_protocol.py'),Path(__file__).with_name('bootstrap_trace_protocol_v2.py'))},'build':bm}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='build'},indent=2))
if __name__=='__main__':main()
