"""Source hook and complete process-output bindings for bootstrap revisions.

Does not import candidate clocks, mutate artifacts, or compare a lazy SPC
callback master with a logical SPC oscillator deadline.
"""
def require(ok,message):
    if not ok:raise ValueError(message)

def bind_cpu_hook(row,snapshot):
    keys={'a':'a','x':'x','y':'y','sp':'sp','ps':'ps','db':'dbr','dp':'d'}
    for key,field in keys.items():
        raw=snapshot['cpu.'+field]
        require(raw.isascii()and raw.isdecimal()and str(int(raw))==raw,'CPU hook snapshot numeric syntax')
        require(type(row[key])is int and row[key]==int(raw),'CPU hook register binding '+key)
    raw=snapshot['cpu.emulationMode']
    require(raw in ('true','false')and type(row['emulation'])is bool and row['emulation']==(raw=='true'),'CPU hook emulation binding')

def validate_stdout(stdout,trace,read_state):
    lines=stdout.splitlines()
    require(len(lines)==4 and stdout=='\n'.join(lines)+'\n','exact four-line process stdout')
    # nba_rom.c's successful canonical-ROM diagnostic; the verifier has already
    # pinned that ROM's SHA, title/header size and the fresh build's source.
    require(lines[0]=='[ROM] Loaded successfully: "NBA Live \'95         " (Reset: 0x800D, Headered: No, Size: 1536 KiB)','canonical ROM-loader diagnostic')
    markers=[t for t in trace if t['kind']in (2,3)]
    require([t['kind']for t in markers]==[3,2],'stdout source marker closure')
    for line,marker,prefix in zip(lines[1:3],markers,('entry','f1')):
        state=read_state(prefix+'.state')
        def scalar(key):
            raw=state['spc.'+key]
            require(raw.isascii()and raw.isdecimal()and str(int(raw))==raw,'stdout boundary scalar syntax')
            return int(raw)
        upload=sum(t['kind']==1 and t['pc']==0xffe2 and t['bus']==3 and t['master']<=marker['master']for t in trace)
        expected=(f"BOUNDARY kind={marker['kind']} master={marker['master']} ticks={marker['spc']} "
                  f"SPC={scalar('pc'):04X} A={scalar('a'):02X} X={scalar('x'):02X} Y={scalar('y'):02X} "
                  f"SP={scalar('sp'):02X} P={scalar('ps'):02X} upload={upload}")
        require(line==expected,'complete source boundary stdout binding '+prefix)
