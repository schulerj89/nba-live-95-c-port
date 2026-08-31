"""Diagnostic dataflow for the fixed original formation blocks, not a CPU.

Read table addresses, DP operands, negations, rotations and swaps from their
actual ROM instruction bytes. There is no opcode dispatch, machine state,
instruction/timing replay, external call, or production initialization here.
The six regulation paths are transcribed as original block destinations rather
than the C helper's swap/flip predicates. All arithmetic assumes M=X=D=0.
"""
import hashlib,struct

ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'

def block(rom,pc,count):
    off=((pc>>16)&0x7f)*0x8000+(pc&0x7fff)
    return rom[off:off+count]
def word(rom,pc):return struct.unpack('<H',block(rom,pc,2))[0]
def require(ok,message):
    if not ok:raise ValueError(message)

def negative(rom,pc,dp):
    # Exactly LDA dp; EOR #word; INC A; STA same-dp at a proven source block.
    b=block(rom,pc,8)
    require(b[0]==0xa5 and b[2]==0x49 and b[5:7]==b'\x1a\x85'and b[1]==b[7],'original negation block')
    dp[b[7]]=(((dp[b[1]]^int.from_bytes(b[3:5],'little'))+1)&65535)

def rotate(rom,pc,dp):
    # Exactly LDA dp; SEC; SBC #word; AND #word; STA same-dp, binary mode.
    b=block(rom,pc,11)
    require(b[0]==0xa5 and b[2:4]==b'\x38\xe9'and b[6]==0x29 and b[9]==0x85 and b[1]==b[10],'original rotation block')
    dp[b[10]]=((dp[b[1]]-int.from_bytes(b[4:6],'little'))&65535)&int.from_bytes(b[7:9],'little')

def exchange(rom,pc,dp):
    # PHY; three LDA first/LDY second/STY first/STA second pairs; PLY.
    b=block(rom,pc,26);require(b[0]==0x5a and b[-1]==0x7a,'original swap block')
    for off in (1,9,17):
        q=b[off:off+8]
        require(q[0]==0xa5 and q[2]==0xa4 and q[4]==0x84 and q[6]==0x85 and q[1]==q[5]and q[3]==q[7],'original swap operands')
        a,y=dp[q[1]],dp[q[3]];dp[q[5]]=y;dp[q[7]]=a

def formation_pair(rom,period,tip,anchor,pair):
    require(0<=period<=4 and tip in (0,5)and 0<=pair<5,'bounded reference inputs')
    # DD97..DDA4 supplies the table choice; E045..E04A supplies its stride.
    require(block(rom,0x86dd97,13)==bytes.fromhex('ad2609f008c904001003a22800'),'original table branch')
    require(block(rom,0x86e045,6)==bytes.fromhex('8a18690800aa'),'original table stride')
    x=(0 if period==0 or period>=word(rom,0x86dd9d)else word(rom,0x86dda2))+pair*word(rom,0x86e048)
    dp={}
    # Six original LDA long,X / STA dp pairs; no shared C formation table.
    for i in range(6):
        b=block(rom,0x86ddaf+i*6,6)
        require(b[0]==0xbf and b[4]==0x85,'original table load/store')
        address=int.from_bytes(b[1:4],'little')+x
        dp[b[5]]=word(rom,address)
    if x<40:
        require(block(rom,0x86ddd8,7)==bytes.fromhex('a5b610034cdede'),'original anchor branch')
        if anchor>=0:
            for pc in (0x86dddf,0x86dde7):negative(rom,pc,dp)
            rotate(rom,0x86ddef,dp)
        # Opening stores and inline transforms at DEDE..DF24. In particular,
        # Y's source EOR/INC at DEFD/DF00 is present for every opening pair.
        require(block(rom,0x86dede,18)==bytes.fromhex('a5ae99040099560049ffff1a990405995605'),'original opening X stores')
        require(block(rom,0x86def0,23)==bytes.fromhex('a5ba990800995800e02800100449ffff1a990805995805'),'original opening Y stores')
        require(block(rom,0x86df07,32)==bytes.fromhex('a5be994e00995000995200e02800100738e90400290700994e05995005995205'),'original opening direction stores')
        first=(dp[block(rom,0x86dede,2)[1]],dp[block(rom,0x86def0,2)[1]],dp[block(rom,0x86df07,2)[1]])
        second=((((first[0]^word(rom,0x86dee7))+1)&65535),(((first[1]^word(rom,0x86defe))+1)&65535),((first[2]-word(rom,0x86df19))&65535)&word(rom,0x86df1c))
        return first,second
    # Original branch destinations: winner==0 starts DE2C, otherwise DE02.
    require(block(rom,0x86ddfd,18)==bytes.fromhex('ad3209f02aad2609c90100f04cc90200d061'),'original winner/nonzero branch')
    require(block(rom,0x86de2c,13)==bytes.fromhex('ad2609c90100f072c90200f037'),'original winner-zero branch')
    routes={(1,0):(),(2,0):(0x86de70,),(3,0):(0x86de39,),
            (1,5):(0x86de56,0x86de70),(2,5):(0x86de0f,),(3,5):(0x86de70,)}
    for pc in routes[period,tip]:
        if pc==0x86de70:
            for at in (0x86de70,0x86de78,0x86de80,0x86de88):negative(rom,at,dp)
            for at in (0x86de90,0x86de9b):rotate(rom,at,dp)
        else:exchange(rom,pc,dp)
    require(block(rom,0x86dea6,56)==bytes.fromhex('a5ae990400995600a5b0990405995605a5ba990800995800a5bc990805995805a5be994e00995000995200a5c0994e059950059952058049'),'original regulation stores')
    return (dp[0xae],dp[0xba],dp[0xbe]),(dp[0xb0],dp[0xbc],dp[0xc0])

def witness(rom):
    require(hashlib.sha256(rom).hexdigest()==ROM_SHA,'original ROM identity')
    cases=[]
    for period in range(5):
        for tip in (0,5):
            for anchor in (-336,336):
                cases.append({'period':period,'tip':tip,'anchor':anchor,'pairs':[formation_pair(rom,period,tip,anchor,i)for i in range(5)]})
    return {'scope':'fixed original source-block dataflow; binary M=X=D=0; no generic CPU or cycle model','rom_sha256':ROM_SHA,'source_range_sha256':hashlib.sha256(block(rom,0x86dd97,0xdf27-0xdd97)).hexdigest(),'cases':cases}
