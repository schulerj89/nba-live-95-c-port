"""Bounded first reset DMA trace/profile guard, not a generic DMA emulator."""
from bootstrap_trace_protocol_v2 import Clock,speed,groups,require,BIOS

def byte(rom,address):
    if 0xffc0<=address<=0xffff:return BIOS[address-0xffc0]
    require(0x380<=address<=0x3dc,'SPC init source closure')
    return rom[0x4687+address-0x380]

def shape(entry,rom):
    op=byte(rom,entry['pc'])
    table={0xcd:(2,2),0xbd:(1,2),0xe8:(2,2),0xc6:(1,4),0x1d:(1,2),
           0x8f:(3,5),0x78:(3,5),0xeb:(2,3),0x7e:(2,3),0xe4:(2,3),
           0xcb:(2,4),0xd7:(2,7),0xfc:(1,2),0xab:(2,4),0xba:(2,5),
           0xda:(2,5),0xc4:(2,4),0xdd:(1,2),0x5d:(1,2),0x1f:(3,6),
           0x20:(1,2),0xd4:(2,5),0xfd:(1,2),0xd6:(3,6),0x8d:(2,2),
           0xdc:(1,2),0x80:(1,2),0xa4:(2,3),0xf8:(2,3)}
    if op in (0xd0,0x10,0x2f):
        take=op==0x2f or not(entry['ps']&(2 if op==0xd0 else 128))
        return 2,4 if take else 2
    if op==0xfe:return 2,6 if (entry['y']-1)&255 else 4
    require(op in table,'SPC concrete opcode')
    return table[op]

