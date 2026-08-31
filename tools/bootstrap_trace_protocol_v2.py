"""Source-profile protocol checks, independent of reported C timestamps.

This validates the bounded pre-DMA trace only. It reconstructs bus clocks from
ordered CPU requests, not observed elapsed costs, and makes no native lazy-SPC
callback-master parity claim. No candidate executable state is used as input.
"""

BIOS=bytes.fromhex('cdefbde800c61dd0fc8faaf48fbbf578ccf4d0fb2f19ebf4d0fc7ef4d00be4f5cbf4d700fcd0f3ab0110ef7ef410ebbaf6da00baf4c4f4dd5dd0db1f0000c0ff')

def require(ok,message):
    if not ok:raise ValueError('v2 protocol: '+message)

class Clock:
    """Pinned MemoryManager startup/refresh and SPC default clock profile."""
    def __init__(self):
        self.master=186;self.h=186;self.line=0;self.odd=False
        self.refresh_at=538;self.refreshes=0;self.spc=4;self.due=[]
    def tick(self):
        self.master+=2;self.h+=2
        if self.h==self.refresh_at:
            self.refreshes+=1
            for _ in range(20):self.tick()
        if self.h>=1364 or(self.h==1360 and self.line==240 and self.odd):
            self.h=0;self.line+=1
            if self.line==262:self.line=0;self.odd=not self.odd
            self.refresh_at=538-(self.master&7)
        target=self.master*2050560//21477270
        while self.spc+1<target:
            self.spc+=2;self.due.append((self.master,self.spc))
    def advance(self,n):
        require(n>=0 and n%2==0,'clock step domain')
        for _ in range(n//2):self.tick()

def speed(address,fast):
    bank=address>>16;lo=address&65535
    if bank&64:return 6 if bank>=192 and fast else 8
    if lo>=32768:return 6 if bank>=128 and fast else 8
    if lo<8192 or lo>=24576:return 8
    return 12 if 16384<=lo<16896 else 6

def groups(trace,entry_kind,bus_kind):
    result=[]
    for event in trace:
        if event['kind']==entry_kind:result.append([event,[]])
        elif event['kind']==bus_kind:
            require(bool(result),'bus before instruction entry')
            result[-1][1].append(event)
    return result

def spc_byte(rom,address):
    if 0xffc0<=address<=0xffff:return BIOS[address-0xffc0]
    require(0x380<=address<=0x38f,'SPC source beyond checkpoint closure')
    return rom[0x4687+address-0x380]

def spc_shape(entry,rom):
    # Concrete opcodes reached in IPL and0380..038E. Length/cycle ownership is
    # read from pinned SPC addressing/operation source, not a captured schedule.
    op=spc_byte(rom,entry['pc'])
    shape={0xcd:(2,2),0xbd:(1,2),0xe8:(2,2),0xc6:(1,4),0x1d:(1,2),
           0x8f:(3,5),0x78:(3,5),0xeb:(2,3),0x7e:(2,3),0xe4:(2,3),
           0xcb:(2,4),0xd7:(2,7),0xfc:(1,2),0xab:(2,4),0xba:(2,5),
           0xda:(2,5),0xc4:(2,4),0xdd:(1,2),0x5d:(1,2),0x1f:(3,6),
           0x20:(1,2),0xd4:(2,5)}
    if op in (0xd0,0x10,0x2f):
        taken=op==0x2f or not(entry['ps']&(2 if op==0xd0 else 128))
        return 2,4 if taken else 2
    require(op in shape,'SPC opcode outside concrete closure')
    return shape[op]

def validate_trace(trace,summary,rom,decoder):
    for event in trace:
        if event['kind'] in (0,1):
            limit=0xffffff if event['kind']==0 else 65535
            require(0<=event['address']<=limit and 0<=event['pc']<=limit,'source bus address/PC width')
    cpu=groups(trace,5,0);spc=groups(trace,6,1)
    require(cpu and spc,'empty instruction stream')
    clock=Clock();fast=False;cpu_count=0
    for entry,buses in cpu:
        pc=entry['pc']
        require(entry['master']==clock.master and entry['cycles']==cpu_count,'CPU entry clock/count')
        require(buses and all(not e['end'] for e in buses[:-1]) and buses[-1]['end'],'complete CPU instruction including terminal')
        ins=decoder.decode_insn(rom,((pc>>16)&127)*32768+(pc&32767),pc&65535,pc>>16,(entry['ps']>>5)&1,(entry['ps']>>4)&1)
        fetches=[e['address'] for e in buses if e['bus']==0 and pc<=e['address']<pc+ins.length]
        require(fetches==list(range(pc,pc+ins.length)),'one ordered CPU opcode/operand fetch each')
        for e in buses:
            n=6 if e['bus']==2 else speed(e['address'],fast)
            if e['bus']==0:
                clock.advance(n-4);sample=clock.master;clock.advance(4)
            else:
                clock.advance(n);sample=clock.master
            require((e['sample_master'],e['master'],e['spc'])==(sample,clock.master,clock.spc),'CPU sample/completion/SPC clocks')
            cpu_count+=1
            if e['bus']==1 and not((e['address']>>16)&64) and e['address']&65535==0x420d:
                fast=bool(e['value']&1)
    require(summary['master']==clock.master and summary['cpu_cycles']==cpu_count,'summary CPU counters')
    require(summary['refresh']==clock.refreshes and summary['spc_ticks']==clock.spc,'summary refresh/SPC counters')
    sbus=[e for e in trace if e['kind']==1]
    require(len(sbus)==len(clock.due),'all due SPC cycles present once')
    for e,(master,ticks) in zip(sbus,clock.due):
        require((e['master'],e['sample_master'],e['spc'])==(master,master,ticks),'SPC exact logical deadline/cycle')
    require({k:spc[0][0][k] for k in ('pc','cycles','a','x','y','sp','ps')}==
            {'pc':0xffc0,'cycles':4,'a':0,'x':0,'y':0,'sp':255,'ps':0},'canonical source power-on SPC entry')
    for index,(entry,buses) in enumerate(spc):
        require(buses,'SPC entry without cycle')
        require(entry['master']==buses[0]['master'] and entry['cycles']==buses[0]['spc']-2,'SPC entry clock binding')
        length,count=spc_shape(entry,rom)
        require(len(buses)<=count,'SPC source cycle count')
        require(all(e['pc']==entry['pc'] and e['spc']==entry['cycles']+2*(j+1) and e['end']==(j+1==count) for j,e in enumerate(buses)),'SPC phase/end positions')
        require(index==len(spc)-1 or len(buses)==count,'only final SPC instruction may be partial')
        for phase,e in enumerate(buses):
            if phase<length:
                require((e['bus'],e['address'],e['value'])==(1,entry['pc']+phase,spc_byte(rom,entry['pc']+phase)),'ordered SPC opcode/operand fetch')
            else:require(e['bus']!=1,'extra SPC fetch')
    entry,buses=spc[-1]
    _,count=spc_shape(entry,rom)
    # This exact checkpoint ends inside BPL038E. Do not invent a next-PC when
    # an expanded future scope ends after instruction completion.
    require(len(buses)<count,'checkpoint must retain an explicit partial SPC instruction')
    require(summary['spc_pc']==entry['pc'] and summary['spc_phase']==len(buses),'summary SPC continuation')
    markers=[e for e in trace if e['kind'] in (2,3)]
    require([e['kind']for e in markers]==[3,2],'resident/F1 marker sequence')
    previous=None
    for e in trace:
        if e['kind']==1:previous=e
        if e['kind']in(2,3):
            require(previous is not None and previous['end'] and e['master']==previous['master'] and e['spc']==previous['spc'] and e['sample_master']==e['master'],'marker committed cycle binding')
            if e['kind']==3:
                require(previous['pc']==0xfffb and (e['pc'],e['address'],e['value'],e['bus'],e['end'])==(0x380,0,0,4,False),'actual resident handoff')
            else:
                require((previous['pc'],previous['address'],previous['bus'],previous['value'])==(0x384,0xf1,3,0x30) and
                        (e['pc'],e['address'],e['value'],e['bus'],e['end'])==(0x384,0xf1,0x30,3,True),'actual F1 write marker')
    require(summary['upload']==sum(e['pc']==0xffe2 and e['bus']==3 for e in sbus),'source IPL upload stores')
    return {'CPU_cycles':cpu_count,'SPC_cycles':len(sbus),'refreshes':clock.refreshes,'source_profile_deadlines':True,'terminal_CPU_complete':True}
