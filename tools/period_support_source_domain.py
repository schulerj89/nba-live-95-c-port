"""Native adapter preconditions implied by typed period support inputs."""
import struct
def need(ok,message):
    if not ok:raise ValueError(message)
def word(raw,address):return struct.unpack_from('<H',raw,address)[0]
def offset(address):return((address>>16)&127)*32768+(address&32767)
def validate_source_domain(raw,mode,rom):
    need(type(raw)is bytes and len(raw)==131072,'complete before-state')
    if mode=='sort':need(word(raw,0x34d1)==0,'D5DB requires original zero leading sentinel')
    elif mode=='assignment':
        need(all(word(raw,0x34eb+i*256)==i for i in range(10)),'D85E requires parent-published actor IDs')
        for side in range(2):
            team=word(raw,0x46eb+side*128);need(team<29,'selected ROM team')
            entry=offset(0x84e640)+team*4
            pointer=int.from_bytes(rom[entry+1:entry+4],'little')
            for slot in range(12):
                relative=struct.unpack_from('<H',rom,offset(pointer)+slot*2)[0]
                carried=struct.unpack_from('<I',raw,0x3471+side*48+slot*4)[0]
                need(carried==pointer+relative,'D7B8 carried roster table must match selected team ROM records')
    elif mode!='attachment':raise ValueError('unknown component')
