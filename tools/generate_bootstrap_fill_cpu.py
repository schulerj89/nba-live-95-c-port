"""Generate only NBA95's fixed reset/upload source labels, never an interpreter.

No capture inputs. Decoder is the same pinned static source reader used by the
accepted source-work modules. Generated instructions still perform live reads.
"""
import argparse, hashlib, sys
from collections import deque
from pathlib import Path
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
DECODER_SHA='b1864664d3ac0abcd439055e88bf7220cf66be7a6ae9dfc5d59b3186f1469a46'
BR={'BEQ':'P&2','BNE':'!(P&2)','BMI':'P&128','BPL':'!(P&128)',
    'BCC':'!(P&1)','BCS':'P&1','BVS':'P&64','BVC':'!(P&64)','BRA':'true'}
def scope(pc):return 0x800d<=pc<0x80c0 or 0x8a57<=pc<=0x8a92 or 0xab06<=pc<=0xab7d
def label(s):return 'source_%04x_m%dx%d'%s
def generate(rom,d):
    todo=deque([(0x800d,1,1)]); found={}; edges={}; returns=set()
    while todo:
        s=todo.popleft()
        if s in found or not scope(s[0]):continue
        pc,m,x=s;i=d.decode_insn(rom,pc&0x7fff,pc,0x80,m,x)
        i.m_flag=m;i.x_flag=x;found[s]=i;post=(m,x)
        if i.mnem in ('REP','SEP'):
            f=int(i.mnem=='SEP');post=(f if i.operand&32 else m,f if i.operand&16 else x)
        if pc in (0xab17,0x8a68):post=(0,0)
        nxt=(pc+i.length,*post);es=[nxt]
        if i.mnem in BR:es=[(i.operand,*post)]+([]if i.mnem=='BRA' else[nxt])
        if i.mnem in ('JML','JMP'):es=[(i.operand&65535,*post)]
        if i.mnem=='JSL':
            es=[(i.operand&65535,*post),(pc+i.length,0,0)];returns.add(es[1])
        if i.mnem=='RTL':es=[]
        edges[s]=es;todo.extend(es)
    def go(s):return 'goto '+label(s)+';' if s in found else f'return boundary(s,0x80{s[0]:04x}u);'
    out=['/* Generated canonical reset/upload/first-fill source. No opcode dispatch. */',
         'static bool advance(NbaBootstrapCpu *s) { switch(B.resume) { case 0: goto source_800d_m1x1;']
    for state,i in sorted(found.items()):
        pc,m,x=state;w=1 if m else 2;xw=1 if x else 2;nxt=(pc+i.length,m,x);b=[];o=i.operand
        def addr():
            if i.mode==d.DP:return f'0x{o:x}u','false'
            if i.mode==d.DP_X:return f'(uint16_t)(0x{o:x}u+X)','true'
            if i.mode==d.ABS:return f'DB(0x{o:x}u)','false'
            if i.mode==d.ABS_Y:return f'DB(0x{o:x}u+Y)',f'index_idle(s,0x{o:x},Y)'
            raise ValueError(str(i))
        if i.mnem in ('LDA','LDX','LDY','CMP','CPX','ADC'):
            width=xw if i.mnem in ('LDX','LDY','CPX')else w
            if i.mode==d.IMM:b+=[f'IMM(0x{pc:x},{i.length});'];v=f'0x{o:x}u'
            elif i.mode==d.INDIR_LY:b+=[f'LONG_READ(0x{pc:x},0x{o:x},{width});'];v='B.read_value'
            else:
                a,index=addr();b+=[f'READ(0x{pc:x},{i.length},{a},{width},{index});'];v='B.read_value'
            if i.mnem.startswith('LD'):b+=[f'set_{i.mnem[-1].lower()}(s,{v});']
            elif i.mnem in ('CMP','CPX'):b+=[f'compare(s,{"X" if i.mnem=="CPX" else "A"},{v},{width});']
            else:b+=[f'add(s,{v});']
        elif i.mnem in ('STA','STZ','STX','STY'):
            a,index=addr();value={'STA':'A','STZ':'0','STX':'X','STY':'Y'}[i.mnem];width=xw if i.mnem in ('STX','STY')else w
            b+=[f'WRITE(0x{pc:x},{i.length},{a},{width},{index},{value});']
        elif i.mnem in ('INX','INY','DEX'):
            r=i.mnem[-1];b+=[f'IMP(0x{pc:x});',f'set_{r.lower()}(s,{r}{"-" if i.mnem=="DEX" else "+"}1u);']
        elif i.mnem in ('INC','ROL'):
            b+=[f'IMP(0x{pc:x});',f'accumulator_change(s,SOUND_{i.mnem});']
        elif i.mnem in ('TAX','TAY'):b+=[f'IMP(0x{pc:x});',f'set_{i.mnem[-1].lower()}(s,A);']
        elif i.mnem=='TXS':b+=[f'IMP(0x{pc:x});','B.registers.stack_pointer=X;']
        elif i.mnem=='TCD':b+=[f'IMP(0x{pc:x});','if(A) return boundary(s,0x00801bu);','nz(s,A,2);']
        elif i.mnem=='CLC':b+=[f'IMP(0x{pc:x});','P&=0xfeu;']
        elif i.mnem=='XCE':b+=[f'IMP(0x{pc:x});','s->emulation=(P&1)!=0; P|=1;','if(s->emulation) return boundary(s,0x00800eu);']
        elif i.mnem in ('REP','SEP'):
            b+=[f'MODE(0x{pc:x});',f'status_set(s,P {"|" if i.mnem=="SEP" else "&"} 0x{o if i.mnem=="SEP" else 255^o:x}u);'];nxt=edges[state][0]
        elif i.mnem=='XBA':b+=[f'SUSPEND(implied(s,0x{pc:x},1,2));','A=(uint16_t)((A<<8)|(A>>8)); nz(s,A,1);']
        elif i.mnem in ('PHP','PHA','PHB','PHK'):
            v,width={'PHP':('P',1),'PHA':('A',w),'PHB':('B.registers.data_bank',1),'PHK':('s->program_bank',1)}[i.mnem]
            b+=[f'PUSH(0x{pc:x},1,{v},{width});']
        elif i.mnem in ('PLP','PLA','PLB'):
            b+=[f'PULL(0x{pc:x},{w if i.mnem=="PLA" else 1},false);']
            b+=[{'PLP':'status_set(s,B.read_value);','PLA':'set_a(s,B.read_value);','PLB':'B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);'}[i.mnem]]
            if pc in (0xab17,0x8a68):nxt=(pc+1,0,0)
        elif i.mnem in BR:
            b+=[f'BRANCH(0x{pc:x},({BR[i.mnem]})!=0);','if(B.branch) { '+go((o,m,x))+' }']
        elif i.mnem in ('JML','JMP'):
            b+=[f'IMM(0x{pc:x},4);','s->program_bank=0x80;',go((o&65535,m,x))];nxt=None
        elif i.mnem=='JSL':
            b+=[f'SUSPEND(jsl_call(s,0x{pc:x}));','B.registers.stack_pointer-=3u;',go((o&65535,m,x))];nxt=None
        elif i.mnem=='RTL':
            b+=[f'PULL(0x{pc:x},3,false);','if(s->read_bank!=0x80)return boundary(s,0x80ab7du);','switch((uint16_t)(B.read_value+1u)){']
            b += [f'case 0x{r[0]:x}: '+go(r)for r in sorted(returns)]
            b+=['default:return boundary(s,0x80ab7du);}'];nxt=None
        else:raise ValueError(str(i))
        if nxt is not None:b.append(go(nxt))
        out.append(label(state)+': /* '+str(i)+' */');out+=['    '+line for line in b]
    out+=['default:return boundary(s,0); }}',''];return '\n'.join(out),found
def main():
    p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--decoder-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
    rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()==ROM_SHA
    assert hashlib.sha256((a.decoder_root/'snes65816.py').read_bytes()).hexdigest()==DECODER_SHA
    sys.path.insert(0,str(a.decoder_root));import snes65816 as d
    result,found=generate(rom,d)
    if a.check:assert a.output.read_text()==result
    else:a.output.write_text(result)
    print('PASS static source states',len(found))
if __name__=='__main__':main()