def validate(trace,summary,rom,decoder):
    for e in trace:
        if e['kind']in(0,1,7,8):
            limit=65535 if e['kind']==1 else 0xffffff
            require(0<=e['address']<=limit and 0<=e['pc']<=limit,'fill bus address/PC width')
    clock=Clock();fast=False;cpu_count=0;pending=False;delay=False
    ram=bytearray(131072);io=bytearray(0x2400);vram=bytearray(65536)
    dma_expected=[];dma_count=0;vaddr=0;vcontrol=0;counter=0
    def word(address):return io[address-0x2000]|io[address-0x1fff]<<8
    def ram_index(address):
        bank=address>>16;lo=address&65535
        if bank in(0x7e,0x7f):return address-0x7e0000
        if not bank&64 and lo<0x2000:return lo
        return None
    for entry,buses in groups(trace,5,0):
        pc=entry['pc']
        require(entry['master']==clock.master and entry['cycles']==cpu_count,'fill CPU entry clock/count')
        require(buses and all(not e['end']for e in buses[:-1])and buses[-1]['end'],'fill complete CPU instruction')
        ins=decoder.decode_insn(rom,((pc>>16)&127)*32768+(pc&32767),pc&65535,pc>>16,(entry['ps']>>5)&1,(entry['ps']>>4)&1)
        require([e['address']for e in buses if e['bus']==0 and pc<=e['address']<pc+ins.length]==list(range(pc,pc+ins.length)),'fill ordered unique CPU fetches')
        for e in buses:
            n=6 if e['bus']==2 else speed(e['address'],fast)
            if pending:
                if delay:delay=False
                else:
                    require(io[0x2310]==9 and io[0x2311]==24 and io[0x2314]==0 and word(0x4312)==22 and word(0x4315)==0,'normal channel1 fixed source config')
                    require(vcontrol==128 and io[0x100]&128 and vaddr==0 and dma_count==0,'normal VRAM first fill config')
                    counter=8-(clock.master&7);clock.advance(counter);clock.advance(8);counter+=8;clock.advance(8);counter+=8
                    size=word(0x4315)or 65536
                    for index in range(size):
                        clock.advance(4);value=ram[word(0x4312)]
                        dma_expected.append((7,clock.master,clock.spc,pc,22,value,0))
                        clock.advance(4);address=0x2118+(index&1)
                        dma_expected.append((7,clock.master,clock.spc,pc,address,value,1))
                        vram[vaddr*2+(index&1)]=value
                        if index&1:vaddr=(vaddr+1)&0x7fff
                    # Pinned reference has a byte-sized RunDma i; preserve
                    # that SOFTWARE synchronization rule, not a fitted cost.
                    counter+=8*(size&255);clock.advance(n-counter%n)
                    dma_expected.append((8,clock.master,clock.spc,pc,0,0,2))
                    dma_count+=size;pending=False
            if e['bus']==0:clock.advance(n-4);sample=clock.master;clock.advance(4)
            else:clock.advance(n);sample=clock.master
            require((e['sample_master'],e['master'],e['spc'])==(sample,clock.master,clock.spc),'fill CPU sample/completion clocks')
            cpu_count+=1
            if e['bus']==1:
                index=ram_index(e['address'])
                if index is not None:ram[index]=e['value']
                lo=e['address']&65535
                if not(e['address']>>16)&64 and 0x2000<=lo<=0x43ff:
                    io[lo-0x2000]=e['value']
                    if lo==0x420d:fast=bool(e['value']&1)
                    if lo==0x2115:vcontrol=e['value']
                    if lo==0x2116:vaddr=(vaddr&0x7f00)|e['value']
                    if lo==0x2117:vaddr=(vaddr&255)|((e['value']&127)<<8)
                    if lo==0x420b and e['value']:
                        require(e['value']==2 and not pending,'only first DMA pending');pending=True;delay=True
    dma=[e for e in trace if e['kind']in(7,8)]
    require(len(dma)==len(dma_expected),'all DMA operations once')
    for e,want in zip(dma,dma_expected):
        require((e['kind'],e['master'],e['spc'],e['pc'],e['address'],e['value'],e['bus'])==want and e['sample_master']==e['master']and not e['end'],'DMA bus/sync profile position')
    require(not pending and dma_count==65536,'normal DMA completion')
    require((summary['master'],summary['cpu_cycles'],summary['spc_ticks'],summary['refresh'])==(clock.master,cpu_count,clock.spc,clock.refreshes),'fill summary clocks')
    require((summary['dma_bytes'],summary['vram_address'],summary['sync_counter'],summary['source_index'])==(dma_count,vaddr,counter,dma_count&255),'fill summary hardware')
    sbus=[e for e in trace if e['kind']==1]
    require(len(sbus)==len(clock.due),'fill due SPC cycle closure')
    for e,(master,ticks)in zip(sbus,clock.due):
        require((e['master'],e['sample_master'],e['spc'])==(master,master,ticks),'fill exact SPC deadline/cycles')
    instructions=groups(trace,6,1)
    require({k:instructions[0][0][k]for k in('pc','cycles','a','x','y','sp','ps')}==
            {'pc':0xffc0,'cycles':4,'a':0,'x':0,'y':0,'sp':255,'ps':0},'fill canonical power-on SPC')
    for index,(entry,buses)in enumerate(instructions):
        require(buses and entry['cycles']==buses[0]['spc']-2 and entry['master']==buses[0]['master'],'fill SPC entry')
        length,count=shape(entry,rom);op=byte(rom,entry['pc'])
        require(len(buses)<=count and(index==len(instructions)-1 or len(buses)==count),'fill SPC cycle count')
        for phase,e in enumerate(buses):
            require(e['spc']==entry['cycles']+2*(phase+1)and e['end']==(phase+1==count),'fill SPC phase/completion')
            fetch=(phase==0 or phase==3)if op==0xfe else phase<length
            if fetch:
                address=entry['pc']+(1 if op==0xfe and phase==3 else phase)
                require((e['bus'],e['address'],e['value'])==(1,address,byte(rom,address)),'fill SPC fetch sequence')
            else:require(e['bus']!=1,'fill extra SPC fetch')
        if op==0xd7:
            # [dp]+Y stores: the same pointer reads feed both the read-before-
            # write and write cycles. These final two cycles can be beyond
            # the native observer's last lazy catch-up and still need binding.
            require(len(buses)>=4 and (buses[2]['bus'],buses[2]['address'],buses[3]['bus'],buses[3]['address'])==(2,0,2,1),'indirect source pointer reads')
            address=(buses[2]['value']+(buses[3]['value']<<8)+entry['y'])&65535
            if len(buses)>5:require((buses[5]['bus'],buses[5]['address'])==(2,address),'indirect source read before write')
            if len(buses)>6:require((buses[6]['bus'],buses[6]['address'],buses[6]['value'])==(3,address,entry['a']),'indirect source write')
    entry,buses=instructions[-1];length,count=shape(entry,rom)
    if len(buses)<count:expected_pc=entry['pc'];expected_phase=len(buses)
    else:
        # Current first-fill checkpoint ends on the indirect store03CA.
        # Refuse to infer a branch successor for a future expanded endpoint.
        require(entry['pc']==0x3ca and byte(rom,entry['pc'])==0xd7,'owned final SPC successor')
        expected_pc=entry['pc']+length;expected_phase=0
    require((summary['spc_pc'],summary['spc_phase'])==(expected_pc,expected_phase),'fill final SPC continuation')
    markers=[e for e in trace if e['kind']in(2,3)]
    require([e['kind']for e in markers]==[3,2],'fill handoff markers')
    previous=None
    for e in trace:
        if e['kind']==1:previous=e
        if e['kind']in(2,3):
            require(previous is not None and previous['end']and e['master']==previous['master']and e['spc']==previous['spc']and e['sample_master']==e['master'],'fill marker cycle binding')
            if e['kind']==3:require(previous['pc']==0xfffb and(e['pc'],e['address'],e['value'],e['bus'],e['end'])==(0x380,0,0,4,False),'fill IPL handoff')
            else:require((previous['pc'],previous['address'],previous['bus'],previous['value'])==(0x384,0xf1,3,0x30)and(e['pc'],e['address'],e['value'],e['bus'],e['end'])==(0x384,0xf1,0x30,3,True),'fill F1 write')
    require(summary['upload']==sum(e['pc']==0xffe2 and e['bus']==3 for e in sbus),'fill upload stores')
    return {'CPU_cycles':cpu_count,'SPC_cycles':len(sbus),'DMA_bytes':dma_count,'DMA_operations':len(dma)-1,'refreshes':clock.refreshes},bytes(vram)
