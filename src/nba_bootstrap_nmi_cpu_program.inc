/* Generated canonical reset/upload/first-fill/reset-tables/early-sound/early-NMI source. No opcode dispatch. */
static bool advance(NbaBootstrapCpu *s) { switch(B.resume) { case 0: goto source_800d_m1x1; case 0xffffffffu: goto source_8156_m1x1;
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
    goto source_80bc_m0x0;
source_80bc_m0x0: /* $80:80BC [M=0 X=0] JSL   $808A57 */
    SUSPEND(jsl_call(s,0x80bc));
    B.registers.stack_pointer-=3u;
    goto source_8a57_m0x0;
source_80c0_m0x0: /* $80:80C0 [M=0 X=0] SEP   #$20 */
    MODE(0x80c0);
    status_set(s,P | 0x20u);
    goto source_80c2_m1x0;
source_80c2_m1x0: /* $80:80C2 [M=1 X=0] LDX   #$1FFF */
    IMM(0x80c2,3);
    set_x(s,0x1fffu);
    goto source_80c5_m1x0;
source_80c5_m1x0: /* $80:80C5 [M=1 X=0] STZ   $00,X */
    WRITE(0x80c5,2,(uint16_t)(0x0u+X),1,true,0);
    goto source_80c7_m1x0;
source_80c7_m1x0: /* $80:80C7 [M=1 X=0] DEX    */
    IMP(0x80c7);
    set_x(s,X-1u);
    goto source_80c8_m1x0;
source_80c8_m1x0: /* $80:80C8 [M=1 X=0] BPL   $80C5 */
    BRANCH(0x80c8,(!(P&128))!=0);
    if(B.branch) { goto source_80c5_m1x0; }
    goto source_80ca_m1x0;
source_80ca_m1x0: /* $80:80CA [M=1 X=0] LDX   #$05 */
    IMM(0x80ca,3);
    set_x(s,0x5u);
    goto source_80cd_m1x0;
source_80cd_m1x0: /* $80:80CD [M=1 X=0] LDA   $8006,X */
    READ(0x80cd,3,DB(0x8006u+X),1,index_idle(s,0x8006,X));
    set_a(s,B.read_value);
    goto source_80d0_m1x0;
source_80d0_m1x0: /* $80:80D0 [M=1 X=0] STA   $059E,X */
    WRITE(0x80d0,3,DB(0x59eu+X),1,true,A);
    goto source_80d3_m1x0;
source_80d3_m1x0: /* $80:80D3 [M=1 X=0] STA   $0006,X */
    WRITE(0x80d3,3,DB(0x6u+X),1,true,A);
    goto source_80d6_m1x0;
source_80d6_m1x0: /* $80:80D6 [M=1 X=0] STA   $7E8FEE,X */
    WRITE(0x80d6,4,((0x7e8feeu+X)&0xffffffu),1,false,A);
    goto source_80da_m1x0;
source_80da_m1x0: /* $80:80DA [M=1 X=0] DEX    */
    IMP(0x80da);
    set_x(s,X-1u);
    goto source_80db_m1x0;
source_80db_m1x0: /* $80:80DB [M=1 X=0] BPL   $80CD */
    BRANCH(0x80db,(!(P&128))!=0);
    if(B.branch) { goto source_80cd_m1x0; }
    goto source_80dd_m1x0;
source_80dd_m1x0: /* $80:80DD [M=1 X=0] LDX   #$00 */
    IMM(0x80dd,3);
    set_x(s,0x0u);
    goto source_80e0_m1x0;
source_80e0_m1x0: /* $80:80E0 [M=1 X=0] BEQ   $80ED */
    BRANCH(0x80e0,(P&2)!=0);
    if(B.branch) { goto source_80ed_m1x0; }
    goto source_80e2_m1x0;
source_80e2_m1x0: /* $80:80E2 [M=1 X=0] LDA   $80FEFF,X */
    READ(0x80e2,4,((0x80feffu+X)&0xffffffu),1,false);
    set_a(s,B.read_value);
    goto source_80e6_m1x0;
source_80e6_m1x0: /* $80:80E6 [M=1 X=0] STA   $7E8FF3,X */
    WRITE(0x80e6,4,((0x7e8ff3u+X)&0xffffffu),1,false,A);
    goto source_80ea_m1x0;
source_80ea_m1x0: /* $80:80EA [M=1 X=0] DEX    */
    IMP(0x80ea);
    set_x(s,X-1u);
    goto source_80eb_m1x0;
source_80eb_m1x0: /* $80:80EB [M=1 X=0] BNE   $80E2 */
    BRANCH(0x80eb,(!(P&2))!=0);
    if(B.branch) { goto source_80e2_m1x0; }
    goto source_80ed_m1x0;
source_80ed_m1x0: /* $80:80ED [M=1 X=0] LDX   #$00 */
    IMM(0x80ed,3);
    set_x(s,0x0u);
    goto source_80f0_m1x0;
source_80f0_m1x0: /* $80:80F0 [M=1 X=0] LDA   $971F,X */
    READ(0x80f0,3,DB(0x971fu+X),1,index_idle(s,0x971f,X));
    set_a(s,B.read_value);
    goto source_80f3_m1x0;
source_80f3_m1x0: /* $80:80F3 [M=1 X=0] STA   $7E3425,X */
    WRITE(0x80f3,4,((0x7e3425u+X)&0xffffffu),1,false,A);
    goto source_80f7_m1x0;
source_80f7_m1x0: /* $80:80F7 [M=1 X=0] INX    */
    IMP(0x80f7);
    set_x(s,X+1u);
    goto source_80f8_m1x0;
source_80f8_m1x0: /* $80:80F8 [M=1 X=0] CPX   #$10 */
    IMM(0x80f8,3);
    compare(s,X,0x10u,2);
    goto source_80fb_m1x0;
source_80fb_m1x0: /* $80:80FB [M=1 X=0] BCC   $80F0 */
    BRANCH(0x80fb,(!(P&1))!=0);
    if(B.branch) { goto source_80f0_m1x0; }
    goto source_80fd_m1x0;
source_80fd_m1x0: /* $80:80FD [M=1 X=0] JSL   $80DA72 */
    SUSPEND(jsl_call(s,0x80fd));
    B.registers.stack_pointer-=3u;
    goto source_da72_m1x0;
source_8101_m0x0: /* $80:8101 [M=0 X=0] SEP   #$20 */
    MODE(0x8101);
    status_set(s,P | 0x20u);
    goto source_8103_m1x0;
source_8103_m1x0: /* $80:8103 [M=1 X=0] REP   #$10 */
    MODE(0x8103);
    status_set(s,P & 0xefu);
    goto source_8105_m1x0;
source_8105_m1x0: /* $80:8105 [M=1 X=0] LDA   #$8F */
    IMM(0x8105,2);
    set_a(s,0x8fu);
    goto source_8107_m1x0;
source_8107_m1x0: /* $80:8107 [M=1 X=0] STA   $0562 */
    WRITE(0x8107,3,DB(0x562u),1,false,A);
    goto source_810a_m1x0;
source_810a_m1x0: /* $80:810A [M=1 X=0] LDX   #$39 */
    IMM(0x810a,3);
    set_x(s,0x39u);
    goto source_810d_m1x0;
source_810d_m1x0: /* $80:810D [M=1 X=0] STZ   $0564,X */
    WRITE(0x810d,3,DB(0x564u+X),1,true,0);
    goto source_8110_m1x0;
source_8110_m1x0: /* $80:8110 [M=1 X=0] DEX    */
    IMP(0x8110);
    set_x(s,X-1u);
    goto source_8111_m1x0;
source_8111_m1x0: /* $80:8111 [M=1 X=0] BPL   $810D */
    BRANCH(0x8111,(!(P&128))!=0);
    if(B.branch) { goto source_810d_m1x0; }
    goto source_8113_m1x0;
source_8113_m1x0: /* $80:8113 [M=1 X=0] REP   #$20 */
    MODE(0x8113);
    status_set(s,P & 0xdfu);
    goto source_8115_m0x0;
source_8115_m0x0: /* $80:8115 [M=0 X=0] LDA   #$8080 */
    IMM(0x8115,3);
    set_a(s,0x8080u);
    goto source_8118_m0x0;
source_8118_m0x0: /* $80:8118 [M=0 X=0] STA   $05C3 */
    WRITE(0x8118,3,DB(0x5c3u),2,false,A);
    goto source_811b_m0x0;
source_811b_m0x0: /* $80:811B [M=0 X=0] STA   $05C6 */
    WRITE(0x811b,3,DB(0x5c6u),2,false,A);
    goto source_811e_m0x0;
source_811e_m0x0: /* $80:811E [M=0 X=0] STA   $05C9 */
    WRITE(0x811e,3,DB(0x5c9u),2,false,A);
    goto source_8121_m0x0;
source_8121_m0x0: /* $80:8121 [M=0 X=0] LDA   #$800C */
    IMM(0x8121,3);
    set_a(s,0x800cu);
    goto source_8124_m0x0;
source_8124_m0x0: /* $80:8124 [M=0 X=0] STA   $05C2 */
    WRITE(0x8124,3,DB(0x5c2u),2,false,A);
    goto source_8127_m0x0;
source_8127_m0x0: /* $80:8127 [M=0 X=0] STA   $05C5 */
    WRITE(0x8127,3,DB(0x5c5u),2,false,A);
    goto source_812a_m0x0;
source_812a_m0x0: /* $80:812A [M=0 X=0] STA   $05C8 */
    WRITE(0x812a,3,DB(0x5c8u),2,false,A);
    goto source_812d_m0x0;
source_812d_m0x0: /* $80:812D [M=0 X=0] STZ   $05CB */
    WRITE(0x812d,3,DB(0x5cbu),2,false,0);
    goto source_8130_m0x0;
source_8130_m0x0: /* $80:8130 [M=0 X=0] STZ   $31 */
    WRITE(0x8130,2,0x31u,2,false,0);
    goto source_8132_m0x0;
source_8132_m0x0: /* $80:8132 [M=0 X=0] STZ   $33 */
    WRITE(0x8132,2,0x33u,2,false,0);
    goto source_8134_m0x0;
source_8134_m0x0: /* $80:8134 [M=0 X=0] STZ   $35 */
    WRITE(0x8134,2,0x35u,2,false,0);
    goto source_8136_m0x0;
source_8136_m0x0: /* $80:8136 [M=0 X=0] STZ   $37 */
    WRITE(0x8136,2,0x37u,2,false,0);
    goto source_8138_m0x0;
source_8138_m0x0: /* $80:8138 [M=0 X=0] LDA   #$00 */
    IMM(0x8138,3);
    set_a(s,0x0u);
    goto source_813b_m0x0;
source_813b_m0x0: /* $80:813B [M=0 X=0] TAX    */
    IMP(0x813b);
    set_x(s,A);
    goto source_813c_m0x0;
source_813c_m0x0: /* $80:813C [M=0 X=0] TAY    */
    IMP(0x813c);
    set_y(s,A);
    goto source_813d_m0x0;
source_813d_m0x0: /* $80:813D [M=0 X=0] JSL   $80AB7E */
    SUSPEND(jsl_call(s,0x813d));
    B.registers.stack_pointer-=3u;
    goto source_ab7e_m0x0;
source_8141_m0x0: /* $80:8141 [M=0 X=0] SEP   #$20 */
    MODE(0x8141);
    status_set(s,P | 0x20u);
    goto source_8143_m1x0;
source_8143_m1x0: /* $80:8143 [M=1 X=0] LDA   #$81 */
    IMM(0x8143,2);
    set_a(s,0x81u);
    goto source_8145_m1x0;
source_8145_m1x0: /* $80:8145 [M=1 X=0] STA   $4200 */
    WRITE(0x8145,3,DB(0x4200u),1,false,A);
    goto source_8148_m1x0;
source_8148_m1x0: /* $80:8148 [M=1 X=0] REP   #$30 */
    MODE(0x8148);
    status_set(s,P & 0xcfu);
    goto source_814a_m0x0;
source_814a_m0x0: /* $80:814A [M=0 X=0] JSL   $809B73 */
    SUSPEND(jsl_call(s,0x814a));
    B.registers.stack_pointer-=3u;
    goto source_9b73_m0x0;
source_8156_m1x1: /* $80:8156 [M=1 X=1] JMP   $80815A */
    IMM(0x8156,4);
    s->program_bank=0x80;
    goto source_815a_m1x1;
source_815a_m1x1: /* $80:815A [M=1 X=1] REP   #$30 */
    MODE(0x815a);
    status_set(s,P & 0xcfu);
    goto source_815c_m0x0;
source_815c_m0x0: /* $80:815C [M=0 X=0] PHD    */
    PUSH(0x815c,1,0,2);
    goto source_815d_m0x0;
source_815d_m0x0: /* $80:815D [M=0 X=0] PHB    */
    PUSH(0x815d,1,B.registers.data_bank,1);
    goto source_815e_m0x0;
source_815e_m0x0: /* $80:815E [M=0 X=0] PHK    */
    PUSH(0x815e,1,s->program_bank,1);
    goto source_815f_m0x0;
source_815f_m0x0: /* $80:815F [M=0 X=0] PLB    */
    PULL(0x815f,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_8160_m0x0;
source_8160_m0x0: /* $80:8160 [M=0 X=0] PHA    */
    PUSH(0x8160,1,A,2);
    goto source_8161_m0x0;
source_8161_m0x0: /* $80:8161 [M=0 X=0] CLI    */
    IMP(0x8161);
    P&=0xfbu;
    goto source_8162_m0x0;
source_8162_m0x0: /* $80:8162 [M=0 X=0] LDA   #$00 */
    IMM(0x8162,3);
    set_a(s,0x0u);
    goto source_8165_m0x0;
source_8165_m0x0: /* $80:8165 [M=0 X=0] TCD    */
    IMP(0x8165);
    if(A) return boundary(s,0x00801bu);
    nz(s,A,2);
    goto source_8166_m0x0;
source_8166_m0x0: /* $80:8166 [M=0 X=0] LDA   $4210 */
    READ(0x8166,3,DB(0x4210u),2,false);
    set_a(s,B.read_value);
    goto source_8169_m0x0;
source_8169_m0x0: /* $80:8169 [M=0 X=0] LDA   $05CB */
    READ(0x8169,3,DB(0x5cbu),2,false);
    set_a(s,B.read_value);
    goto source_816c_m0x0;
source_816c_m0x0: /* $80:816C [M=0 X=0] BEQ   $8178 */
    BRANCH(0x816c,(P&2)!=0);
    if(B.branch) { goto source_8178_m0x0; }
    return boundary(s,0x80816eu);
source_8178_m0x0: /* $80:8178 [M=0 X=0] PHX    */
    PUSH(0x8178,1,X,2);
    goto source_8179_m0x0;
source_8179_m0x0: /* $80:8179 [M=0 X=0] PHY    */
    PUSH(0x8179,1,Y,2);
    goto source_817a_m0x0;
source_817a_m0x0: /* $80:817A [M=0 X=0] LDA   #$FFFF */
    IMM(0x817a,3);
    set_a(s,0xffffu);
    goto source_817d_m0x0;
source_817d_m0x0: /* $80:817D [M=0 X=0] STA   $05CB */
    WRITE(0x817d,3,DB(0x5cbu),2,false,A);
    goto source_8180_m0x0;
source_8180_m0x0: /* $80:8180 [M=0 X=0] SEP   #$20 */
    MODE(0x8180);
    status_set(s,P | 0x20u);
    goto source_8182_m1x0;
source_8182_m1x0: /* $80:8182 [M=1 X=0] LDA   #$80 */
    IMM(0x8182,2);
    set_a(s,0x80u);
    return boundary(s,0x808184u);
source_8a57_m0x0: /* $80:8A57 [M=0 X=0] PHP    */
    PUSH(0x8a57,1,P,1);
    goto source_8a58_m0x0;
source_8a58_m0x0: /* $80:8A58 [M=0 X=0] SEP   #$20 */
    MODE(0x8a58);
    status_set(s,P | 0x20u);
    goto source_8a5a_m1x0;
source_8a5a_m1x0: /* $80:8A5A [M=1 X=0] PHA    */
    PUSH(0x8a5a,1,A,1);
    goto source_8a5b_m1x0;
source_8a5b_m1x0: /* $80:8A5B [M=1 X=0] PHB    */
    PUSH(0x8a5b,1,B.registers.data_bank,1);
    goto source_8a5c_m1x0;
source_8a5c_m1x0: /* $80:8A5C [M=1 X=0] PLA    */
    PULL(0x8a5c,1,false);
    set_a(s,B.read_value);
    goto source_8a5d_m1x0;
source_8a5d_m1x0: /* $80:8A5D [M=1 X=0] CMP   #$7E */
    IMM(0x8a5d,2);
    compare(s,A,0x7eu,1);
    goto source_8a5f_m1x0;
source_8a5f_m1x0: /* $80:8A5F [M=1 X=0] BEQ   $8A65 */
    BRANCH(0x8a5f,(P&2)!=0);
    if(B.branch) { goto source_8a65_m1x0; }
    goto source_8a61_m1x0;
source_8a61_m1x0: /* $80:8A61 [M=1 X=0] CMP   #$7F */
    IMM(0x8a61,2);
    compare(s,A,0x7fu,1);
    goto source_8a63_m1x0;
source_8a63_m1x0: /* $80:8A63 [M=1 X=0] BNE   $8A67 */
    BRANCH(0x8a63,(!(P&2))!=0);
    if(B.branch) { goto source_8a67_m1x0; }
    goto source_8a65_m1x0;
source_8a65_m1x0: /* $80:8A65 [M=1 X=0] PHK    */
    PUSH(0x8a65,1,s->program_bank,1);
    goto source_8a66_m1x0;
source_8a66_m1x0: /* $80:8A66 [M=1 X=0] PLB    */
    PULL(0x8a66,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_8a67_m1x0;
source_8a67_m1x0: /* $80:8A67 [M=1 X=0] PLA    */
    PULL(0x8a67,1,false);
    set_a(s,B.read_value);
    goto source_8a68_m1x0;
source_8a68_m1x0: /* $80:8A68 [M=1 X=0] PLP    */
    PULL(0x8a68,1,false);
    status_set(s,B.read_value);
    goto source_8a69_m0x0;
source_8a69_m0x0: /* $80:8A69 [M=0 X=0] STA   $16 */
    WRITE(0x8a69,2,0x16u,2,false,A);
    goto source_8a6b_m0x0;
source_8a6b_m0x0: /* $80:8A6B [M=0 X=0] LDA   $0561 */
    READ(0x8a6b,3,DB(0x561u),2,false);
    set_a(s,B.read_value);
    goto source_8a6e_m0x0;
source_8a6e_m0x0: /* $80:8A6E [M=0 X=0] BPL   $8A93 */
    BRANCH(0x8a6e,(!(P&128))!=0);
    if(B.branch) { return boundary(s,0x808a93u); }
    goto source_8a70_m0x0;
source_8a70_m0x0: /* $80:8A70 [M=0 X=0] STX   $4315 */
    WRITE(0x8a70,3,DB(0x4315u),2,false,X);
    goto source_8a73_m0x0;
source_8a73_m0x0: /* $80:8A73 [M=0 X=0] STY   $2116 */
    WRITE(0x8a73,3,DB(0x2116u),2,false,Y);
    goto source_8a76_m0x0;
source_8a76_m0x0: /* $80:8A76 [M=0 X=0] LDY   #$16 */
    IMM(0x8a76,3);
    set_y(s,0x16u);
    goto source_8a79_m0x0;
source_8a79_m0x0: /* $80:8A79 [M=0 X=0] STY   $4312 */
    WRITE(0x8a79,3,DB(0x4312u),2,false,Y);
    goto source_8a7c_m0x0;
source_8a7c_m0x0: /* $80:8A7C [M=0 X=0] SEP   #$20 */
    MODE(0x8a7c);
    status_set(s,P | 0x20u);
    goto source_8a7e_m1x0;
source_8a7e_m1x0: /* $80:8A7E [M=1 X=0] LDA   #$09 */
    IMM(0x8a7e,2);
    set_a(s,0x9u);
    goto source_8a80_m1x0;
source_8a80_m1x0: /* $80:8A80 [M=1 X=0] STA   $4310 */
    WRITE(0x8a80,3,DB(0x4310u),1,false,A);
    goto source_8a83_m1x0;
source_8a83_m1x0: /* $80:8A83 [M=1 X=0] LDA   #$18 */
    IMM(0x8a83,2);
    set_a(s,0x18u);
    goto source_8a85_m1x0;
source_8a85_m1x0: /* $80:8A85 [M=1 X=0] STA   $4311 */
    WRITE(0x8a85,3,DB(0x4311u),1,false,A);
    goto source_8a88_m1x0;
source_8a88_m1x0: /* $80:8A88 [M=1 X=0] STZ   $4314 */
    WRITE(0x8a88,3,DB(0x4314u),1,false,0);
    goto source_8a8b_m1x0;
source_8a8b_m1x0: /* $80:8A8B [M=1 X=0] LDA   #$02 */
    IMM(0x8a8b,2);
    set_a(s,0x2u);
    goto source_8a8d_m1x0;
source_8a8d_m1x0: /* $80:8A8D [M=1 X=0] STA   $420B */
    WRITE(0x8a8d,3,DB(0x420bu),1,false,A);
    goto source_8a90_m1x0;
source_8a90_m1x0: /* $80:8A90 [M=1 X=0] REP   #$20 */
    MODE(0x8a90);
    status_set(s,P & 0xdfu);
    goto source_8a92_m0x0;
source_8a92_m0x0: /* $80:8A92 [M=0 X=0] RTL    */
    PULL(0x8a92,3,false);
    if(s->read_bank!=0x80)return boundary(s,0x80ab7du);
    switch((uint16_t)(B.read_value+1u)){
    case 0x8043: goto source_8043_m0x0;
    case 0x80c0: goto source_80c0_m0x0;
    case 0x8101: goto source_8101_m0x0;
    case 0x8141: goto source_8141_m0x0;
    case 0x814e: return boundary(s,0x80814eu);
    case 0x9bb6: return boundary(s,0x809bb6u);
    case 0x9bf4: return boundary(s,0x809bf4u);
    case 0xac07: goto source_ac07_m0x0;
    case 0xac0b: goto source_ac0b_m0x0;
    default:return boundary(s,0x80ab7du);}
source_9b73_m0x0: /* $80:9B73 [M=0 X=0] PHP    */
    PUSH(0x9b73,1,P,1);
    goto source_9b74_m0x0;
source_9b74_m0x0: /* $80:9B74 [M=0 X=0] SEP   #$20 */
    MODE(0x9b74);
    status_set(s,P | 0x20u);
    goto source_9b76_m1x0;
source_9b76_m1x0: /* $80:9B76 [M=1 X=0] PHA    */
    PUSH(0x9b76,1,A,1);
    goto source_9b77_m1x0;
source_9b77_m1x0: /* $80:9B77 [M=1 X=0] PHB    */
    PUSH(0x9b77,1,B.registers.data_bank,1);
    goto source_9b78_m1x0;
source_9b78_m1x0: /* $80:9B78 [M=1 X=0] PLA    */
    PULL(0x9b78,1,false);
    set_a(s,B.read_value);
    goto source_9b79_m1x0;
source_9b79_m1x0: /* $80:9B79 [M=1 X=0] CMP   #$7E */
    IMM(0x9b79,2);
    compare(s,A,0x7eu,1);
    goto source_9b7b_m1x0;
source_9b7b_m1x0: /* $80:9B7B [M=1 X=0] BEQ   $9B81 */
    BRANCH(0x9b7b,(P&2)!=0);
    if(B.branch) { goto source_9b81_m1x0; }
    goto source_9b7d_m1x0;
source_9b7d_m1x0: /* $80:9B7D [M=1 X=0] CMP   #$7F */
    IMM(0x9b7d,2);
    compare(s,A,0x7fu,1);
    goto source_9b7f_m1x0;
source_9b7f_m1x0: /* $80:9B7F [M=1 X=0] BNE   $9B83 */
    BRANCH(0x9b7f,(!(P&2))!=0);
    if(B.branch) { goto source_9b83_m1x0; }
    goto source_9b81_m1x0;
source_9b81_m1x0: /* $80:9B81 [M=1 X=0] PHK    */
    PUSH(0x9b81,1,s->program_bank,1);
    goto source_9b82_m1x0;
source_9b82_m1x0: /* $80:9B82 [M=1 X=0] PLB    */
    PULL(0x9b82,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_9b83_m1x0;
source_9b83_m1x0: /* $80:9B83 [M=1 X=0] PLA    */
    PULL(0x9b83,1,false);
    set_a(s,B.read_value);
    goto source_9b84_m1x0;
source_9b84_m1x0: /* $80:9B84 [M=1 X=0] PLP    */
    PULL(0x9b84,1,false);
    status_set(s,B.read_value);
    goto source_9b85_m0x0;
source_9b85_m0x0: /* $80:9B85 [M=0 X=0] PHP    */
    PUSH(0x9b85,1,P,1);
    goto source_9b86_m0x0;
source_9b86_m0x0: /* $80:9B86 [M=0 X=0] SEP   #$20 */
    MODE(0x9b86);
    status_set(s,P | 0x20u);
    goto source_9b88_m1x0;
source_9b88_m1x0: /* $80:9B88 [M=1 X=0] REP   #$10 */
    MODE(0x9b88);
    status_set(s,P & 0xefu);
    goto source_9b8a_m1x0;
source_9b8a_m1x0: /* $80:9B8A [M=1 X=0] DEC   $53 */
    SUSPEND(decrement_guard(s,0x9b8a));
    nz(s,(uint8_t)(B.read_value-1u),1);
    goto source_9b8c_m1x0;
source_9b8c_m1x0: /* $80:9B8C [M=1 X=0] LDX   $33 */
    READ(0x9b8c,2,0x33u,2,false);
    set_x(s,B.read_value);
    goto source_9b8e_m1x0;
source_9b8e_m1x0: /* $80:9B8E [M=1 X=0] STX   $5A */
    WRITE(0x9b8e,2,0x5au,2,false,X);
    goto source_9b90_m1x0;
source_9b90_m1x0: /* $80:9B90 [M=1 X=0] LDX   #$01C3 */
    IMM(0x9b90,3);
    set_x(s,0x1c3u);
    goto source_9b93_m1x0;
source_9b93_m1x0: /* $80:9B93 [M=1 X=0] STZ   $062A,X */
    WRITE(0x9b93,3,DB(0x62au+X),1,true,0);
    goto source_9b96_m1x0;
source_9b96_m1x0: /* $80:9B96 [M=1 X=0] DEX    */
    IMP(0x9b96);
    set_x(s,X-1u);
    goto source_9b97_m1x0;
source_9b97_m1x0: /* $80:9B97 [M=1 X=0] BPL   $9B93 */
    BRANCH(0x9b97,(!(P&128))!=0);
    if(B.branch) { goto source_9b93_m1x0; }
    goto source_9b99_m1x0;
source_9b99_m1x0: /* $80:9B99 [M=1 X=0] SEP   #$10 */
    MODE(0x9b99);
    status_set(s,P | 0x10u);
    goto source_9b9b_m1x1;
source_9b9b_m1x1: /* $80:9B9B [M=1 X=1] LDA   #$FF */
    IMM(0x9b9b,2);
    set_a(s,0xffu);
    goto source_9b9d_m1x1;
source_9b9d_m1x1: /* $80:9B9D [M=1 X=1] STA   $07CD */
    WRITE(0x9b9d,3,DB(0x7cdu),1,false,A);
    goto source_9ba0_m1x1;
source_9ba0_m1x1: /* $80:9BA0 [M=1 X=1] STA   $07CE */
    WRITE(0x9ba0,3,DB(0x7ceu),1,false,A);
    goto source_9ba3_m1x1;
source_9ba3_m1x1: /* $80:9BA3 [M=1 X=1] STA   $07CF */
    WRITE(0x9ba3,3,DB(0x7cfu),1,false,A);
    goto source_9ba6_m1x1;
source_9ba6_m1x1: /* $80:9BA6 [M=1 X=1] LDA   #$7F */
    IMM(0x9ba6,2);
    set_a(s,0x7fu);
    goto source_9ba8_m1x1;
source_9ba8_m1x1: /* $80:9BA8 [M=1 X=1] STA   $062F */
    WRITE(0x9ba8,3,DB(0x62fu),1,false,A);
    goto source_9bab_m1x1;
source_9bab_m1x1: /* $80:9BAB [M=1 X=1] STZ   $062E */
    WRITE(0x9bab,3,DB(0x62eu),1,false,0);
    goto source_9bae_m1x1;
source_9bae_m1x1: /* $80:9BAE [M=1 X=1] INC   A */
    IMP(0x9bae);
    accumulator_change(s,SOUND_INC);
    goto source_9baf_m1x1;
source_9baf_m1x1: /* $80:9BAF [M=1 X=1] STA   $062D */
    WRITE(0x9baf,3,DB(0x62du),1,false,A);
    goto source_9bb2_m1x1;
source_9bb2_m1x1: /* $80:9BB2 [M=1 X=1] JSL   $809BD5 */
    SUSPEND(jsl_call(s,0x9bb2));
    B.registers.stack_pointer-=3u;
    goto source_9bd5_m1x1;
source_9bd5_m1x1: /* $80:9BD5 [M=1 X=1] PHP    */
    PUSH(0x9bd5,1,P,1);
    goto source_9bd6_m1x1;
source_9bd6_m1x1: /* $80:9BD6 [M=1 X=1] PHB    */
    PUSH(0x9bd6,1,B.registers.data_bank,1);
    goto source_9bd7_m1x1;
source_9bd7_m1x1: /* $80:9BD7 [M=1 X=1] PHK    */
    PUSH(0x9bd7,1,s->program_bank,1);
    goto source_9bd8_m1x1;
source_9bd8_m1x1: /* $80:9BD8 [M=1 X=1] PLB    */
    PULL(0x9bd8,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_9bd9_m1x1;
source_9bd9_m1x1: /* $80:9BD9 [M=1 X=1] PHP    */
    PUSH(0x9bd9,1,P,1);
    goto source_9bda_m1x1;
source_9bda_m1x1: /* $80:9BDA [M=1 X=1] SEP   #$20 */
    MODE(0x9bda);
    status_set(s,P | 0x20u);
    goto source_9bdc_m1x1;
source_9bdc_m1x1: /* $80:9BDC [M=1 X=1] PHA    */
    PUSH(0x9bdc,1,A,1);
    goto source_9bdd_m1x1;
source_9bdd_m1x1: /* $80:9BDD [M=1 X=1] PHB    */
    PUSH(0x9bdd,1,B.registers.data_bank,1);
    goto source_9bde_m1x1;
source_9bde_m1x1: /* $80:9BDE [M=1 X=1] PLA    */
    PULL(0x9bde,1,false);
    set_a(s,B.read_value);
    goto source_9bdf_m1x1;
source_9bdf_m1x1: /* $80:9BDF [M=1 X=1] CMP   #$7E */
    IMM(0x9bdf,2);
    compare(s,A,0x7eu,1);
    goto source_9be1_m1x1;
source_9be1_m1x1: /* $80:9BE1 [M=1 X=1] BEQ   $9BE7 */
    BRANCH(0x9be1,(P&2)!=0);
    if(B.branch) { goto source_9be7_m1x1; }
    goto source_9be3_m1x1;
source_9be3_m1x1: /* $80:9BE3 [M=1 X=1] CMP   #$7F */
    IMM(0x9be3,2);
    compare(s,A,0x7fu,1);
    goto source_9be5_m1x1;
source_9be5_m1x1: /* $80:9BE5 [M=1 X=1] BNE   $9BE9 */
    BRANCH(0x9be5,(!(P&2))!=0);
    if(B.branch) { goto source_9be9_m1x1; }
    goto source_9be7_m1x1;
source_9be7_m1x1: /* $80:9BE7 [M=1 X=1] PHK    */
    PUSH(0x9be7,1,s->program_bank,1);
    goto source_9be8_m1x1;
source_9be8_m1x1: /* $80:9BE8 [M=1 X=1] PLB    */
    PULL(0x9be8,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_9be9_m1x1;
source_9be9_m1x1: /* $80:9BE9 [M=1 X=1] PLA    */
    PULL(0x9be9,1,false);
    set_a(s,B.read_value);
    goto source_9bea_m1x1;
source_9bea_m1x1: /* $80:9BEA [M=1 X=1] PLP    */
    PULL(0x9bea,1,false);
    status_set(s,B.read_value);
    goto source_9beb_m1x1;
source_9beb_m1x1: /* $80:9BEB [M=1 X=1] SEP   #$30 */
    MODE(0x9beb);
    status_set(s,P | 0x30u);
    goto source_9bed_m1x1;
source_9bed_m1x1: /* $80:9BED [M=1 X=1] DEC   $53 */
    SUSPEND(decrement_guard(s,0x9bed));
    nz(s,(uint8_t)(B.read_value-1u),1);
    goto source_9bef_m1x1;
source_9bef_m1x1: /* $80:9BEF [M=1 X=1] LDY   #$07 */
    IMM(0x9bef,2);
    set_y(s,0x7u);
    goto source_9bf1_m1x1;
source_9bf1_m1x1: /* $80:9BF1 [M=1 X=1] JSR   $A4FC */
    PUSH(0x9bf1,3,0x9bf3,2);
    goto source_a4fc_m1x1;
source_a4fc_m1x1: /* $80:A4FC [M=1 X=1] SEP   #$30 */
    MODE(0xa4fc);
    status_set(s,P | 0x30u);
    goto source_a4fe_m1x1;
source_a4fe_m1x1: /* $80:A4FE [M=1 X=1] LDA   #$00 */
    IMM(0xa4fe,2);
    set_a(s,0x0u);
    goto source_a500_m1x1;
source_a500_m1x1: /* $80:A500 [M=1 X=1] STA   $073A,Y */
    WRITE(0xa500,3,DB(0x73au+Y),1,true,A);
    goto source_a503_m1x1;
source_a503_m1x1: /* $80:A503 [M=1 X=1] LDA   #$FF */
    IMM(0xa503,2);
    set_a(s,0xffu);
    goto source_a505_m1x1;
source_a505_m1x1: /* $80:A505 [M=1 X=1] STA   $0742,Y */
    WRITE(0xa505,3,DB(0x742u+Y),1,true,A);
    goto source_a508_m1x1;
source_a508_m1x1: /* $80:A508 [M=1 X=1] JMP   $AACD */
    IMM(0xa508,3);
    s->program_bank=0x80;
    goto source_aacd_m1x1;
source_aacd_m1x1: /* $80:AACD [M=1 X=1] LDA   $2140 */
    READ(0xaacd,3,DB(0x2140u),1,false);
    set_a(s,B.read_value);
    goto source_aad0_m1x1;
source_aad0_m1x1: /* $80:AAD0 [M=1 X=1] BNE   $AACD */
    BRANCH(0xaad0,(!(P&2))!=0);
    if(B.branch) { goto source_aacd_m1x1; }
    return boundary(s,0x80aad2u);
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
    case 0x80c0: goto source_80c0_m0x0;
    case 0x8101: goto source_8101_m0x0;
    case 0x8141: goto source_8141_m0x0;
    case 0x814e: return boundary(s,0x80814eu);
    case 0x9bb6: return boundary(s,0x809bb6u);
    case 0x9bf4: return boundary(s,0x809bf4u);
    case 0xac07: goto source_ac07_m0x0;
    case 0xac0b: goto source_ac0b_m0x0;
    default:return boundary(s,0x80ab7du);}
source_ab7e_m0x0: /* $80:AB7E [M=0 X=0] STA   $05EB */
    WRITE(0xab7e,3,DB(0x5ebu),2,false,A);
    goto source_ab81_m0x0;
source_ab81_m0x0: /* $80:AB81 [M=0 X=0] PHB    */
    PUSH(0xab81,1,B.registers.data_bank,1);
    goto source_ab82_m0x0;
source_ab82_m0x0: /* $80:AB82 [M=0 X=0] PEA   $7E7E */
    SUSPEND(pea(s,0xab82,0x7e7e));
    B.registers.stack_pointer-=2u;
    goto source_ab85_m0x0;
source_ab85_m0x0: /* $80:AB85 [M=0 X=0] PLB    */
    PULL(0xab85,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_ab86_m0x0;
source_ab86_m0x0: /* $80:AB86 [M=0 X=0] PLB    */
    PULL(0xab86,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_ab87_m0x0;
source_ab87_m0x0: /* $80:AB87 [M=0 X=0] STX   $05ED */
    WRITE(0xab87,3,DB(0x5edu),2,false,X);
    goto source_ab8a_m0x0;
source_ab8a_m0x0: /* $80:AB8A [M=0 X=0] TYA    */
    IMP(0xab8a);
    set_a(s,Y);
    goto source_ab8b_m0x0;
source_ab8b_m0x0: /* $80:AB8B [M=0 X=0] CMP   #$01C0 */
    IMM(0xab8b,3);
    compare(s,A,0x1c0u,2);
    goto source_ab8e_m0x0;
source_ab8e_m0x0: /* $80:AB8E [M=0 X=0] BCS   $AB93 */
    BRANCH(0xab8e,(P&1)!=0);
    if(B.branch) { goto source_ab93_m0x0; }
    goto source_ab90_m0x0;
source_ab90_m0x0: /* $80:AB90 [M=0 X=0] LDA   #$01A0 */
    IMM(0xab90,3);
    set_a(s,0x1a0u);
    goto source_ab93_m0x0;
source_ab93_m0x0: /* $80:AB93 [M=0 X=0] CLC    */
    IMP(0xab93);
    P&=0xfeu;
    goto source_ab94_m0x0;
source_ab94_m0x0: /* $80:AB94 [M=0 X=0] ADC   #$1F */
    IMM(0xab94,3);
    add(s,0x1fu);
    goto source_ab97_m0x0;
source_ab97_m0x0: /* $80:AB97 [M=0 X=0] AND   #$FFE0 */
    IMM(0xab97,3);
    set_a(s,A&0xffe0u);
    goto source_ab9a_m0x0;
source_ab9a_m0x0: /* $80:AB9A [M=0 X=0] STA   $05EF */
    WRITE(0xab9a,3,DB(0x5efu),2,false,A);
    goto source_ab9d_m0x0;
source_ab9d_m0x0: /* $80:AB9D [M=0 X=0] STZ   $05F1 */
    WRITE(0xab9d,3,DB(0x5f1u),2,false,0);
    goto source_aba0_m0x0;
source_aba0_m0x0: /* $80:ABA0 [M=0 X=0] LSR   A */
    IMP(0xaba0);
    accumulator_change(s,SOUND_LSR);
    goto source_aba1_m0x0;
source_aba1_m0x0: /* $80:ABA1 [M=0 X=0] LSR   A */
    IMP(0xaba1);
    accumulator_change(s,SOUND_LSR);
    goto source_aba2_m0x0;
source_aba2_m0x0: /* $80:ABA2 [M=0 X=0] SEP   #$30 */
    MODE(0xaba2);
    status_set(s,P | 0x30u);
    goto source_aba4_m1x1;
source_aba4_m1x1: /* $80:ABA4 [M=1 X=1] TAX    */
    IMP(0xaba4);
    set_x(s,A);
    goto source_aba5_m1x1;
source_aba5_m1x1: /* $80:ABA5 [M=1 X=1] LDA   #$00 */
    IMM(0xaba5,2);
    set_a(s,0x0u);
    goto source_aba7_m1x1;
source_aba7_m1x1: /* $80:ABA7 [M=1 X=1] STA   $3271,X */
    WRITE(0xaba7,3,DB(0x3271u+X),1,true,A);
    goto source_abaa_m1x1;
source_abaa_m1x1: /* $80:ABAA [M=1 X=1] TXA    */
    IMP(0xabaa);
    set_a(s,X);
    goto source_abab_m1x1;
source_abab_m1x1: /* $80:ABAB [M=1 X=1] SEC    */
    IMP(0xabab);
    P|=1;
    goto source_abac_m1x1;
source_abac_m1x1: /* $80:ABAC [M=1 X=1] SBC   #$08 */
    IMM(0xabac,2);
    subtract(s,0x8u);
    goto source_abae_m1x1;
source_abae_m1x1: /* $80:ABAE [M=1 X=1] STA   $32EA,X */
    WRITE(0xabae,3,DB(0x32eau+X),1,true,A);
    goto source_abb1_m1x1;
source_abb1_m1x1: /* $80:ABB1 [M=1 X=1] TAX    */
    IMP(0xabb1);
    set_x(s,A);
    goto source_abb2_m1x1;
source_abb2_m1x1: /* $80:ABB2 [M=1 X=1] LDA   #$08 */
    IMM(0xabb2,2);
    set_a(s,0x8u);
    goto source_abb4_m1x1;
source_abb4_m1x1: /* $80:ABB4 [M=1 X=1] CPX   #$00 */
    IMM(0xabb4,2);
    compare(s,X,0x0u,1);
    goto source_abb6_m1x1;
source_abb6_m1x1: /* $80:ABB6 [M=1 X=1] BPL   $ABA7 */
    BRANCH(0xabb6,(!(P&128))!=0);
    if(B.branch) { goto source_aba7_m1x1; }
    goto source_abb8_m1x1;
source_abb8_m1x1: /* $80:ABB8 [M=1 X=1] LDA   #$FF */
    IMM(0xabb8,2);
    set_a(s,0xffu);
    goto source_abba_m1x1;
source_abba_m1x1: /* $80:ABBA [M=1 X=1] STA   $32EA */
    WRITE(0xabba,3,DB(0x32eau),1,false,A);
    goto source_abbd_m1x1;
source_abbd_m1x1: /* $80:ABBD [M=1 X=1] REP   #$30 */
    MODE(0xabbd);
    status_set(s,P & 0xcfu);
    goto source_abbf_m0x0;
source_abbf_m0x0: /* $80:ABBF [M=0 X=0] LDA   #$02 */
    IMM(0xabbf,3);
    set_a(s,0x2u);
    goto source_abc2_m0x0;
source_abc2_m0x0: /* $80:ABC2 [M=0 X=0] STA   $05F9 */
    WRITE(0xabc2,3,DB(0x5f9u),2,false,A);
    goto source_abc5_m0x0;
source_abc5_m0x0: /* $80:ABC5 [M=0 X=0] LDA   #$0200 */
    IMM(0xabc5,3);
    set_a(s,0x200u);
    goto source_abc8_m0x0;
source_abc8_m0x0: /* $80:ABC8 [M=0 X=0] SEC    */
    IMP(0xabc8);
    P|=1;
    goto source_abc9_m0x0;
source_abc9_m0x0: /* $80:ABC9 [M=0 X=0] SBC   $05EF */
    READ(0xabc9,3,DB(0x5efu),2,false);
    subtract(s,B.read_value);
    goto source_abcc_m0x0;
source_abcc_m0x0: /* $80:ABCC [M=0 X=0] SEP   #$30 */
    MODE(0xabcc);
    status_set(s,P | 0x30u);
    goto source_abce_m1x1;
source_abce_m1x1: /* $80:ABCE [M=1 X=1] TAX    */
    IMP(0xabce);
    set_x(s,A);
    goto source_abcf_m1x1;
source_abcf_m1x1: /* $80:ABCF [M=1 X=1] STA   $3363 */
    WRITE(0xabcf,3,DB(0x3363u),1,false,A);
    goto source_abd2_m1x1;
source_abd2_m1x1: /* $80:ABD2 [M=1 X=1] STZ   $3363,X */
    WRITE(0xabd2,3,DB(0x3363u+X),1,true,0);
    goto source_abd5_m1x1;
source_abd5_m1x1: /* $80:ABD5 [M=1 X=1] STZ   $33C4,X */
    WRITE(0xabd5,3,DB(0x33c4u+X),1,true,0);
    goto source_abd8_m1x1;
source_abd8_m1x1: /* $80:ABD8 [M=1 X=1] LDA   #$FF */
    IMM(0xabd8,2);
    set_a(s,0xffu);
    goto source_abda_m1x1;
source_abda_m1x1: /* $80:ABDA [M=1 X=1] STA   $33C4 */
    WRITE(0xabda,3,DB(0x33c4u),1,false,A);
    goto source_abdd_m1x1;
source_abdd_m1x1: /* $80:ABDD [M=1 X=1] REP   #$30 */
    MODE(0xabdd);
    status_set(s,P & 0xcfu);
    goto source_abdf_m0x0;
source_abdf_m0x0: /* $80:ABDF [M=0 X=0] LDA   #$80 */
    IMM(0xabdf,3);
    set_a(s,0x80u);
    goto source_abe2_m0x0;
source_abe2_m0x0: /* $80:ABE2 [M=0 X=0] STA   $05E5 */
    WRITE(0xabe2,3,DB(0x5e5u),2,false,A);
    goto source_abe5_m0x0;
source_abe5_m0x0: /* $80:ABE5 [M=0 X=0] LDA   #$2000 */
    IMM(0xabe5,3);
    set_a(s,0x2000u);
    goto source_abe8_m0x0;
source_abe8_m0x0: /* $80:ABE8 [M=0 X=0] STA   $05F3 */
    WRITE(0xabe8,3,DB(0x5f3u),2,false,A);
    goto source_abeb_m0x0;
source_abeb_m0x0: /* $80:ABEB [M=0 X=0] LDA   #$2200 */
    IMM(0xabeb,3);
    set_a(s,0x2200u);
    goto source_abee_m0x0;
source_abee_m0x0: /* $80:ABEE [M=0 X=0] STA   $05F5 */
    WRITE(0xabee,3,DB(0x5f5u),2,false,A);
    goto source_abf1_m0x0;
source_abf1_m0x0: /* $80:ABF1 [M=0 X=0] LDA   #$2420 */
    IMM(0xabf1,3);
    set_a(s,0x2420u);
    goto source_abf4_m0x0;
source_abf4_m0x0: /* $80:ABF4 [M=0 X=0] STA   $05DF */
    WRITE(0xabf4,3,DB(0x5dfu),2,false,A);
    goto source_abf7_m0x0;
source_abf7_m0x0: /* $80:ABF7 [M=0 X=0] LDA   #$2220 */
    IMM(0xabf7,3);
    set_a(s,0x2220u);
    goto source_abfa_m0x0;
source_abfa_m0x0: /* $80:ABFA [M=0 X=0] STA   $05E3 */
    WRITE(0xabfa,3,DB(0x5e3u),2,false,A);
    goto source_abfd_m0x0;
source_abfd_m0x0: /* $80:ABFD [M=0 X=0] LDA   #$2000 */
    IMM(0xabfd,3);
    set_a(s,0x2000u);
    goto source_ac00_m0x0;
source_ac00_m0x0: /* $80:AC00 [M=0 X=0] STA   $05E1 */
    WRITE(0xac00,3,DB(0x5e1u),2,false,A);
    goto source_ac03_m0x0;
source_ac03_m0x0: /* $80:AC03 [M=0 X=0] JSL   $80AC0D */
    SUSPEND(jsl_call(s,0xac03));
    B.registers.stack_pointer-=3u;
    goto source_ac0d_m0x0;
source_ac07_m0x0: /* $80:AC07 [M=0 X=0] JSL   $80AC89 */
    SUSPEND(jsl_call(s,0xac07));
    B.registers.stack_pointer-=3u;
    goto source_ac89_m0x0;
source_ac0b_m0x0: /* $80:AC0B [M=0 X=0] PLB    */
    PULL(0xac0b,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_ac0c_m0x0;
source_ac0c_m0x0: /* $80:AC0C [M=0 X=0] RTL    */
    PULL(0xac0c,3,false);
    if(s->read_bank!=0x80)return boundary(s,0x80ab7du);
    switch((uint16_t)(B.read_value+1u)){
    case 0x8043: goto source_8043_m0x0;
    case 0x80c0: goto source_80c0_m0x0;
    case 0x8101: goto source_8101_m0x0;
    case 0x8141: goto source_8141_m0x0;
    case 0x814e: return boundary(s,0x80814eu);
    case 0x9bb6: return boundary(s,0x809bb6u);
    case 0x9bf4: return boundary(s,0x809bf4u);
    case 0xac07: goto source_ac07_m0x0;
    case 0xac0b: goto source_ac0b_m0x0;
    default:return boundary(s,0x80ab7du);}
source_ac0d_m0x0: /* $80:AC0D [M=0 X=0] LDX   #$0830 */
    IMM(0xac0d,3);
    set_x(s,0x830u);
    goto source_ac10_m0x0;
source_ac10_m0x0: /* $80:AC10 [M=0 X=0] LDA   #$FFFF */
    IMM(0xac10,3);
    set_a(s,0xffffu);
    goto source_ac13_m0x0;
source_ac13_m0x0: /* $80:AC13 [M=0 X=0] STA   $2640,X */
    WRITE(0xac13,3,DB(0x2640u+X),2,true,A);
    goto source_ac16_m0x0;
source_ac16_m0x0: /* $80:AC16 [M=0 X=0] DEX    */
    IMP(0xac16);
    set_x(s,X-1u);
    goto source_ac17_m0x0;
source_ac17_m0x0: /* $80:AC17 [M=0 X=0] DEX    */
    IMP(0xac17);
    set_x(s,X-1u);
    goto source_ac18_m0x0;
source_ac18_m0x0: /* $80:AC18 [M=0 X=0] BPL   $AC13 */
    BRANCH(0xac18,(!(P&128))!=0);
    if(B.branch) { goto source_ac13_m0x0; }
    goto source_ac1a_m0x0;
source_ac1a_m0x0: /* $80:AC1A [M=0 X=0] RTL    */
    PULL(0xac1a,3,false);
    if(s->read_bank!=0x80)return boundary(s,0x80ab7du);
    switch((uint16_t)(B.read_value+1u)){
    case 0x8043: goto source_8043_m0x0;
    case 0x80c0: goto source_80c0_m0x0;
    case 0x8101: goto source_8101_m0x0;
    case 0x8141: goto source_8141_m0x0;
    case 0x814e: return boundary(s,0x80814eu);
    case 0x9bb6: return boundary(s,0x809bb6u);
    case 0x9bf4: return boundary(s,0x809bf4u);
    case 0xac07: goto source_ac07_m0x0;
    case 0xac0b: goto source_ac0b_m0x0;
    default:return boundary(s,0x80ab7du);}
source_ac89_m0x0: /* $80:AC89 [M=0 X=0] LDA   $05E5 */
    READ(0xac89,3,DB(0x5e5u),2,false);
    set_a(s,B.read_value);
    goto source_ac8c_m0x0;
source_ac8c_m0x0: /* $80:AC8C [M=0 X=0] LDX   $05F3 */
    READ(0xac8c,3,DB(0x5f3u),2,false);
    set_x(s,B.read_value);
    goto source_ac8f_m0x0;
source_ac8f_m0x0: /* $80:AC8F [M=0 X=0] LDA   #$E100 */
    IMM(0xac8f,3);
    set_a(s,0xe100u);
    goto source_ac92_m0x0;
source_ac92_m0x0: /* $80:AC92 [M=0 X=0] CPX   $05F5 */
    READ(0xac92,3,DB(0x5f5u),2,false);
    compare(s,X,B.read_value,2);
    goto source_ac95_m0x0;
source_ac95_m0x0: /* $80:AC95 [M=0 X=0] BCS   $ACA0 */
    BRANCH(0xac95,(P&1)!=0);
    if(B.branch) { goto source_aca0_m0x0; }
    goto source_ac97_m0x0;
source_ac97_m0x0: /* $80:AC97 [M=0 X=0] STA   $0000,X */
    WRITE(0xac97,3,DB(0x0u+X),2,true,A);
    goto source_ac9a_m0x0;
source_ac9a_m0x0: /* $80:AC9A [M=0 X=0] INX    */
    IMP(0xac9a);
    set_x(s,X+1u);
    goto source_ac9b_m0x0;
source_ac9b_m0x0: /* $80:AC9B [M=0 X=0] INX    */
    IMP(0xac9b);
    set_x(s,X+1u);
    goto source_ac9c_m0x0;
source_ac9c_m0x0: /* $80:AC9C [M=0 X=0] INX    */
    IMP(0xac9c);
    set_x(s,X+1u);
    goto source_ac9d_m0x0;
source_ac9d_m0x0: /* $80:AC9D [M=0 X=0] INX    */
    IMP(0xac9d);
    set_x(s,X+1u);
    goto source_ac9e_m0x0;
source_ac9e_m0x0: /* $80:AC9E [M=0 X=0] BNE   $AC92 */
    BRANCH(0xac9e,(!(P&2))!=0);
    if(B.branch) { goto source_ac92_m0x0; }
    goto source_aca0_m0x0;
source_aca0_m0x0: /* $80:ACA0 [M=0 X=0] LDY   $05DF */
    READ(0xaca0,3,DB(0x5dfu),2,false);
    set_y(s,B.read_value);
    goto source_aca3_m0x0;
source_aca3_m0x0: /* $80:ACA3 [M=0 X=0] LDA   $05F3 */
    READ(0xaca3,3,DB(0x5f3u),2,false);
    set_a(s,B.read_value);
    goto source_aca6_m0x0;
source_aca6_m0x0: /* $80:ACA6 [M=0 X=0] STA   $05DF */
    WRITE(0xaca6,3,DB(0x5dfu),2,false,A);
    goto source_aca9_m0x0;
source_aca9_m0x0: /* $80:ACA9 [M=0 X=0] STY   $05F5 */
    WRITE(0xaca9,3,DB(0x5f5u),2,false,Y);
    goto source_acac_m0x0;
source_acac_m0x0: /* $80:ACAC [M=0 X=0] LDX   $05E1 */
    READ(0xacac,3,DB(0x5e1u),2,false);
    set_x(s,B.read_value);
    goto source_acaf_m0x0;
source_acaf_m0x0: /* $80:ACAF [M=0 X=0] LDY   $05E3 */
    READ(0xacaf,3,DB(0x5e3u),2,false);
    set_y(s,B.read_value);
    goto source_acb2_m0x0;
source_acb2_m0x0: /* $80:ACB2 [M=0 X=0] STX   $05E3 */
    WRITE(0xacb2,3,DB(0x5e3u),2,false,X);
    goto source_acb5_m0x0;
source_acb5_m0x0: /* $80:ACB5 [M=0 X=0] STY   $05E1 */
    WRITE(0xacb5,3,DB(0x5e1u),2,false,Y);
    goto source_acb8_m0x0;
source_acb8_m0x0: /* $80:ACB8 [M=0 X=0] SEP   #$20 */
    MODE(0xacb8);
    status_set(s,P | 0x20u);
    goto source_acba_m1x0;
source_acba_m1x0: /* $80:ACBA [M=1 X=0] LDA   #$01 */
    IMM(0xacba,2);
    set_a(s,0x1u);
    goto source_acbc_m1x0;
source_acbc_m1x0: /* $80:ACBC [M=1 X=0] STA   $0566 */
    WRITE(0xacbc,3,DB(0x566u),1,false,A);
    goto source_acbf_m1x0;
source_acbf_m1x0: /* $80:ACBF [M=1 X=0] REP   #$20 */
    MODE(0xacbf);
    status_set(s,P & 0xdfu);
    goto source_acc1_m0x0;
source_acc1_m0x0: /* $80:ACC1 [M=0 X=0] RTL    */
    PULL(0xacc1,3,false);
    if(s->read_bank!=0x80)return boundary(s,0x80ab7du);
    switch((uint16_t)(B.read_value+1u)){
    case 0x8043: goto source_8043_m0x0;
    case 0x80c0: goto source_80c0_m0x0;
    case 0x8101: goto source_8101_m0x0;
    case 0x8141: goto source_8141_m0x0;
    case 0x814e: return boundary(s,0x80814eu);
    case 0x9bb6: return boundary(s,0x809bb6u);
    case 0x9bf4: return boundary(s,0x809bf4u);
    case 0xac07: goto source_ac07_m0x0;
    case 0xac0b: goto source_ac0b_m0x0;
    default:return boundary(s,0x80ab7du);}
source_da72_m1x0: /* $80:DA72 [M=1 X=0] REP   #$30 */
    MODE(0xda72);
    status_set(s,P & 0xcfu);
    goto source_da74_m0x0;
source_da74_m0x0: /* $80:DA74 [M=0 X=0] PHB    */
    PUSH(0xda74,1,B.registers.data_bank,1);
    goto source_da75_m0x0;
source_da75_m0x0: /* $80:DA75 [M=0 X=0] PEA   $7E7E */
    SUSPEND(pea(s,0xda75,0x7e7e));
    B.registers.stack_pointer-=2u;
    goto source_da78_m0x0;
source_da78_m0x0: /* $80:DA78 [M=0 X=0] PLB    */
    PULL(0xda78,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_da79_m0x0;
source_da79_m0x0: /* $80:DA79 [M=0 X=0] PLB    */
    PULL(0xda79,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_da7a_m0x0;
source_da7a_m0x0: /* $80:DA7A [M=0 X=0] LDA   #$01 */
    IMM(0xda7a,3);
    set_a(s,0x1u);
    goto source_da7d_m0x0;
source_da7d_m0x0: /* $80:DA7D [M=0 X=0] STA   $062C */
    WRITE(0xda7d,3,DB(0x62cu),2,false,A);
    goto source_da80_m0x0;
source_da80_m0x0: /* $80:DA80 [M=0 X=0] INC   A */
    IMP(0xda80);
    accumulator_change(s,SOUND_INC);
    goto source_da81_m0x0;
source_da81_m0x0: /* $80:DA81 [M=0 X=0] STA   $07F2 */
    WRITE(0xda81,3,DB(0x7f2u),2,false,A);
    goto source_da84_m0x0;
source_da84_m0x0: /* $80:DA84 [M=0 X=0] LDA   #$00 */
    IMM(0xda84,3);
    set_a(s,0x0u);
    goto source_da87_m0x0;
source_da87_m0x0: /* $80:DA87 [M=0 X=0] STA   $0007FE */
    WRITE(0xda87,4,0x7feu,2,false,A);
    goto source_da8b_m0x0;
source_da8b_m0x0: /* $80:DA8B [M=0 X=0] STA   $0008FE */
    WRITE(0xda8b,4,0x8feu,2,false,A);
    goto source_da8f_m0x0;
source_da8f_m0x0: /* $80:DA8F [M=0 X=0] PLB    */
    PULL(0xda8f,1,false);
    B.registers.data_bank=(uint8_t)B.read_value; nz(s,B.read_value,1);
    goto source_da90_m0x0;
source_da90_m0x0: /* $80:DA90 [M=0 X=0] RTL    */
    PULL(0xda90,3,false);
    if(s->read_bank!=0x80)return boundary(s,0x80ab7du);
    switch((uint16_t)(B.read_value+1u)){
    case 0x8043: goto source_8043_m0x0;
    case 0x80c0: goto source_80c0_m0x0;
    case 0x8101: goto source_8101_m0x0;
    case 0x8141: goto source_8141_m0x0;
    case 0x814e: return boundary(s,0x80814eu);
    case 0x9bb6: return boundary(s,0x809bb6u);
    case 0x9bf4: return boundary(s,0x809bf4u);
    case 0xac07: goto source_ac07_m0x0;
    case 0xac0b: goto source_ac0b_m0x0;
    default:return boundary(s,0x80ab7du);}
default:return boundary(s,0); }}
