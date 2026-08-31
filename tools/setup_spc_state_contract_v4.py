"""Exact scalar Map schema from pinned Mesen serialization; no DSP execution.

Spc.cpp/SpcTypes.h/SpcTimer.h and DSP Dsp{Types,Voice}.{h,cpp}, commit
b9fa69ddc6d0a331fb103fdb5eef6904305703c2. Serializer::WriteMapFormat omits
enums; StreamArray omits arrays longer than 64. No key is inferred from a
native fixture, and none of these values initializes production hardware.
"""
import math,re

def check(ok,message):
    if not ok:raise ValueError(message)

def schema(control=False):
    out={}
    def add(prefix,names,domain):
        for name in names.split():out[prefix+name]=domain
    add('spc.','a x y sp ps dspReg',(0,255))
    add('spc.','pc',(0,65535));add('spc.','cycle',(0,2**63-1))
    add('spc.','internalSpeed externalSpeed',(0,3))
    add('spc.','writeEnabled romEnabled timersEnabled timersDisabled','bool')
    add('spc.','clockRatio','ratio')
    for name,count in (('cpuRegs',4),('outputReg',4),('ramReg',2)):
        for i in range(count):out[f'spc.{name}[{i}]']=(0,255)
    for i in range(3):
        pre=f'spc.timer{i}.'
        add(pre,'stage0',(0,15 if i==2 else 127))
        add(pre,'stage1 prevStage1',(0,1))
        add(pre,'stage2 output target',(0,255))
        add(pre,'enabled timersEnabled','bool')
    add('spc.dsp.','noiseLfsr voiceOutput pitch',(-2**31,2**31-1))
    add('spc.dsp.','counter sampleAddress brrNextAddress echoPointer echoLength echoOffset',(0,65535))
    add('spc.dsp.','step outRegBuffer envRegBuffer voiceEndBuffer dirSampleTableAddress noiseOn pitchModulationOn keyOn newKeyOn keyOff everyOtherSample sourceNumber brrHeader brrData looped adsr1 echoHistoryPos echoRingBufferAddress echoOn',(0,255))
    add('spc.dsp.','echoEnabled','bool')
    for name in ('outSamples','echoIn','echoOut'):
        for i in range(2):out[f'spc.dsp.{name}{i}']=(-2**31,2**31-1)
    for i in range(16):out[f'spc.dsp.echoHistory{i}']=(-32768,32767)
    for i in range(8):
        pre=f'spc.dsp.voices[{i}].'
        add(pre,'envVolume prevCalculatedEnv interpolationPos',(-2**31,2**31-1))
        add(pre,'brrAddress brrOffset',(0,65535))
        add(pre,'voiceBit keyOnDelay envOut bufferPos',(0,255))
        for j in range(12):out[f'{pre}sampleBuffer{j}']=(-32768,32767)
    check(len(out)==268,'source scalar schema closure')
    if control:out.update(source_pc=(0,65535),value=(0,255))
    return out

def validate(s,control=False):
    domains=schema(control)
    check(type(s)is dict and set(s)==set(domains),'required SPC scalar state keys')
    for name,domain in domains.items():
        text=s[name];check(type(text)is str,'SPC scalar text '+name)
        if domain=='bool':check(text in ('true','false'),'SPC boolean '+name)
        elif domain=='ratio':
            check(re.fullmatch(r'(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?',text)is not None,'SPC ratio text')
            value=float(text);check(math.isfinite(value)and value>0,'SPC finite positive clock ratio')
        else:
            check(re.fullmatch(r'(?:0|-?[1-9][0-9]*)',text)is not None,'SPC canonical integer '+name)
            check(domain[0]<=int(text)<=domain[1],'SPC numeric domain '+name)
    return s

def pending_dsp(s,instruction,read):
    """03DB OR A,F3 read callback: PC consumed two bytes, A/PS not updated.

    IncCycleCount precedes ProcessMemoryRead. With captured normal speed zero,
    opcode, DP operand, and read each add 2 hardware clocks. The read value is
    unresolved to the C source continuation; it is not compared or supplied.
    """
    validate(s)
    check(instruction[0]==0x3db and read[:2]==(0x3db,0xf3),'pending DSP source instruction')
    check(int(s['spc.pc'])==0x3dd,'pending DSP consumed PC')
    for index,key in enumerate(('a','x','y','sp','ps'),1):
        check(int(s['spc.'+key])==instruction[index],'pending DSP register relation '+key)
    check(int(s['spc.cycle'])==instruction[-1]+6==read[-1],'pending DSP read clock relation')

def control_boundary(s,source_pc,value):
    validate(s,control=True)
    check(int(s['source_pc'])==source_pc and int(s['value'])==value,'F1 source publication')
    # 8F immediate DP consumes all three source bytes before the write callback;
    # the following instruction callback observes the same PC and SPC cycle.
    check(int(s['spc.pc'])==source_pc+3,'F1 consumed instruction PC')
    # Spc::Write invokes this observer only inside the WriteEnabled branch.
    # This is a capture precondition, not a restriction on the C commit API:
    # F1 register effects still execute when underlying ARAM writes are disabled.
    check(s['spc.writeEnabled']=='true','F1 native write callback requires ARAM write enable')
