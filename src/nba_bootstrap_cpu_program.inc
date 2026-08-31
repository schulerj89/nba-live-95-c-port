/* Generated canonical reset/upload source. No opcode dispatch. */
static bool advance(NbaBootstrapCpu *s) { switch(B.resume) { case 0: goto source_800d_m1x1;
source_800d_m1x1: /* $80:800D [M=1 X=1] CLC    */
    IMP(0x800d);
    P&=0xfeu;
    goto source_800e_m1x1;
source_800e_m1x1: /* $80:800E [M=1 X=1] XCE    */
    IMP(0x800e);
    s->emulation=(P&1)!=0; P|=1;
    if(s->emulation) return boundary(s,0x00800eu);
    goto source_800f_m1x1;
source_800f_m1x1: /* $80:800F [M=1 X=1] REP   #$30 */
    MODE(0x800f);
    status_set(s,P & 0xcfu);
    goto source_8011_m0x0;
source_8011_m0x0: /* $80:8011 [M=0 X=0] STZ   $00CA */
    WRITE(0x8011,3,DB(0xcau),2,false,0);
    goto source_8014_m0x0;
source_8014_m0x0: /* $80:8014 [M=0 X=0] LDX   #$1FFF */
    IMM(0x8014,3);
    set_x(s,0x1fffu);
    goto source_8017_m0x0;
source_8017_m0x0: /* $80:8017 [M=0 X=0] TXS    */
    IMP(0x8017);
    B.registers.stack_pointer=X;
    goto source_8018_m0x0;
source_8018_m0x0: /* $80:8018 [M=0 X=0] LDA   #$00 */
    IMM(0x8018,3);
    set_a(s,0x0u);
    goto source_801b_m0x0;
source_801b_m0x0: /* $80:801B [M=0 X=0] TCD    */
    IMP(0x801b);
    if(A) return boundary(s,0x00801bu);
    nz(s,A,2);
    goto source_801c_m0x0;
source_801c_m0x0: /* $80:801C [M=0 X=0] JMP   $808020 */
    IMM(0x801c,4);
    s->program_bank=0x80;
    goto source_8020_m0x0;
source_8020_m0x0: /* $80:8020 [M=0 X=0] PHK    */
    PUSH(0x8020,1,s->program_bank,1);
    goto source_8021_m0x0;
source_8021_m0x0: /* $80:8021 [M=0 X=0] PLB    */
    PULL(0x8021,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_8022_m0x0;
source_8022_m0x0: /* $80:8022 [M=0 X=0] STZ   $4200 */
    WRITE(0x8022,3,DB(0x4200u),2,false,0);
    goto source_8025_m0x0;
source_8025_m0x0: /* $80:8025 [M=0 X=0] LDA   #$FF */
    IMM(0x8025,3);
    set_a(s,0xffu);
    goto source_8028_m0x0;
source_8028_m0x0: /* $80:8028 [M=0 X=0] STA   $4201 */
    WRITE(0x8028,3,DB(0x4201u),2,false,A);
    goto source_802b_m0x0;
source_802b_m0x0: /* $80:802B [M=0 X=0] LDX   #$4202 */
    IMM(0x802b,3);
    set_x(s,0x4202u);
    goto source_802e_m0x0;
source_802e_m0x0: /* $80:802E [M=0 X=0] STZ   $00,X */
    WRITE(0x802e,2,(uint16_t)(0x0u+X),2,true,0);
    goto source_8030_m0x0;
source_8030_m0x0: /* $80:8030 [M=0 X=0] INX    */
    IMP(0x8030);
    set_x(s,X+1u);
    goto source_8031_m0x0;
source_8031_m0x0: /* $80:8031 [M=0 X=0] CPX   #$420C */
    IMM(0x8031,3);
    compare(s,X,0x420cu,2);
    goto source_8034_m0x0;
source_8034_m0x0: /* $80:8034 [M=0 X=0] BNE   $802E */
    BRANCH(0x8034,(!(P&2))!=0);
    if(B.branch) { goto source_802e_m0x0; }
    goto source_8036_m0x0;
source_8036_m0x0: /* $80:8036 [M=0 X=0] REP   #$20 */
    MODE(0x8036);
    status_set(s,P & 0xdfu);
    goto source_8038_m0x0;
source_8038_m0x0: /* $80:8038 [M=0 X=0] LDA   #$C683 */
    IMM(0x8038,3);
    set_a(s,0xc683u);
    goto source_803b_m0x0;
source_803b_m0x0: /* $80:803B [M=0 X=0] STA   $0C */
    WRITE(0x803b,2,0xcu,2,false,A);
    goto source_803d_m0x0;
source_803d_m0x0: /* $80:803D [M=0 X=0] STZ   $0E */
    WRITE(0x803d,2,0xeu,2,false,0);
    goto source_803f_m0x0;
source_803f_m0x0: /* $80:803F [M=0 X=0] JSL   $80AB06 */
    SUSPEND(jsl_call(s,0x803f));
    B.registers.stack_pointer-=3u;
    goto source_ab06_m0x0;
source_8043_m0x0: /* $80:8043 [M=0 X=0] SEP   #$20 */
    MODE(0x8043);
    status_set(s,P | 0x20u);
    goto source_8045_m1x0;
source_8045_m1x0: /* $80:8045 [M=1 X=0] SEP   #$20 */
    MODE(0x8045);
    status_set(s,P | 0x20u);
    goto source_8047_m1x0;
source_8047_m1x0: /* $80:8047 [M=1 X=0] LDA   #$01 */
    IMM(0x8047,2);
    set_a(s,0x1u);
    goto source_8049_m1x0;
source_8049_m1x0: /* $80:8049 [M=1 X=0] STA   $420D */
    WRITE(0x8049,3,DB(0x420du),1,false,A);
    goto source_804c_m1x0;
source_804c_m1x0: /* $80:804C [M=1 X=0] STZ   $4200 */
    WRITE(0x804c,3,DB(0x4200u),1,false,0);
    goto source_804f_m1x0;
source_804f_m1x0: /* $80:804F [M=1 X=0] LDA   #$8F */
    IMM(0x804f,2);
    set_a(s,0x8fu);
    goto source_8051_m1x0;
source_8051_m1x0: /* $80:8051 [M=1 X=0] STA   $2100 */
    WRITE(0x8051,3,DB(0x2100u),1,false,A);
    goto source_8054_m1x0;
source_8054_m1x0: /* $80:8054 [M=1 X=0] STZ   $2101 */
    WRITE(0x8054,3,DB(0x2101u),1,false,0);
    goto source_8057_m1x0;
source_8057_m1x0: /* $80:8057 [M=1 X=0] STZ   $2102 */
    WRITE(0x8057,3,DB(0x2102u),1,false,0);
    goto source_805a_m1x0;
source_805a_m1x0: /* $80:805A [M=1 X=0] STZ   $2103 */
    WRITE(0x805a,3,DB(0x2103u),1,false,0);
    goto source_805d_m1x0;
source_805d_m1x0: /* $80:805D [M=1 X=0] STZ   $2104 */
    WRITE(0x805d,3,DB(0x2104u),1,false,0);
    goto source_8060_m1x0;
source_8060_m1x0: /* $80:8060 [M=1 X=0] STZ   $2104 */
    WRITE(0x8060,3,DB(0x2104u),1,false,0);
    goto source_8063_m1x0;
source_8063_m1x0: /* $80:8063 [M=1 X=0] LDX   #$2105 */
    IMM(0x8063,3);
    set_x(s,0x2105u);
    goto source_8066_m1x0;
source_8066_m1x0: /* $80:8066 [M=1 X=0] STZ   $00,X */
    WRITE(0x8066,2,(uint16_t)(0x0u+X),1,true,0);
    goto source_8068_m1x0;
source_8068_m1x0: /* $80:8068 [M=1 X=0] INX    */
    IMP(0x8068);
    set_x(s,X+1u);
    goto source_8069_m1x0;
source_8069_m1x0: /* $80:8069 [M=1 X=0] CPX   #$210D */
    IMM(0x8069,3);
    compare(s,X,0x210du,2);
    goto source_806c_m1x0;
source_806c_m1x0: /* $80:806C [M=1 X=0] BNE   $8066 */
    BRANCH(0x806c,(!(P&2))!=0);
    if(B.branch) { goto source_8066_m1x0; }
    goto source_806e_m1x0;
source_806e_m1x0: /* $80:806E [M=1 X=0] STZ   $00,X */
    WRITE(0x806e,2,(uint16_t)(0x0u+X),1,true,0);
    goto source_8070_m1x0;
source_8070_m1x0: /* $80:8070 [M=1 X=0] STZ   $00,X */
    WRITE(0x8070,2,(uint16_t)(0x0u+X),1,true,0);
    goto source_8072_m1x0;
source_8072_m1x0: /* $80:8072 [M=1 X=0] INX    */
    IMP(0x8072);
    set_x(s,X+1u);
    goto source_8073_m1x0;
source_8073_m1x0: /* $80:8073 [M=1 X=0] CPX   #$2115 */
    IMM(0x8073,3);
    compare(s,X,0x2115u,2);
    goto source_8076_m1x0;
source_8076_m1x0: /* $80:8076 [M=1 X=0] BNE   $806E */
    BRANCH(0x8076,(!(P&2))!=0);
    if(B.branch) { goto source_806e_m1x0; }
    goto source_8078_m1x0;
source_8078_m1x0: /* $80:8078 [M=1 X=0] LDA   #$80 */
    IMM(0x8078,2);
    set_a(s,0x80u);
    goto source_807a_m1x0;
source_807a_m1x0: /* $80:807A [M=1 X=0] STA   $2115 */
    WRITE(0x807a,3,DB(0x2115u),1,false,A);
    goto source_807d_m1x0;
source_807d_m1x0: /* $80:807D [M=1 X=0] INX    */
    IMP(0x807d);
    set_x(s,X+1u);
    goto source_807e_m1x0;
source_807e_m1x0: /* $80:807E [M=1 X=0] STZ   $00,X */
    WRITE(0x807e,2,(uint16_t)(0x0u+X),1,true,0);
    goto source_8080_m1x0;
source_8080_m1x0: /* $80:8080 [M=1 X=0] INX    */
    IMP(0x8080);
    set_x(s,X+1u);
    goto source_8081_m1x0;
source_8081_m1x0: /* $80:8081 [M=1 X=0] CPX   #$211B */
    IMM(0x8081,3);
    compare(s,X,0x211bu,2);
    goto source_8084_m1x0;
source_8084_m1x0: /* $80:8084 [M=1 X=0] BNE   $807E */
    BRANCH(0x8084,(!(P&2))!=0);
    if(B.branch) { goto source_807e_m1x0; }
    goto source_8086_m1x0;
source_8086_m1x0: /* $80:8086 [M=1 X=0] LDY   #$00 */
    IMM(0x8086,3);
    set_y(s,0x0u);
    goto source_8089_m1x0;
source_8089_m1x0: /* $80:8089 [M=1 X=0] STZ   $00,X */
    WRITE(0x8089,2,(uint16_t)(0x0u+X),1,true,0);
    goto source_808b_m1x0;
source_808b_m1x0: /* $80:808B [M=1 X=0] LDA   $8000,Y */
    READ(0x808b,3,DB(0x8000u+Y),1,index_idle(s,0x8000,Y));
    set_a(s,B.read_value);
    goto source_808e_m1x0;
source_808e_m1x0: /* $80:808E [M=1 X=0] STA   $00,X */
    WRITE(0x808e,2,(uint16_t)(0x0u+X),1,true,A);
    goto source_8090_m1x0;
source_8090_m1x0: /* $80:8090 [M=1 X=0] INY    */
    IMP(0x8090);
    set_y(s,Y+1u);
    goto source_8091_m1x0;
source_8091_m1x0: /* $80:8091 [M=1 X=0] INX    */
    IMP(0x8091);
    set_x(s,X+1u);
    goto source_8092_m1x0;
source_8092_m1x0: /* $80:8092 [M=1 X=0] CPX   #$2121 */
    IMM(0x8092,3);
    compare(s,X,0x2121u,2);
    goto source_8095_m1x0;
source_8095_m1x0: /* $80:8095 [M=1 X=0] BNE   $8089 */
    BRANCH(0x8095,(!(P&2))!=0);
    if(B.branch) { goto source_8089_m1x0; }
    goto source_8097_m1x0;
source_8097_m1x0: /* $80:8097 [M=1 X=0] STZ   $00,X */
    WRITE(0x8097,2,(uint16_t)(0x0u+X),1,true,0);
    goto source_8099_m1x0;
source_8099_m1x0: /* $80:8099 [M=1 X=0] INX    */
    IMP(0x8099);
    set_x(s,X+1u);
    goto source_809a_m1x0;
source_809a_m1x0: /* $80:809A [M=1 X=0] CPX   #$2130 */
    IMM(0x809a,3);
    compare(s,X,0x2130u,2);
    goto source_809d_m1x0;
source_809d_m1x0: /* $80:809D [M=1 X=0] BNE   $8097 */
    BRANCH(0x809d,(!(P&2))!=0);
    if(B.branch) { goto source_8097_m1x0; }
    goto source_809f_m1x0;
source_809f_m1x0: /* $80:809F [M=1 X=0] LDA   #$30 */
    IMM(0x809f,2);
    set_a(s,0x30u);
    goto source_80a1_m1x0;
source_80a1_m1x0: /* $80:80A1 [M=1 X=0] STA   $2130 */
    WRITE(0x80a1,3,DB(0x2130u),1,false,A);
    goto source_80a4_m1x0;
source_80a4_m1x0: /* $80:80A4 [M=1 X=0] STZ   $2131 */
    WRITE(0x80a4,3,DB(0x2131u),1,false,0);
    goto source_80a7_m1x0;
source_80a7_m1x0: /* $80:80A7 [M=1 X=0] LDA   #$E0 */
    IMM(0x80a7,2);
    set_a(s,0xe0u);
    goto source_80a9_m1x0;
source_80a9_m1x0: /* $80:80A9 [M=1 X=0] STA   $2132 */
    WRITE(0x80a9,3,DB(0x2132u),1,false,A);
    goto source_80ac_m1x0;
source_80ac_m1x0: /* $80:80AC [M=1 X=0] STZ   $2133 */
    WRITE(0x80ac,3,DB(0x2133u),1,false,0);
    goto source_80af_m1x0;
source_80af_m1x0: /* $80:80AF [M=1 X=0] REP   #$20 */
    MODE(0x80af);
    status_set(s,P & 0xdfu);
    goto source_80b1_m0x0;
source_80b1_m0x0: /* $80:80B1 [M=0 X=0] LDA   #$8F */
    IMM(0x80b1,3);
    set_a(s,0x8fu);
    goto source_80b4_m0x0;
source_80b4_m0x0: /* $80:80B4 [M=0 X=0] STA   $0562 */
    WRITE(0x80b4,3,DB(0x562u),2,false,A);
    goto source_80b7_m0x0;
source_80b7_m0x0: /* $80:80B7 [M=0 X=0] LDA   #$00 */
    IMM(0x80b7,3);
    set_a(s,0x0u);
    goto source_80ba_m0x0;
source_80ba_m0x0: /* $80:80BA [M=0 X=0] TAX    */
    IMP(0x80ba);
    set_x(s,A);
    goto source_80bb_m0x0;
source_80bb_m0x0: /* $80:80BB [M=0 X=0] TAY    */
    IMP(0x80bb);
    set_y(s,A);
    return boundary(s,0x8080bcu);
source_ab06_m0x0: /* $80:AB06 [M=0 X=0] PHP    */
    PUSH(0xab06,1,P,1);
    goto source_ab07_m0x0;
source_ab07_m0x0: /* $80:AB07 [M=0 X=0] SEP   #$20 */
    MODE(0xab07);
    status_set(s,P | 0x20u);
    goto source_ab09_m1x0;
source_ab09_m1x0: /* $80:AB09 [M=1 X=0] PHA    */
    PUSH(0xab09,1,A,1);
    goto source_ab0a_m1x0;
source_ab0a_m1x0: /* $80:AB0A [M=1 X=0] PHB    */
    PUSH(0xab0a,1,B.registers.data_bank,1);
    goto source_ab0b_m1x0;
source_ab0b_m1x0: /* $80:AB0B [M=1 X=0] PLA    */
    PULL(0xab0b,1,false);
    set_a(s,B.read_value);
    goto source_ab0c_m1x0;
source_ab0c_m1x0: /* $80:AB0C [M=1 X=0] CMP   #$7E */
    IMM(0xab0c,2);
    compare(s,A,0x7eu,1);
    goto source_ab0e_m1x0;
source_ab0e_m1x0: /* $80:AB0E [M=1 X=0] BEQ   $AB14 */
    BRANCH(0xab0e,(P&2)!=0);
    if(B.branch) { goto source_ab14_m1x0; }
    goto source_ab10_m1x0;
source_ab10_m1x0: /* $80:AB10 [M=1 X=0] CMP   #$7F */
    IMM(0xab10,2);
    compare(s,A,0x7fu,1);
    goto source_ab12_m1x0;
source_ab12_m1x0: /* $80:AB12 [M=1 X=0] BNE   $AB16 */
    BRANCH(0xab12,(!(P&2))!=0);
    if(B.branch) { goto source_ab16_m1x0; }
    goto source_ab14_m1x0;
source_ab14_m1x0: /* $80:AB14 [M=1 X=0] PHK    */
    PUSH(0xab14,1,s->program_bank,1);
    goto source_ab15_m1x0;
source_ab15_m1x0: /* $80:AB15 [M=1 X=0] PLB    */
    PULL(0xab15,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_ab16_m1x0;
source_ab16_m1x0: /* $80:AB16 [M=1 X=0] PLA    */
    PULL(0xab16,1,false);
    set_a(s,B.read_value);
    goto source_ab17_m1x0;
source_ab17_m1x0: /* $80:AB17 [M=1 X=0] PLP    */
    PULL(0xab17,1,false);
    status_set(s,B.read_value);
    goto source_ab18_m0x0;
source_ab18_m0x0: /* $80:AB18 [M=0 X=0] REP   #$30 */
    MODE(0xab18);
    status_set(s,P & 0xcfu);
    goto source_ab1a_m0x0;
source_ab1a_m0x0: /* $80:AB1A [M=0 X=0] LDY   #$00 */
    IMM(0xab1a,3);
    set_y(s,0x0u);
    goto source_ab1d_m0x0;
source_ab1d_m0x0: /* $80:AB1D [M=0 X=0] LDA   #$BBAA */
    IMM(0xab1d,3);
    set_a(s,0xbbaau);
    goto source_ab20_m0x0;
source_ab20_m0x0: /* $80:AB20 [M=0 X=0] CMP   $2140 */
    READ(0xab20,3,DB(0x2140u),2,false);
    compare(s,A,B.read_value,2);
    goto source_ab23_m0x0;
source_ab23_m0x0: /* $80:AB23 [M=0 X=0] BNE   $AB20 */
    BRANCH(0xab23,(!(P&2))!=0);
    if(B.branch) { goto source_ab20_m0x0; }
    goto source_ab25_m0x0;
source_ab25_m0x0: /* $80:AB25 [M=0 X=0] SEP   #$20 */
    MODE(0xab25);
    status_set(s,P | 0x20u);
    goto source_ab27_m1x0;
source_ab27_m1x0: /* $80:AB27 [M=1 X=0] LDA   #$CC */
    IMM(0xab27,2);
    set_a(s,0xccu);
    goto source_ab29_m1x0;
source_ab29_m1x0: /* $80:AB29 [M=1 X=0] BRA   $AB51 */
    BRANCH(0xab29,(true)!=0);
    if(B.branch) { goto source_ab51_m1x0; }
    goto source_ab2b_m1x0;
source_ab2b_m1x0: /* $80:AB2B [M=1 X=0] LDA   [$0C],Y */
    LONG_READ(0xab2b,0xc,1);
    set_a(s,B.read_value);
    goto source_ab2d_m1x0;
source_ab2d_m1x0: /* $80:AB2D [M=1 X=0] INY    */
    IMP(0xab2d);
    set_y(s,Y+1u);
    goto source_ab2e_m1x0;
source_ab2e_m1x0: /* $80:AB2E [M=1 X=0] XBA    */
    SUSPEND(implied(s,0xab2e,1,2));
    A=(uint16_t)((A<<8)|(A>>8)); nz(s,A,1);
    goto source_ab2f_m1x0;
source_ab2f_m1x0: /* $80:AB2F [M=1 X=0] LDA   #$00 */
    IMM(0xab2f,2);
    set_a(s,0x0u);
    goto source_ab31_m1x0;
source_ab31_m1x0: /* $80:AB31 [M=1 X=0] BRA   $AB3E */
    BRANCH(0xab31,(true)!=0);
    if(B.branch) { goto source_ab3e_m1x0; }
    goto source_ab33_m1x0;
source_ab33_m1x0: /* $80:AB33 [M=1 X=0] XBA    */
    SUSPEND(implied(s,0xab33,1,2));
    A=(uint16_t)((A<<8)|(A>>8)); nz(s,A,1);
    goto source_ab34_m1x0;
source_ab34_m1x0: /* $80:AB34 [M=1 X=0] LDA   [$0C],Y */
    LONG_READ(0xab34,0xc,1);
    set_a(s,B.read_value);
    goto source_ab36_m1x0;
source_ab36_m1x0: /* $80:AB36 [M=1 X=0] INY    */
    IMP(0xab36);
    set_y(s,Y+1u);
    goto source_ab37_m1x0;
source_ab37_m1x0: /* $80:AB37 [M=1 X=0] XBA    */
    SUSPEND(implied(s,0xab37,1,2));
    A=(uint16_t)((A<<8)|(A>>8)); nz(s,A,1);
    goto source_ab38_m1x0;
source_ab38_m1x0: /* $80:AB38 [M=1 X=0] CMP   $2140 */
    READ(0xab38,3,DB(0x2140u),1,false);
    compare(s,A,B.read_value,1);
    goto source_ab3b_m1x0;
source_ab3b_m1x0: /* $80:AB3B [M=1 X=0] BNE   $AB38 */
    BRANCH(0xab3b,(!(P&2))!=0);
    if(B.branch) { goto source_ab38_m1x0; }
    goto source_ab3d_m1x0;
source_ab3d_m1x0: /* $80:AB3D [M=1 X=0] INC   A */
    IMP(0xab3d);
    accumulator_change(s,SOUND_INC);
    goto source_ab3e_m1x0;
source_ab3e_m1x0: /* $80:AB3E [M=1 X=0] REP   #$20 */
    MODE(0xab3e);
    status_set(s,P & 0xdfu);
    goto source_ab40_m0x0;
source_ab40_m0x0: /* $80:AB40 [M=0 X=0] STA   $2140 */
    WRITE(0xab40,3,DB(0x2140u),2,false,A);
    goto source_ab43_m0x0;
source_ab43_m0x0: /* $80:AB43 [M=0 X=0] SEP   #$20 */
    MODE(0xab43);
    status_set(s,P | 0x20u);
    goto source_ab45_m1x0;
source_ab45_m1x0: /* $80:AB45 [M=1 X=0] DEX    */
    IMP(0xab45);
    set_x(s,X-1u);
    goto source_ab46_m1x0;
source_ab46_m1x0: /* $80:AB46 [M=1 X=0] BNE   $AB33 */
    BRANCH(0xab46,(!(P&2))!=0);
    if(B.branch) { goto source_ab33_m1x0; }
    goto source_ab48_m1x0;
source_ab48_m1x0: /* $80:AB48 [M=1 X=0] CMP   $2140 */
    READ(0xab48,3,DB(0x2140u),1,false);
    compare(s,A,B.read_value,1);
    goto source_ab4b_m1x0;
source_ab4b_m1x0: /* $80:AB4B [M=1 X=0] BNE   $AB48 */
    BRANCH(0xab4b,(!(P&2))!=0);
    if(B.branch) { goto source_ab48_m1x0; }
    goto source_ab4d_m1x0;
source_ab4d_m1x0: /* $80:AB4D [M=1 X=0] ADC   #$03 */
    IMM(0xab4d,2);
    add(s,0x3u);
    goto source_ab4f_m1x0;
source_ab4f_m1x0: /* $80:AB4F [M=1 X=0] BEQ   $AB4D */
    BRANCH(0xab4f,(P&2)!=0);
    if(B.branch) { goto source_ab4d_m1x0; }
    goto source_ab51_m1x0;
source_ab51_m1x0: /* $80:AB51 [M=1 X=0] PHA    */
    PUSH(0xab51,1,A,1);
    goto source_ab52_m1x0;
source_ab52_m1x0: /* $80:AB52 [M=1 X=0] REP   #$20 */
    MODE(0xab52);
    status_set(s,P & 0xdfu);
    goto source_ab54_m0x0;
source_ab54_m0x0: /* $80:AB54 [M=0 X=0] LDA   [$0C],Y */
    LONG_READ(0xab54,0xc,2);
    set_a(s,B.read_value);
    goto source_ab56_m0x0;
source_ab56_m0x0: /* $80:AB56 [M=0 X=0] INY    */
    IMP(0xab56);
    set_y(s,Y+1u);
    goto source_ab57_m0x0;
source_ab57_m0x0: /* $80:AB57 [M=0 X=0] INY    */
    IMP(0xab57);
    set_y(s,Y+1u);
    goto source_ab58_m0x0;
source_ab58_m0x0: /* $80:AB58 [M=0 X=0] TAX    */
    IMP(0xab58);
    set_x(s,A);
    goto source_ab59_m0x0;
source_ab59_m0x0: /* $80:AB59 [M=0 X=0] LDA   [$0C],Y */
    LONG_READ(0xab59,0xc,2);
    set_a(s,B.read_value);
    goto source_ab5b_m0x0;
source_ab5b_m0x0: /* $80:AB5B [M=0 X=0] INY    */
    IMP(0xab5b);
    set_y(s,Y+1u);
    goto source_ab5c_m0x0;
source_ab5c_m0x0: /* $80:AB5C [M=0 X=0] INY    */
    IMP(0xab5c);
    set_y(s,Y+1u);
    goto source_ab5d_m0x0;
source_ab5d_m0x0: /* $80:AB5D [M=0 X=0] STA   $2142 */
    WRITE(0xab5d,3,DB(0x2142u),2,false,A);
    goto source_ab60_m0x0;
source_ab60_m0x0: /* $80:AB60 [M=0 X=0] SEP   #$20 */
    MODE(0xab60);
    status_set(s,P | 0x20u);
    goto source_ab62_m1x0;
source_ab62_m1x0: /* $80:AB62 [M=1 X=0] CPX   #$01 */
    IMM(0xab62,3);
    compare(s,X,0x1u,2);
    goto source_ab65_m1x0;
source_ab65_m1x0: /* $80:AB65 [M=1 X=0] LDA   #$00 */
    IMM(0xab65,2);
    set_a(s,0x0u);
    goto source_ab67_m1x0;
source_ab67_m1x0: /* $80:AB67 [M=1 X=0] ROL   A */
    IMP(0xab67);
    accumulator_change(s,SOUND_ROL);
    goto source_ab68_m1x0;
source_ab68_m1x0: /* $80:AB68 [M=1 X=0] STA   $2141 */
    WRITE(0xab68,3,DB(0x2141u),1,false,A);
    goto source_ab6b_m1x0;
source_ab6b_m1x0: /* $80:AB6B [M=1 X=0] ADC   #$7F */
    IMM(0xab6b,2);
    add(s,0x7fu);
    goto source_ab6d_m1x0;
source_ab6d_m1x0: /* $80:AB6D [M=1 X=0] PLA    */
    PULL(0xab6d,1,false);
    set_a(s,B.read_value);
    goto source_ab6e_m1x0;
source_ab6e_m1x0: /* $80:AB6E [M=1 X=0] STA   $2140 */
    WRITE(0xab6e,3,DB(0x2140u),1,false,A);
    goto source_ab71_m1x0;
source_ab71_m1x0: /* $80:AB71 [M=1 X=0] CMP   $2140 */
    READ(0xab71,3,DB(0x2140u),1,false);
    compare(s,A,B.read_value,1);
    goto source_ab74_m1x0;
source_ab74_m1x0: /* $80:AB74 [M=1 X=0] BNE   $AB71 */
    BRANCH(0xab74,(!(P&2))!=0);
    if(B.branch) { goto source_ab71_m1x0; }
    goto source_ab76_m1x0;
source_ab76_m1x0: /* $80:AB76 [M=1 X=0] BVS   $AB2B */
    BRANCH(0xab76,(P&64)!=0);
    if(B.branch) { goto source_ab2b_m1x0; }
    goto source_ab78_m1x0;
source_ab78_m1x0: /* $80:AB78 [M=1 X=0] STZ   $2140 */
    WRITE(0xab78,3,DB(0x2140u),1,false,0);
    goto source_ab7b_m1x0;
source_ab7b_m1x0: /* $80:AB7B [M=1 X=0] REP   #$30 */
    MODE(0xab7b);
    status_set(s,P & 0xcfu);
    goto source_ab7d_m0x0;
source_ab7d_m0x0: /* $80:AB7D [M=0 X=0] RTL    */
    PULL(0xab7d,3,false);
    if(s->read_bank!=0x80)return boundary(s,0x80ab7du);
    switch((uint16_t)(B.read_value+1u)){
    case 0x8043: goto source_8043_m0x0;
    default:return boundary(s,0x80ab7du);}
default:return boundary(s,0); }}
