/* Generated from canonical ROM only by tools/generate_setup_fb30_work.py.
 * No capture, fitted cost, opcode dispatcher or runtime ROM execution.
 * Each label is static C source flow; SUSPEND labels are C continuations. */
static bool advance(NbaSetupFb30Work *s)
{
    switch (B.resume) {
    case 0: goto source_c62b_m0x0;
source_86da_m0x0: /* $80:86DA [M=0 X=0] PHA */
        PUSH(0x86da, 1, A, 2);
        goto source_86db_m0x0;
source_86db_m0x0: /* $80:86DB [M=0 X=0] LDA   $0561 */
        READ(0x86db, 3, DB(0x0561u), 2, false);
        set_a(s, B.read_value);
        goto source_86de_m0x0;
source_86de_m0x0: /* $80:86DE [M=0 X=0] BMI   $86E8 */
        BRANCH(0x86de, (P & 0x80u) != 0);
        if (B.branch) { goto source_86e8_m0x0; }
        goto source_86e0_m0x0;
source_86e0_m0x0: /* $80:86E0 [M=0 X=0] LDA   $35 */
        READ(0x86e0, 2, 0x35u, 2, false);
        set_a(s, B.read_value);
        goto source_86e2_m0x0;
source_86e2_m0x0: /* $80:86E2 [M=0 X=0] CMP   $37 */
        READ(0x86e2, 2, 0x37u, 2, false);
        compare(s, A, B.read_value, 2);
        goto source_86e4_m0x0;
source_86e4_m0x0: /* $80:86E4 [M=0 X=0] BNE   $86E0 */
        BRANCH(0x86e4, (P & 2u) == 0);
        if (B.branch) return unsupported(s);
        goto source_86e6_m0x0;
source_86e6_m0x0: /* $80:86E6 [M=0 X=0] PLA */
        PULL(0x86e6, 2, false);
        set_a(s, B.read_value);
        goto source_86e7_m0x0;
source_86e7_m0x0: /* $80:86E7 [M=0 X=0] RTL */
        PULL(0x86e7, 3, false);
        if (B.read_value != 0xc632 || s->read_bank != 0x80) return unsupported(s);
        goto source_c633_m0x0;
source_86e8_m0x0: /* $80:86E8 [M=0 X=0] LDA   $35 */
        READ(0x86e8, 2, 0x35u, 2, false);
        set_a(s, B.read_value);
        goto source_86ea_m0x0;
source_86ea_m0x0: /* $80:86EA [M=0 X=0] CMP   $37 */
        READ(0x86ea, 2, 0x37u, 2, false);
        compare(s, A, B.read_value, 2);
        goto source_86ec_m0x0;
source_86ec_m0x0: /* $80:86EC [M=0 X=0] BEQ   $86E6 */
        BRANCH(0x86ec, (P & 2u) != 0);
        if (B.branch) { goto source_86e6_m0x0; }
        return unsupported(s);
source_be6b_m0x0: /* $80:BE6B [M=0 X=0] SEP   #$10 */
        MODE(0xbe6b);
        status_set(s, P | 0x10u);
        goto source_be6d_m0x1;
source_be6d_m0x1: /* $80:BE6D [M=0 X=1] LDA   $0C */
        READ(0xbe6d, 2, 0x0cu, 2, false);
        set_a(s, B.read_value);
        goto source_be6f_m0x1;
source_be6f_m0x1: /* $80:BE6F [M=0 X=1] CLC */
        IMP(0xbe6f);
        P = (uint8_t)(P & 0xfeu);
        goto source_be70_m0x1;
source_be70_m0x1: /* $80:BE70 [M=0 X=1] ADC   #$05 */
        IMM(0xbe70, 3);
        add_sub(s, 0x0005u, false);
        goto source_be73_m0x1;
source_be73_m0x1: /* $80:BE73 [M=0 X=1] STA   $0C */
        WRITE(0xbe73, 2, 0x0cu, 2, false, A);
        goto source_be75_m0x1;
source_be75_m0x1: /* $80:BE75 [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xbe75, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_be77_m0x1;
source_be77_m0x1: /* $80:BE77 [M=0 X=1] STA   $25 */
        WRITE(0xbe77, 2, 0x25u, 2, false, A);
        goto source_be79_m0x1;
source_be79_m0x1: /* $80:BE79 [M=0 X=1] INC   $0C */
        CHANGE(0xbe79, 2, 0x0cu, 2, false, FB30_INC);
        goto source_be7b_m0x1;
source_be7b_m0x1: /* $80:BE7B [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xbe7b, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_be7d_m0x1;
source_be7d_m0x1: /* $80:BE7D [M=0 X=1] INC   $0C */
        CHANGE(0xbe7d, 2, 0x0cu, 2, false, FB30_INC);
        goto source_be7f_m0x1;
source_be7f_m0x1: /* $80:BE7F [M=0 X=1] INC   $0C */
        CHANGE(0xbe7f, 2, 0x0cu, 2, false, FB30_INC);
        goto source_be81_m0x1;
source_be81_m0x1: /* $80:BE81 [M=0 X=1] XBA */
        SUSPEND(implied(s, 0xbe81, 1, 2));
        A = (uint16_t)((A << 8) | (A >> 8));
        nz(s, A, 1);
        goto source_be82_m0x1;
source_be82_m0x1: /* $80:BE82 [M=0 X=1] STA   $1E */
        WRITE(0xbe82, 2, 0x1eu, 2, false, A);
        goto source_be84_m0x1;
source_be84_m0x1: /* $80:BE84 [M=0 X=1] LDY   #$08 */
        IMM(0xbe84, 2);
        set_y(s, 0x0008u);
        goto source_be86_m0x1;
source_be86_m0x1: /* $80:BE86 [M=0 X=1] STZ   $29 */
        WRITE(0xbe86, 2, 0x29u, 2, false, 0);
        goto source_be88_m0x1;
source_be88_m0x1: /* $80:BE88 [M=0 X=1] STZ   $27 */
        WRITE(0xbe88, 2, 0x27u, 2, false, 0);
        goto source_be8a_m0x1;
source_be8a_m0x1: /* $80:BE8A [M=0 X=1] LDA   #$10 */
        IMM(0xbe8a, 3);
        set_a(s, 0x0010u);
        goto source_be8d_m0x1;
source_be8d_m0x1: /* $80:BE8D [M=0 X=1] STA   $14 */
        WRITE(0xbe8d, 2, 0x14u, 2, false, A);
        goto source_be8f_m0x1;
source_be8f_m0x1: /* $80:BE8F [M=0 X=1] LDX   #$FE */
        IMM(0xbe8f, 2);
        set_x(s, 0x00feu);
        goto source_be91_m0x1;
source_be91_m0x1: /* $80:BE91 [M=0 X=1] INX */
        IMP(0xbe91);
        set_x(s, X + 1u);
        goto source_be92_m0x1;
source_be92_m0x1: /* $80:BE92 [M=0 X=1] INX */
        IMP(0xbe92);
        set_x(s, X + 1u);
        goto source_be93_m0x1;
source_be93_m0x1: /* $80:BE93 [M=0 X=1] DEC   $14 */
        CHANGE(0xbe93, 2, 0x14u, 2, false, FB30_DEC);
        goto source_be95_m0x1;
source_be95_m0x1: /* $80:BE95 [M=0 X=1] ASL   $27 */
        CHANGE(0xbe95, 2, 0x27u, 2, false, FB30_ASL);
        goto source_be97_m0x1;
source_be97_m0x1: /* $80:BE97 [M=0 X=1] LDA   $27 */
        READ(0xbe97, 2, 0x27u, 2, false);
        set_a(s, B.read_value);
        goto source_be99_m0x1;
source_be99_m0x1: /* $80:BE99 [M=0 X=1] SEC */
        IMP(0xbe99);
        P = (uint8_t)(P | 1u);
        goto source_be9a_m0x1;
source_be9a_m0x1: /* $80:BE9A [M=0 X=1] SBC   $29 */
        READ(0xbe9a, 2, 0x29u, 2, false);
        add_sub(s, B.read_value, true);
        goto source_be9c_m0x1;
source_be9c_m0x1: /* $80:BE9C [M=0 X=1] STA   $0520,X */
        WRITE(0xbe9c, 3, DB(0x0520u + X), 2, true, A);
        goto source_be9f_m0x1;
source_be9f_m0x1: /* $80:BE9F [M=0 X=1] JSR   $C468 */
        PUSH(0xbe9f, 3, 0xbea1, 2);
        goto source_c468_m0x1;
source_bea2_m0x1: /* $80:BEA2 [M=0 X=1] STA   $0500,X */
        WRITE(0xbea2, 3, DB(0x0500u + X), 2, true, A);
        goto source_bea5_m0x1;
source_bea5_m0x1: /* $80:BEA5 [M=0 X=1] CLC */
        IMP(0xbea5);
        P = (uint8_t)(P & 0xfeu);
        goto source_bea6_m0x1;
source_bea6_m0x1: /* $80:BEA6 [M=0 X=1] ADC   $29 */
        READ(0xbea6, 2, 0x29u, 2, false);
        add_sub(s, B.read_value, false);
        goto source_bea8_m0x1;
source_bea8_m0x1: /* $80:BEA8 [M=0 X=1] STA   $29 */
        WRITE(0xbea8, 2, 0x29u, 2, false, A);
        goto source_beaa_m0x1;
source_beaa_m0x1: /* $80:BEAA [M=0 X=1] LDA   $21 */
        READ(0xbeaa, 2, 0x21u, 2, false);
        set_a(s, B.read_value);
        goto source_beac_m0x1;
source_beac_m0x1: /* $80:BEAC [M=0 X=1] CLC */
        IMP(0xbeac);
        P = (uint8_t)(P & 0xfeu);
        goto source_bead_m0x1;
source_bead_m0x1: /* $80:BEAD [M=0 X=1] ADC   $27 */
        READ(0xbead, 2, 0x27u, 2, false);
        add_sub(s, B.read_value, false);
        goto source_beaf_m0x1;
source_beaf_m0x1: /* $80:BEAF [M=0 X=1] STA   $27 */
        WRITE(0xbeaf, 2, 0x27u, 2, false, A);
        goto source_beb1_m0x1;
source_beb1_m0x1: /* $80:BEB1 [M=0 X=1] LDA   $21 */
        READ(0xbeb1, 2, 0x21u, 2, false);
        set_a(s, B.read_value);
        goto source_beb3_m0x1;
source_beb3_m0x1: /* $80:BEB3 [M=0 X=1] BNE   $BEBA */
        BRANCH(0xbeb3, (P & 2u) == 0);
        if (B.branch) { goto source_beba_m0x1; }
        goto source_beb5_m0x1;
source_beb5_m0x1: /* $80:BEB5 [M=0 X=1] STZ   $0540,X */
        WRITE(0xbeb5, 3, DB(0x0540u + X), 2, true, 0);
        goto source_beb8_m0x1;
source_beb8_m0x1: /* $80:BEB8 [M=0 X=1] BRA   $BE91 */
        BRANCH(0xbeb8, true);
        goto source_be91_m0x1;
source_beba_m0x1: /* $80:BEBA [M=0 X=1] LDA   $27 */
        READ(0xbeba, 2, 0x27u, 2, false);
        set_a(s, B.read_value);
        goto source_bebc_m0x1;
source_bebc_m0x1: /* $80:BEBC [M=0 X=1] STA   $00 */
        WRITE(0xbebc, 2, 0x00u, 2, false, A);
        goto source_bebe_m0x1;
source_bebe_m0x1: /* $80:BEBE [M=0 X=1] LDA   $14 */
        READ(0xbebe, 2, 0x14u, 2, false);
        set_a(s, B.read_value);
        goto source_bec0_m0x1;
source_bec0_m0x1: /* $80:BEC0 [M=0 X=1] CLC */
        IMP(0xbec0);
        P = (uint8_t)(P & 0xfeu);
        goto source_bec1_m0x1;
source_bec1_m0x1: /* $80:BEC1 [M=0 X=1] BEQ   $BEC8 */
        BRANCH(0xbec1, (P & 2u) != 0);
        if (B.branch) { goto source_bec8_m0x1; }
        goto source_bec3_m0x1;
source_bec3_m0x1: /* $80:BEC3 [M=0 X=1] ASL   $00 */
        CHANGE(0xbec3, 2, 0x00u, 2, false, FB30_ASL);
        goto source_bec5_m0x1;
source_bec5_m0x1: /* $80:BEC5 [M=0 X=1] DEC   A */
        IMP(0xbec5);
        accumulator_change(s, FB30_DEC);
        goto source_bec6_m0x1;
source_bec6_m0x1: /* $80:BEC6 [M=0 X=1] BNE   $BEC3 */
        BRANCH(0xbec6, (P & 2u) == 0);
        if (B.branch) { goto source_bec3_m0x1; }
        goto source_bec8_m0x1;
source_bec8_m0x1: /* $80:BEC8 [M=0 X=1] LDA   $00 */
        READ(0xbec8, 2, 0x00u, 2, false);
        set_a(s, B.read_value);
        goto source_beca_m0x1;
source_beca_m0x1: /* $80:BECA [M=0 X=1] STA   $0540,X */
        WRITE(0xbeca, 3, DB(0x0540u + X), 2, true, A);
        goto source_becd_m0x1;
source_becd_m0x1: /* $80:BECD [M=0 X=1] BCC   $BE91 */
        BRANCH(0xbecd, (P & 1u) == 0);
        if (B.branch) { goto source_be91_m0x1; }
        goto source_becf_m0x1;
source_becf_m0x1: /* $80:BECF [M=0 X=1] TXA */
        IMP(0xbecf);
        set_a(s, X);
        goto source_bed0_m0x1;
source_bed0_m0x1: /* $80:BED0 [M=0 X=1] LSR   A */
        IMP(0xbed0);
        accumulator_change(s, FB30_LSR);
        goto source_bed1_m0x1;
source_bed1_m0x1: /* $80:BED1 [M=0 X=1] STA   $2B */
        WRITE(0xbed1, 2, 0x2bu, 2, false, A);
        goto source_bed3_m0x1;
source_bed3_m0x1: /* $80:BED3 [M=0 X=1] LDX   #$3E */
        IMM(0xbed3, 2);
        set_x(s, 0x003eu);
        goto source_bed5_m0x1;
source_bed5_m0x1: /* $80:BED5 [M=0 X=1] STZ   $0300,X */
        WRITE(0xbed5, 3, DB(0x0300u + X), 2, true, 0);
        goto source_bed8_m0x1;
source_bed8_m0x1: /* $80:BED8 [M=0 X=1] STZ   $0340,X */
        WRITE(0xbed8, 3, DB(0x0340u + X), 2, true, 0);
        goto source_bedb_m0x1;
source_bedb_m0x1: /* $80:BEDB [M=0 X=1] STZ   $0380,X */
        WRITE(0xbedb, 3, DB(0x0380u + X), 2, true, 0);
        goto source_bede_m0x1;
source_bede_m0x1: /* $80:BEDE [M=0 X=1] STZ   $03C0,X */
        WRITE(0xbede, 3, DB(0x03c0u + X), 2, true, 0);
        goto source_bee1_m0x1;
source_bee1_m0x1: /* $80:BEE1 [M=0 X=1] DEX */
        IMP(0xbee1);
        set_x(s, X - 1u);
        goto source_bee2_m0x1;
source_bee2_m0x1: /* $80:BEE2 [M=0 X=1] DEX */
        IMP(0xbee2);
        set_x(s, X - 1u);
        goto source_bee3_m0x1;
source_bee3_m0x1: /* $80:BEE3 [M=0 X=1] BPL   $BED5 */
        BRANCH(0xbee3, (P & 0x80u) == 0);
        if (B.branch) { goto source_bed5_m0x1; }
        goto source_bee5_m0x1;
source_bee5_m0x1: /* $80:BEE5 [M=0 X=1] LDX   #$7E */
        IMM(0xbee5, 2);
        set_x(s, 0x007eu);
        goto source_bee7_m0x1;
source_bee7_m0x1: /* $80:BEE7 [M=0 X=1] STX   $2183 */
        WRITE(0xbee7, 3, DB(0x2183u), 1, false, X);
        goto source_beea_m0x1;
source_beea_m0x1: /* $80:BEEA [M=0 X=1] LDA   #$0100 */
        IMM(0xbeea, 3);
        set_a(s, 0x0100u);
        goto source_beed_m0x1;
source_beed_m0x1: /* $80:BEED [M=0 X=1] STA   $2181 */
        WRITE(0xbeed, 3, DB(0x2181u), 2, false, A);
        goto source_bef0_m0x1;
source_bef0_m0x1: /* $80:BEF0 [M=0 X=1] LDX   #$FF */
        IMM(0xbef0, 2);
        set_x(s, 0x00ffu);
        goto source_bef2_m0x1;
source_bef2_m0x1: /* $80:BEF2 [M=0 X=1] JSR   $C468 */
        PUSH(0xbef2, 3, 0xbef4, 2);
        goto source_c468_m0x1;
source_bef5_m0x1: /* $80:BEF5 [M=0 X=1] SEP   #$20 */
        MODE(0xbef5);
        status_set(s, P | 0x20u);
        goto source_bef7_m1x1;
source_bef7_m1x1: /* $80:BEF7 [M=1 X=1] INC   A */
        IMP(0xbef7);
        accumulator_change(s, FB30_INC);
        goto source_bef8_m1x1;
source_bef8_m1x1: /* $80:BEF8 [M=1 X=1] INX */
        IMP(0xbef8);
        set_x(s, X + 1u);
        goto source_bef9_m1x1;
source_bef9_m1x1: /* $80:BEF9 [M=1 X=1] BIT   $0300,X */
        READ(0xbef9, 3, DB(0x0300u + X), 1, index_idle(s, 0x0300u, X));
        bit_test(s, B.read_value);
        goto source_befc_m1x1;
source_befc_m1x1: /* $80:BEFC [M=1 X=1] BMI   $BEF8 */
        BRANCH(0xbefc, (P & 0x80u) != 0);
        if (B.branch) { goto source_bef8_m1x1; }
        goto source_befe_m1x1;
source_befe_m1x1: /* $80:BEFE [M=1 X=1] DEC   A */
        IMP(0xbefe);
        accumulator_change(s, FB30_DEC);
        goto source_beff_m1x1;
source_beff_m1x1: /* $80:BEFF [M=1 X=1] BNE   $BEF8 */
        BRANCH(0xbeff, (P & 2u) == 0);
        if (B.branch) { goto source_bef8_m1x1; }
        goto source_bf01_m1x1;
source_bf01_m1x1: /* $80:BF01 [M=1 X=1] DEC   $0300,X */
        CHANGE(0xbf01, 3, DB(0x0300u + X), 1, true, FB30_DEC);
        goto source_bf04_m1x1;
source_bf04_m1x1: /* $80:BF04 [M=1 X=1] REP   #$20 */
        MODE(0xbf04);
        status_set(s, P & 0xdfu);
        goto source_bf06_m0x1;
source_bf06_m0x1: /* $80:BF06 [M=0 X=1] STX   $2180 */
        WRITE(0xbf06, 3, DB(0x2180u), 1, false, X);
        goto source_bf09_m0x1;
source_bf09_m0x1: /* $80:BF09 [M=0 X=1] DEC   $29 */
        CHANGE(0xbf09, 2, 0x29u, 2, false, FB30_DEC);
        goto source_bf0b_m0x1;
source_bf0b_m0x1: /* $80:BF0B [M=0 X=1] BNE   $BEF2 */
        BRANCH(0xbf0b, (P & 2u) == 0);
        if (B.branch) { goto source_bef2_m0x1; }
        goto source_bf0d_m0x1;
source_bf0d_m0x1: /* $80:BF0D [M=0 X=1] TYA */
        IMP(0xbf0d);
        set_a(s, Y);
        goto source_bf0e_m0x1;
source_bf0e_m0x1: /* $80:BF0E [M=0 X=1] ASL   A */
        IMP(0xbf0e);
        accumulator_change(s, FB30_ASL);
        goto source_bf0f_m0x1;
source_bf0f_m0x1: /* $80:BF0F [M=0 X=1] STA   $23 */
        WRITE(0xbf0f, 2, 0x23u, 2, false, A);
        goto source_bf11_m0x1;
source_bf11_m0x1: /* $80:BF11 [M=0 X=1] SEP   #$20 */
        MODE(0xbf11);
        status_set(s, P | 0x20u);
        goto source_bf13_m1x1;
source_bf13_m1x1: /* $80:BF13 [M=1 X=1] LDX   #$00 */
        IMM(0xbf13, 2);
        set_x(s, 0x0000u);
        goto source_bf15_m1x1;
source_bf15_m1x1: /* $80:BF15 [M=1 X=1] TXY */
        IMP(0xbf15);
        set_y(s, X);
        goto source_bf16_m1x1;
source_bf16_m1x1: /* $80:BF16 [M=1 X=1] STZ   $2D */
        WRITE(0xbf16, 2, 0x2du, 1, false, 0);
        goto source_bf18_m1x1;
source_bf18_m1x1: /* $80:BF18 [M=1 X=1] STX   $00 */
        WRITE(0xbf18, 2, 0x00u, 1, false, X);
        goto source_bf1a_m1x1;
source_bf1a_m1x1: /* $80:BF1A [M=1 X=1] LDA   $2D */
        READ(0xbf1a, 2, 0x2du, 1, false);
        set_a(s, B.read_value);
        goto source_bf1c_m1x1;
source_bf1c_m1x1: /* $80:BF1C [M=1 X=1] ASL   A */
        IMP(0xbf1c);
        accumulator_change(s, FB30_ASL);
        goto source_bf1d_m1x1;
source_bf1d_m1x1: /* $80:BF1D [M=1 X=1] TAX */
        IMP(0xbf1d);
        set_x(s, A);
        goto source_bf1e_m1x1;
source_bf1e_m1x1: /* $80:BF1E [M=1 X=1] CPX   #$10 */
        IMM(0xbf1e, 2);
        compare(s, X, 0x0010u, 1);
        goto source_bf20_m1x1;
source_bf20_m1x1: /* $80:BF20 [M=1 X=1] BEQ   $BF77 */
        BRANCH(0xbf20, (P & 2u) != 0);
        if (B.branch) { goto source_bf77_m1x1; }
        goto source_bf22_m1x1;
source_bf22_m1x1: /* $80:BF22 [M=1 X=1] LDA   $0500,X */
        READ(0xbf22, 3, DB(0x0500u + X), 1, index_idle(s, 0x0500u, X));
        set_a(s, B.read_value);
        goto source_bf25_m1x1;
source_bf25_m1x1: /* $80:BF25 [M=1 X=1] STA   $29 */
        WRITE(0xbf25, 2, 0x29u, 1, false, A);
        goto source_bf27_m1x1;
source_bf27_m1x1: /* $80:BF27 [M=1 X=1] LDX   $2D */
        READ(0xbf27, 2, 0x2du, 1, false);
        set_x(s, B.read_value);
        goto source_bf29_m1x1;
source_bf29_m1x1: /* $80:BF29 [M=1 X=1] LDA   $80BF33,X */
        READ(0xbf29, 4, (0x80bf33u + X), 1, false);
        set_a(s, B.read_value);
        goto source_bf2d_m1x1;
source_bf2d_m1x1: /* $80:BF2D [M=1 X=1] STA   $2F */
        WRITE(0xbf2d, 2, 0x2fu, 1, false, A);
        goto source_bf2f_m1x1;
source_bf2f_m1x1: /* $80:BF2F [M=1 X=1] LDX   $00 */
        READ(0xbf2f, 2, 0x00u, 1, false);
        set_x(s, B.read_value);
        goto source_bf31_m1x1;
source_bf31_m1x1: /* $80:BF31 [M=1 X=1] BRA   $BF69 */
        BRANCH(0xbf31, true);
        goto source_bf69_m1x1;
source_bf3b_m1x1: /* $80:BF3B [M=1 X=1] LDA   $2D */
        READ(0xbf3b, 2, 0x2du, 1, false);
        set_a(s, B.read_value);
        goto source_bf3d_m1x1;
source_bf3d_m1x1: /* $80:BF3D [M=1 X=1] ASL   A */
        IMP(0xbf3d);
        accumulator_change(s, FB30_ASL);
        goto source_bf3e_m1x1;
source_bf3e_m1x1: /* $80:BF3E [M=1 X=1] STA   $00 */
        WRITE(0xbf3e, 2, 0x00u, 1, false, A);
        goto source_bf40_m1x1;
source_bf40_m1x1: /* $80:BF40 [M=1 X=1] LDA   $0100,Y */
        READ(0xbf40, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_bf43_m1x1;
source_bf43_m1x1: /* $80:BF43 [M=1 X=1] STA   $01 */
        WRITE(0xbf43, 2, 0x01u, 1, false, A);
        goto source_bf45_m1x1;
source_bf45_m1x1: /* $80:BF45 [M=1 X=1] INY */
        IMP(0xbf45);
        set_y(s, Y + 1u);
        goto source_bf46_m1x1;
source_bf46_m1x1: /* $80:BF46 [M=1 X=1] CMP   $25 */
        READ(0xbf46, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_bf48_m1x1;
source_bf48_m1x1: /* $80:BF48 [M=1 X=1] BNE   $BF53 */
        BRANCH(0xbf48, (P & 2u) == 0);
        if (B.branch) { goto source_bf53_m1x1; }
        goto source_bf4a_m1x1;
source_bf4a_m1x1: /* $80:BF4A [M=1 X=1] LDA   $2D */
        READ(0xbf4a, 2, 0x2du, 1, false);
        set_a(s, B.read_value);
        goto source_bf4c_m1x1;
source_bf4c_m1x1: /* $80:BF4C [M=1 X=1] INC   A */
        IMP(0xbf4c);
        accumulator_change(s, FB30_INC);
        goto source_bf4d_m1x1;
source_bf4d_m1x1: /* $80:BF4D [M=1 X=1] STA   $26 */
        WRITE(0xbf4d, 2, 0x26u, 1, false, A);
        goto source_bf4f_m1x1;
source_bf4f_m1x1: /* $80:BF4F [M=1 X=1] LDA   #$12 */
        IMM(0xbf4f, 2);
        set_a(s, 0x0012u);
        goto source_bf51_m1x1;
source_bf51_m1x1: /* $80:BF51 [M=1 X=1] STA   $00 */
        WRITE(0xbf51, 2, 0x00u, 1, false, A);
        goto source_bf53_m1x1;
source_bf53_m1x1: /* $80:BF53 [M=1 X=1] STY   $04 */
        WRITE(0xbf53, 2, 0x04u, 1, false, Y);
        goto source_bf55_m1x1;
source_bf55_m1x1: /* $80:BF55 [M=1 X=1] LDY   $2F */
        READ(0xbf55, 2, 0x2fu, 1, false);
        set_y(s, B.read_value);
        goto source_bf57_m1x1;
source_bf57_m1x1: /* $80:BF57 [M=1 X=1] BRA   $BF64 */
        BRANCH(0xbf57, true);
        goto source_bf64_m1x1;
source_bf59_m1x1: /* $80:BF59 [M=1 X=1] LDA   $01 */
        READ(0xbf59, 2, 0x01u, 1, false);
        set_a(s, B.read_value);
        goto source_bf5b_m1x1;
source_bf5b_m1x1: /* $80:BF5B [M=1 X=1] STA   $0300,X */
        WRITE(0xbf5b, 3, DB(0x0300u + X), 1, true, A);
        goto source_bf5e_m1x1;
source_bf5e_m1x1: /* $80:BF5E [M=1 X=1] LDA   $00 */
        READ(0xbf5e, 2, 0x00u, 1, false);
        set_a(s, B.read_value);
        goto source_bf60_m1x1;
source_bf60_m1x1: /* $80:BF60 [M=1 X=1] STA   $0400,X */
        WRITE(0xbf60, 3, DB(0x0400u + X), 1, true, A);
        goto source_bf63_m1x1;
source_bf63_m1x1: /* $80:BF63 [M=1 X=1] INX */
        IMP(0xbf63);
        set_x(s, X + 1u);
        goto source_bf64_m1x1;
source_bf64_m1x1: /* $80:BF64 [M=1 X=1] DEY */
        IMP(0xbf64);
        set_y(s, Y - 1u);
        goto source_bf65_m1x1;
source_bf65_m1x1: /* $80:BF65 [M=1 X=1] BNE   $BF59 */
        BRANCH(0xbf65, (P & 2u) == 0);
        if (B.branch) { goto source_bf59_m1x1; }
        goto source_bf67_m1x1;
source_bf67_m1x1: /* $80:BF67 [M=1 X=1] LDY   $04 */
        READ(0xbf67, 2, 0x04u, 1, false);
        set_y(s, B.read_value);
        goto source_bf69_m1x1;
source_bf69_m1x1: /* $80:BF69 [M=1 X=1] DEC   $29 */
        CHANGE(0xbf69, 2, 0x29u, 1, false, FB30_DEC);
        goto source_bf6b_m1x1;
source_bf6b_m1x1: /* $80:BF6B [M=1 X=1] BPL   $BF3B */
        BRANCH(0xbf6b, (P & 0x80u) == 0);
        if (B.branch) { goto source_bf3b_m1x1; }
        goto source_bf6d_m1x1;
source_bf6d_m1x1: /* $80:BF6D [M=1 X=1] INC   $2D */
        CHANGE(0xbf6d, 2, 0x2du, 1, false, FB30_INC);
        goto source_bf6f_m1x1;
source_bf6f_m1x1: /* $80:BF6F [M=1 X=1] LDA   $2B */
        READ(0xbf6f, 2, 0x2bu, 1, false);
        set_a(s, B.read_value);
        goto source_bf71_m1x1;
source_bf71_m1x1: /* $80:BF71 [M=1 X=1] CMP   $2D */
        READ(0xbf71, 2, 0x2du, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_bf73_m1x1;
source_bf73_m1x1: /* $80:BF73 [M=1 X=1] BCS   $BF18 */
        BRANCH(0xbf73, (P & 1u) != 0);
        if (B.branch) { goto source_bf18_m1x1; }
        goto source_bf75_m1x1;
source_bf75_m1x1: /* $80:BF75 [M=1 X=1] STX   $00 */
        WRITE(0xbf75, 2, 0x00u, 1, false, X);
        goto source_bf77_m1x1;
source_bf77_m1x1: /* $80:BF77 [M=1 X=1] LDX   $00 */
        READ(0xbf77, 2, 0x00u, 1, false);
        set_x(s, B.read_value);
        goto source_bf79_m1x1;
source_bf79_m1x1: /* $80:BF79 [M=1 X=1] BEQ   $BF83 */
        BRANCH(0xbf79, (P & 2u) != 0);
        if (B.branch) { goto source_bf83_m1x1; }
        goto source_bf7b_m1x1;
source_bf7b_m1x1: /* $80:BF7B [M=1 X=1] LDA   #$10 */
        IMM(0xbf7b, 2);
        set_a(s, 0x0010u);
        goto source_bf7d_m1x1;
source_bf7d_m1x1: /* $80:BF7D [M=1 X=1] STA   $0400,X */
        WRITE(0xbf7d, 3, DB(0x0400u + X), 1, true, A);
        goto source_bf80_m1x1;
source_bf80_m1x1: /* $80:BF80 [M=1 X=1] INX */
        IMP(0xbf80);
        set_x(s, X + 1u);
        goto source_bf81_m1x1;
source_bf81_m1x1: /* $80:BF81 [M=1 X=1] BNE   $BF7D */
        BRANCH(0xbf81, (P & 2u) == 0);
        if (B.branch) { goto source_bf7d_m1x1; }
        goto source_bf83_m1x1;
source_bf83_m1x1: /* $80:BF83 [M=1 X=1] LDA   $2B */
        READ(0xbf83, 2, 0x2bu, 1, false);
        set_a(s, B.read_value);
        goto source_bf85_m1x1;
source_bf85_m1x1: /* $80:BF85 [M=1 X=1] ASL   A */
        IMP(0xbf85);
        accumulator_change(s, FB30_ASL);
        goto source_bf86_m1x1;
source_bf86_m1x1: /* $80:BF86 [M=1 X=1] TAX */
        IMP(0xbf86);
        set_x(s, A);
        goto source_bf87_m1x1;
source_bf87_m1x1: /* $80:BF87 [M=1 X=1] REP   #$20 */
        MODE(0xbf87);
        status_set(s, P & 0xdfu);
        goto source_bf89_m0x1;
source_bf89_m0x1: /* $80:BF89 [M=0 X=1] LDA   $80BF91,X */
        READ(0xbf89, 4, (0x80bf91u + X), 2, false);
        set_a(s, B.read_value);
        goto source_bf8d_m0x1;
source_bf8d_m0x1: /* $80:BF8D [M=0 X=1] STA   $0560 */
        WRITE(0xbf8d, 3, DB(0x0560u), 2, false, A);
        goto source_bf90_m0x1;
source_bf90_m0x1: /* $80:BF90 [M=0 X=1] LDY   $12 */
        READ(0xbf90, 2, 0x12u, 1, false);
        set_y(s, B.read_value);
        goto source_bf92_m0x1;
source_bf92_m0x1: /* $80:BF92 [M=0 X=1] STY   $2183 */
        WRITE(0xbf92, 3, DB(0x2183u), 1, false, Y);
        goto source_bf95_m0x1;
source_bf95_m0x1: /* $80:BF95 [M=0 X=1] LDA   $10 */
        READ(0xbf95, 2, 0x10u, 2, false);
        set_a(s, B.read_value);
        goto source_bf97_m0x1;
source_bf97_m0x1: /* $80:BF97 [M=0 X=1] STA   $2181 */
        WRITE(0xbf97, 3, DB(0x2181u), 2, false, A);
        goto source_bf9a_m0x1;
source_bf9a_m0x1: /* $80:BF9A [M=0 X=1] LDA   $1E */
        READ(0xbf9a, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_bf9c_m0x1;
source_bf9c_m0x1: /* $80:BF9C [M=0 X=1] LDX   $23 */
        READ(0xbf9c, 2, 0x23u, 1, false);
        set_x(s, B.read_value);
        goto source_bf9e_m0x1;
source_bf9e_m0x1: /* $80:BF9E [M=0 X=1] JMP   ($BFB1,X) */
        READ(0xbf9e, 3, (0x800000u | (uint16_t)(0xbfb1u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfdb: goto source_bfdb_m0x1;
        case 0xc028: goto source_c028_m0x1;
        case 0xc076: goto source_c076_m0x1;
        case 0xc0c5: goto source_c0c5_m0x1;
        case 0xc115: goto source_c115_m0x1;
        case 0xc166: goto source_c166_m0x1;
        case 0xc1b8: goto source_c1b8_m0x1;
        case 0xc20b: goto source_c20b_m0x1;
        default: return unsupported(s);
        }
source_bfc3_m0x1: /* $80:BFC3 [M=0 X=1] ASL   A */
        IMP(0xbfc3);
        accumulator_change(s, FB30_ASL);
        goto source_bfc4_m0x1;
source_bfc4_m0x1: /* $80:BFC4 [M=0 X=1] ASL   A */
        IMP(0xbfc4);
        accumulator_change(s, FB30_ASL);
        goto source_bfc5_m0x1;
source_bfc5_m0x1: /* $80:BFC5 [M=0 X=1] ASL   A */
        IMP(0xbfc5);
        accumulator_change(s, FB30_ASL);
        goto source_bfc6_m0x1;
source_bfc6_m0x1: /* $80:BFC6 [M=0 X=1] ASL   A */
        IMP(0xbfc6);
        accumulator_change(s, FB30_ASL);
        goto source_bfc7_m0x1;
source_bfc7_m0x1: /* $80:BFC7 [M=0 X=1] ASL   A */
        IMP(0xbfc7);
        accumulator_change(s, FB30_ASL);
        goto source_bfc8_m0x1;
source_bfc8_m0x1: /* $80:BFC8 [M=0 X=1] ASL   A */
        IMP(0xbfc8);
        accumulator_change(s, FB30_ASL);
        goto source_bfc9_m0x1;
source_bfc9_m0x1: /* $80:BFC9 [M=0 X=1] ASL   A */
        IMP(0xbfc9);
        accumulator_change(s, FB30_ASL);
        goto source_bfca_m0x1;
source_bfca_m0x1: /* $80:BFCA [M=0 X=1] ASL   A */
        IMP(0xbfca);
        accumulator_change(s, FB30_ASL);
        goto source_bfcb_m0x1;
source_bfcb_m0x1: /* $80:BFCB [M=0 X=1] SEP   #$20 */
        MODE(0xbfcb);
        status_set(s, P | 0x20u);
        goto source_bfcd_m1x1;
source_bfcd_m1x1: /* $80:BFCD [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xbfcd, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_bfcf_m1x1;
source_bfcf_m1x1: /* $80:BFCF [M=1 X=1] REP   #$20 */
        MODE(0xbfcf);
        status_set(s, P & 0xdfu);
        goto source_bfd1_m0x1;
source_bfd1_m0x1: /* $80:BFD1 [M=0 X=1] INC   $0C */
        CHANGE(0xbfd1, 2, 0x0cu, 2, false, FB30_INC);
        goto source_bfd3_m0x1;
source_bfd3_m0x1: /* $80:BFD3 [M=0 X=1] LDX   $0300,Y */
        READ(0xbfd3, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_bfd6_m0x1;
source_bfd6_m0x1: /* $80:BFD6 [M=0 X=1] STX   $2180 */
        WRITE(0xbfd6, 3, DB(0x2180u), 1, false, X);
        goto source_bfd9_m0x1;
source_bfd9_m0x1: /* $80:BFD9 [M=0 X=1] STX   $08 */
        WRITE(0xbfd9, 2, 0x08u, 1, false, X);
        goto source_bfdb_m0x1;
source_bfdb_m0x1: /* $80:BFDB [M=0 X=1] STA   $1E */
        WRITE(0xbfdb, 2, 0x1eu, 2, false, A);
        goto source_bfdd_m0x1;
source_bfdd_m0x1: /* $80:BFDD [M=0 X=1] LDY   $1F */
        READ(0xbfdd, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_bfdf_m0x1;
source_bfdf_m0x1: /* $80:BFDF [M=0 X=1] LDX   $0400,Y */
        READ(0xbfdf, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_bfe2_m0x1;
source_bfe2_m0x1: /* $80:BFE2 [M=0 X=1] JMP   ($BFE5,X) */
        READ(0xbfe2, 3, (0x800000u | (uint16_t)(0xbfe5u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfc3: goto source_bfc3_m0x1;
        case 0xbff9: goto source_bff9_m0x1;
        case 0xbffe: goto source_bffe_m0x1;
        case 0xc01f: goto source_c01f_m0x1;
        case 0xc06c: goto source_c06c_m0x1;
        case 0xc0ba: goto source_c0ba_m0x1;
        case 0xc109: goto source_c109_m0x1;
        case 0xc159: goto source_c159_m0x1;
        case 0xc1aa: goto source_c1aa_m0x1;
        case 0xc1fc: goto source_c1fc_m0x1;
        default: return unsupported(s);
        }
source_bff9_m0x1: /* $80:BFF9 [M=0 X=1] LDX   #$10 */
        IMM(0xbff9, 2);
        set_x(s, 0x0010u);
        goto source_bffb_m0x1;
source_bffb_m0x1: /* $80:BFFB [M=0 X=1] JMP   $C434 */
        JUMP(0xbffb);
        goto source_c434_m0x1;
source_bffe_m0x1: /* $80:BFFE [M=0 X=1] LDX   #$10 */
        IMM(0xbffe, 2);
        set_x(s, 0x0010u);
        goto source_c000_m0x1;
source_c000_m0x1: /* $80:C000 [M=0 X=1] STZ   $1C */
        WRITE(0xc000, 2, 0x1cu, 2, false, 0);
        goto source_c002_m0x1;
source_c002_m0x1: /* $80:C002 [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc002, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c004_m0x1;
source_c004_m0x1: /* $80:C004 [M=0 X=1] AND   #$FF */
        IMM(0xc004, 3);
        set_a(s, A & 0x00ffu);
        goto source_c007_m0x1;
source_c007_m0x1: /* $80:C007 [M=0 X=1] ORA   $1D */
        READ(0xc007, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c009_m0x1;
source_c009_m0x1: /* $80:C009 [M=0 X=1] STA   $1D */
        WRITE(0xc009, 2, 0x1du, 2, false, A);
        goto source_c00b_m0x1;
source_c00b_m0x1: /* $80:C00B [M=0 X=1] LDA   $1E */
        READ(0xc00b, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c00d_m0x1;
source_c00d_m0x1: /* $80:C00D [M=0 X=1] JMP   ($0560) */
        READ(0xc00d, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c010_m0x1: /* $80:C010 [M=0 X=1] ASL   A */
        IMP(0xc010);
        accumulator_change(s, FB30_ASL);
        goto source_c011_m0x1;
source_c011_m0x1: /* $80:C011 [M=0 X=1] ASL   A */
        IMP(0xc011);
        accumulator_change(s, FB30_ASL);
        goto source_c012_m0x1;
source_c012_m0x1: /* $80:C012 [M=0 X=1] ASL   A */
        IMP(0xc012);
        accumulator_change(s, FB30_ASL);
        goto source_c013_m0x1;
source_c013_m0x1: /* $80:C013 [M=0 X=1] ASL   A */
        IMP(0xc013);
        accumulator_change(s, FB30_ASL);
        goto source_c014_m0x1;
source_c014_m0x1: /* $80:C014 [M=0 X=1] ASL   A */
        IMP(0xc014);
        accumulator_change(s, FB30_ASL);
        goto source_c015_m0x1;
source_c015_m0x1: /* $80:C015 [M=0 X=1] ASL   A */
        IMP(0xc015);
        accumulator_change(s, FB30_ASL);
        goto source_c016_m0x1;
source_c016_m0x1: /* $80:C016 [M=0 X=1] ASL   A */
        IMP(0xc016);
        accumulator_change(s, FB30_ASL);
        goto source_c017_m0x1;
source_c017_m0x1: /* $80:C017 [M=0 X=1] SEP   #$20 */
        MODE(0xc017);
        status_set(s, P | 0x20u);
        goto source_c019_m1x1;
source_c019_m1x1: /* $80:C019 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc019, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c01b_m1x1;
source_c01b_m1x1: /* $80:C01B [M=1 X=1] REP   #$20 */
        MODE(0xc01b);
        status_set(s, P & 0xdfu);
        goto source_c01d_m0x1;
source_c01d_m0x1: /* $80:C01D [M=0 X=1] INC   $0C */
        CHANGE(0xc01d, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c01f_m0x1;
source_c01f_m0x1: /* $80:C01F [M=0 X=1] ASL   A */
        IMP(0xc01f);
        accumulator_change(s, FB30_ASL);
        goto source_c020_m0x1;
source_c020_m0x1: /* $80:C020 [M=0 X=1] LDX   $0300,Y */
        READ(0xc020, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_c023_m0x1;
source_c023_m0x1: /* $80:C023 [M=0 X=1] STX   $2180 */
        WRITE(0xc023, 3, DB(0x2180u), 1, false, X);
        goto source_c026_m0x1;
source_c026_m0x1: /* $80:C026 [M=0 X=1] STX   $08 */
        WRITE(0xc026, 2, 0x08u, 1, false, X);
        goto source_c028_m0x1;
source_c028_m0x1: /* $80:C028 [M=0 X=1] STA   $1E */
        WRITE(0xc028, 2, 0x1eu, 2, false, A);
        goto source_c02a_m0x1;
source_c02a_m0x1: /* $80:C02A [M=0 X=1] LDY   $1F */
        READ(0xc02a, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c02c_m0x1;
source_c02c_m0x1: /* $80:C02C [M=0 X=1] LDX   $0400,Y */
        READ(0xc02c, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_c02f_m0x1;
source_c02f_m0x1: /* $80:C02F [M=0 X=1] JMP   ($C032,X) */
        READ(0xc02f, 3, (0x800000u | (uint16_t)(0xc032u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfc4: goto source_bfc4_m0x1;
        case 0xc010: goto source_c010_m0x1;
        case 0xc046: goto source_c046_m0x1;
        case 0xc04b: goto source_c04b_m0x1;
        case 0xc06d: goto source_c06d_m0x1;
        case 0xc0bb: goto source_c0bb_m0x1;
        case 0xc10a: goto source_c10a_m0x1;
        case 0xc15a: goto source_c15a_m0x1;
        case 0xc1ab: goto source_c1ab_m0x1;
        case 0xc1fd: goto source_c1fd_m0x1;
        default: return unsupported(s);
        }
source_c046_m0x1: /* $80:C046 [M=0 X=1] LDX   #$0E */
        IMM(0xc046, 2);
        set_x(s, 0x000eu);
        goto source_c048_m0x1;
source_c048_m0x1: /* $80:C048 [M=0 X=1] JMP   $C434 */
        JUMP(0xc048);
        goto source_c434_m0x1;
source_c04b_m0x1: /* $80:C04B [M=0 X=1] LDX   #$0E */
        IMM(0xc04b, 2);
        set_x(s, 0x000eu);
        goto source_c04d_m0x1;
source_c04d_m0x1: /* $80:C04D [M=0 X=1] STZ   $1C */
        WRITE(0xc04d, 2, 0x1cu, 2, false, 0);
        goto source_c04f_m0x1;
source_c04f_m0x1: /* $80:C04F [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc04f, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c051_m0x1;
source_c051_m0x1: /* $80:C051 [M=0 X=1] AND   #$FF */
        IMM(0xc051, 3);
        set_a(s, A & 0x00ffu);
        goto source_c054_m0x1;
source_c054_m0x1: /* $80:C054 [M=0 X=1] ASL   A */
        IMP(0xc054);
        accumulator_change(s, FB30_ASL);
        goto source_c055_m0x1;
source_c055_m0x1: /* $80:C055 [M=0 X=1] ORA   $1D */
        READ(0xc055, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c057_m0x1;
source_c057_m0x1: /* $80:C057 [M=0 X=1] STA   $1D */
        WRITE(0xc057, 2, 0x1du, 2, false, A);
        goto source_c059_m0x1;
source_c059_m0x1: /* $80:C059 [M=0 X=1] LDA   $1E */
        READ(0xc059, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c05b_m0x1;
source_c05b_m0x1: /* $80:C05B [M=0 X=1] JMP   ($0560) */
        READ(0xc05b, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c05e_m0x1: /* $80:C05E [M=0 X=1] ASL   A */
        IMP(0xc05e);
        accumulator_change(s, FB30_ASL);
        goto source_c05f_m0x1;
source_c05f_m0x1: /* $80:C05F [M=0 X=1] ASL   A */
        IMP(0xc05f);
        accumulator_change(s, FB30_ASL);
        goto source_c060_m0x1;
source_c060_m0x1: /* $80:C060 [M=0 X=1] ASL   A */
        IMP(0xc060);
        accumulator_change(s, FB30_ASL);
        goto source_c061_m0x1;
source_c061_m0x1: /* $80:C061 [M=0 X=1] ASL   A */
        IMP(0xc061);
        accumulator_change(s, FB30_ASL);
        goto source_c062_m0x1;
source_c062_m0x1: /* $80:C062 [M=0 X=1] ASL   A */
        IMP(0xc062);
        accumulator_change(s, FB30_ASL);
        goto source_c063_m0x1;
source_c063_m0x1: /* $80:C063 [M=0 X=1] ASL   A */
        IMP(0xc063);
        accumulator_change(s, FB30_ASL);
        goto source_c064_m0x1;
source_c064_m0x1: /* $80:C064 [M=0 X=1] SEP   #$20 */
        MODE(0xc064);
        status_set(s, P | 0x20u);
        goto source_c066_m1x1;
source_c066_m1x1: /* $80:C066 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc066, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c068_m1x1;
source_c068_m1x1: /* $80:C068 [M=1 X=1] REP   #$20 */
        MODE(0xc068);
        status_set(s, P & 0xdfu);
        goto source_c06a_m0x1;
source_c06a_m0x1: /* $80:C06A [M=0 X=1] INC   $0C */
        CHANGE(0xc06a, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c06c_m0x1;
source_c06c_m0x1: /* $80:C06C [M=0 X=1] ASL   A */
        IMP(0xc06c);
        accumulator_change(s, FB30_ASL);
        goto source_c06d_m0x1;
source_c06d_m0x1: /* $80:C06D [M=0 X=1] ASL   A */
        IMP(0xc06d);
        accumulator_change(s, FB30_ASL);
        goto source_c06e_m0x1;
source_c06e_m0x1: /* $80:C06E [M=0 X=1] LDX   $0300,Y */
        READ(0xc06e, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_c071_m0x1;
source_c071_m0x1: /* $80:C071 [M=0 X=1] STX   $2180 */
        WRITE(0xc071, 3, DB(0x2180u), 1, false, X);
        goto source_c074_m0x1;
source_c074_m0x1: /* $80:C074 [M=0 X=1] STX   $08 */
        WRITE(0xc074, 2, 0x08u, 1, false, X);
        goto source_c076_m0x1;
source_c076_m0x1: /* $80:C076 [M=0 X=1] STA   $1E */
        WRITE(0xc076, 2, 0x1eu, 2, false, A);
        goto source_c078_m0x1;
source_c078_m0x1: /* $80:C078 [M=0 X=1] LDY   $1F */
        READ(0xc078, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c07a_m0x1;
source_c07a_m0x1: /* $80:C07A [M=0 X=1] LDX   $0400,Y */
        READ(0xc07a, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_c07d_m0x1;
source_c07d_m0x1: /* $80:C07D [M=0 X=1] JMP   ($C080,X) */
        READ(0xc07d, 3, (0x800000u | (uint16_t)(0xc080u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfc5: goto source_bfc5_m0x1;
        case 0xc011: goto source_c011_m0x1;
        case 0xc05e: goto source_c05e_m0x1;
        case 0xc094: goto source_c094_m0x1;
        case 0xc099: goto source_c099_m0x1;
        case 0xc0bc: goto source_c0bc_m0x1;
        case 0xc10b: goto source_c10b_m0x1;
        case 0xc15b: goto source_c15b_m0x1;
        case 0xc1ac: goto source_c1ac_m0x1;
        case 0xc1fe: goto source_c1fe_m0x1;
        default: return unsupported(s);
        }
source_c094_m0x1: /* $80:C094 [M=0 X=1] LDX   #$0C */
        IMM(0xc094, 2);
        set_x(s, 0x000cu);
        goto source_c096_m0x1;
source_c096_m0x1: /* $80:C096 [M=0 X=1] JMP   $C434 */
        JUMP(0xc096);
        goto source_c434_m0x1;
source_c099_m0x1: /* $80:C099 [M=0 X=1] LDX   #$0C */
        IMM(0xc099, 2);
        set_x(s, 0x000cu);
        goto source_c09b_m0x1;
source_c09b_m0x1: /* $80:C09B [M=0 X=1] STZ   $1C */
        WRITE(0xc09b, 2, 0x1cu, 2, false, 0);
        goto source_c09d_m0x1;
source_c09d_m0x1: /* $80:C09D [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc09d, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c09f_m0x1;
source_c09f_m0x1: /* $80:C09F [M=0 X=1] AND   #$FF */
        IMM(0xc09f, 3);
        set_a(s, A & 0x00ffu);
        goto source_c0a2_m0x1;
source_c0a2_m0x1: /* $80:C0A2 [M=0 X=1] ASL   A */
        IMP(0xc0a2);
        accumulator_change(s, FB30_ASL);
        goto source_c0a3_m0x1;
source_c0a3_m0x1: /* $80:C0A3 [M=0 X=1] ASL   A */
        IMP(0xc0a3);
        accumulator_change(s, FB30_ASL);
        goto source_c0a4_m0x1;
source_c0a4_m0x1: /* $80:C0A4 [M=0 X=1] ORA   $1D */
        READ(0xc0a4, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c0a6_m0x1;
source_c0a6_m0x1: /* $80:C0A6 [M=0 X=1] STA   $1D */
        WRITE(0xc0a6, 2, 0x1du, 2, false, A);
        goto source_c0a8_m0x1;
source_c0a8_m0x1: /* $80:C0A8 [M=0 X=1] LDA   $1E */
        READ(0xc0a8, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c0aa_m0x1;
source_c0aa_m0x1: /* $80:C0AA [M=0 X=1] JMP   ($0560) */
        READ(0xc0aa, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c0ad_m0x1: /* $80:C0AD [M=0 X=1] ASL   A */
        IMP(0xc0ad);
        accumulator_change(s, FB30_ASL);
        goto source_c0ae_m0x1;
source_c0ae_m0x1: /* $80:C0AE [M=0 X=1] ASL   A */
        IMP(0xc0ae);
        accumulator_change(s, FB30_ASL);
        goto source_c0af_m0x1;
source_c0af_m0x1: /* $80:C0AF [M=0 X=1] ASL   A */
        IMP(0xc0af);
        accumulator_change(s, FB30_ASL);
        goto source_c0b0_m0x1;
source_c0b0_m0x1: /* $80:C0B0 [M=0 X=1] ASL   A */
        IMP(0xc0b0);
        accumulator_change(s, FB30_ASL);
        goto source_c0b1_m0x1;
source_c0b1_m0x1: /* $80:C0B1 [M=0 X=1] ASL   A */
        IMP(0xc0b1);
        accumulator_change(s, FB30_ASL);
        goto source_c0b2_m0x1;
source_c0b2_m0x1: /* $80:C0B2 [M=0 X=1] SEP   #$20 */
        MODE(0xc0b2);
        status_set(s, P | 0x20u);
        goto source_c0b4_m1x1;
source_c0b4_m1x1: /* $80:C0B4 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc0b4, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c0b6_m1x1;
source_c0b6_m1x1: /* $80:C0B6 [M=1 X=1] REP   #$20 */
        MODE(0xc0b6);
        status_set(s, P & 0xdfu);
        goto source_c0b8_m0x1;
source_c0b8_m0x1: /* $80:C0B8 [M=0 X=1] INC   $0C */
        CHANGE(0xc0b8, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c0ba_m0x1;
source_c0ba_m0x1: /* $80:C0BA [M=0 X=1] ASL   A */
        IMP(0xc0ba);
        accumulator_change(s, FB30_ASL);
        goto source_c0bb_m0x1;
source_c0bb_m0x1: /* $80:C0BB [M=0 X=1] ASL   A */
        IMP(0xc0bb);
        accumulator_change(s, FB30_ASL);
        goto source_c0bc_m0x1;
source_c0bc_m0x1: /* $80:C0BC [M=0 X=1] ASL   A */
        IMP(0xc0bc);
        accumulator_change(s, FB30_ASL);
        goto source_c0bd_m0x1;
source_c0bd_m0x1: /* $80:C0BD [M=0 X=1] LDX   $0300,Y */
        READ(0xc0bd, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_c0c0_m0x1;
source_c0c0_m0x1: /* $80:C0C0 [M=0 X=1] STX   $2180 */
        WRITE(0xc0c0, 3, DB(0x2180u), 1, false, X);
        goto source_c0c3_m0x1;
source_c0c3_m0x1: /* $80:C0C3 [M=0 X=1] STX   $08 */
        WRITE(0xc0c3, 2, 0x08u, 1, false, X);
        goto source_c0c5_m0x1;
source_c0c5_m0x1: /* $80:C0C5 [M=0 X=1] STA   $1E */
        WRITE(0xc0c5, 2, 0x1eu, 2, false, A);
        goto source_c0c7_m0x1;
source_c0c7_m0x1: /* $80:C0C7 [M=0 X=1] LDY   $1F */
        READ(0xc0c7, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c0c9_m0x1;
source_c0c9_m0x1: /* $80:C0C9 [M=0 X=1] LDX   $0400,Y */
        READ(0xc0c9, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_c0cc_m0x1;
source_c0cc_m0x1: /* $80:C0CC [M=0 X=1] JMP   ($C0CF,X) */
        READ(0xc0cc, 3, (0x800000u | (uint16_t)(0xc0cfu + X)), 2, true);
        switch (B.read_value) {
        case 0xbfc6: goto source_bfc6_m0x1;
        case 0xc012: goto source_c012_m0x1;
        case 0xc05f: goto source_c05f_m0x1;
        case 0xc0ad: goto source_c0ad_m0x1;
        case 0xc0e3: goto source_c0e3_m0x1;
        case 0xc0e8: goto source_c0e8_m0x1;
        case 0xc10c: goto source_c10c_m0x1;
        case 0xc15c: goto source_c15c_m0x1;
        case 0xc1ad: goto source_c1ad_m0x1;
        case 0xc1ff: goto source_c1ff_m0x1;
        default: return unsupported(s);
        }
source_c0e3_m0x1: /* $80:C0E3 [M=0 X=1] LDX   #$0A */
        IMM(0xc0e3, 2);
        set_x(s, 0x000au);
        goto source_c0e5_m0x1;
source_c0e5_m0x1: /* $80:C0E5 [M=0 X=1] JMP   $C434 */
        JUMP(0xc0e5);
        goto source_c434_m0x1;
source_c0e8_m0x1: /* $80:C0E8 [M=0 X=1] LDX   #$0A */
        IMM(0xc0e8, 2);
        set_x(s, 0x000au);
        goto source_c0ea_m0x1;
source_c0ea_m0x1: /* $80:C0EA [M=0 X=1] STZ   $1C */
        WRITE(0xc0ea, 2, 0x1cu, 2, false, 0);
        goto source_c0ec_m0x1;
source_c0ec_m0x1: /* $80:C0EC [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc0ec, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c0ee_m0x1;
source_c0ee_m0x1: /* $80:C0EE [M=0 X=1] AND   #$FF */
        IMM(0xc0ee, 3);
        set_a(s, A & 0x00ffu);
        goto source_c0f1_m0x1;
source_c0f1_m0x1: /* $80:C0F1 [M=0 X=1] ASL   A */
        IMP(0xc0f1);
        accumulator_change(s, FB30_ASL);
        goto source_c0f2_m0x1;
source_c0f2_m0x1: /* $80:C0F2 [M=0 X=1] ASL   A */
        IMP(0xc0f2);
        accumulator_change(s, FB30_ASL);
        goto source_c0f3_m0x1;
source_c0f3_m0x1: /* $80:C0F3 [M=0 X=1] ASL   A */
        IMP(0xc0f3);
        accumulator_change(s, FB30_ASL);
        goto source_c0f4_m0x1;
source_c0f4_m0x1: /* $80:C0F4 [M=0 X=1] ORA   $1D */
        READ(0xc0f4, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c0f6_m0x1;
source_c0f6_m0x1: /* $80:C0F6 [M=0 X=1] STA   $1D */
        WRITE(0xc0f6, 2, 0x1du, 2, false, A);
        goto source_c0f8_m0x1;
source_c0f8_m0x1: /* $80:C0F8 [M=0 X=1] LDA   $1E */
        READ(0xc0f8, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c0fa_m0x1;
source_c0fa_m0x1: /* $80:C0FA [M=0 X=1] JMP   ($0560) */
        READ(0xc0fa, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c0fd_m0x1: /* $80:C0FD [M=0 X=1] ASL   A */
        IMP(0xc0fd);
        accumulator_change(s, FB30_ASL);
        goto source_c0fe_m0x1;
source_c0fe_m0x1: /* $80:C0FE [M=0 X=1] ASL   A */
        IMP(0xc0fe);
        accumulator_change(s, FB30_ASL);
        goto source_c0ff_m0x1;
source_c0ff_m0x1: /* $80:C0FF [M=0 X=1] ASL   A */
        IMP(0xc0ff);
        accumulator_change(s, FB30_ASL);
        goto source_c100_m0x1;
source_c100_m0x1: /* $80:C100 [M=0 X=1] ASL   A */
        IMP(0xc100);
        accumulator_change(s, FB30_ASL);
        goto source_c101_m0x1;
source_c101_m0x1: /* $80:C101 [M=0 X=1] SEP   #$20 */
        MODE(0xc101);
        status_set(s, P | 0x20u);
        goto source_c103_m1x1;
source_c103_m1x1: /* $80:C103 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc103, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c105_m1x1;
source_c105_m1x1: /* $80:C105 [M=1 X=1] REP   #$20 */
        MODE(0xc105);
        status_set(s, P & 0xdfu);
        goto source_c107_m0x1;
source_c107_m0x1: /* $80:C107 [M=0 X=1] INC   $0C */
        CHANGE(0xc107, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c109_m0x1;
source_c109_m0x1: /* $80:C109 [M=0 X=1] ASL   A */
        IMP(0xc109);
        accumulator_change(s, FB30_ASL);
        goto source_c10a_m0x1;
source_c10a_m0x1: /* $80:C10A [M=0 X=1] ASL   A */
        IMP(0xc10a);
        accumulator_change(s, FB30_ASL);
        goto source_c10b_m0x1;
source_c10b_m0x1: /* $80:C10B [M=0 X=1] ASL   A */
        IMP(0xc10b);
        accumulator_change(s, FB30_ASL);
        goto source_c10c_m0x1;
source_c10c_m0x1: /* $80:C10C [M=0 X=1] ASL   A */
        IMP(0xc10c);
        accumulator_change(s, FB30_ASL);
        goto source_c10d_m0x1;
source_c10d_m0x1: /* $80:C10D [M=0 X=1] LDX   $0300,Y */
        READ(0xc10d, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_c110_m0x1;
source_c110_m0x1: /* $80:C110 [M=0 X=1] STX   $2180 */
        WRITE(0xc110, 3, DB(0x2180u), 1, false, X);
        goto source_c113_m0x1;
source_c113_m0x1: /* $80:C113 [M=0 X=1] STX   $08 */
        WRITE(0xc113, 2, 0x08u, 1, false, X);
        goto source_c115_m0x1;
source_c115_m0x1: /* $80:C115 [M=0 X=1] STA   $1E */
        WRITE(0xc115, 2, 0x1eu, 2, false, A);
        goto source_c117_m0x1;
source_c117_m0x1: /* $80:C117 [M=0 X=1] LDY   $1F */
        READ(0xc117, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c119_m0x1;
source_c119_m0x1: /* $80:C119 [M=0 X=1] LDX   $0400,Y */
        READ(0xc119, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_c11c_m0x1;
source_c11c_m0x1: /* $80:C11C [M=0 X=1] JMP   ($C11F,X) */
        READ(0xc11c, 3, (0x800000u | (uint16_t)(0xc11fu + X)), 2, true);
        switch (B.read_value) {
        case 0xbfc7: goto source_bfc7_m0x1;
        case 0xc013: goto source_c013_m0x1;
        case 0xc060: goto source_c060_m0x1;
        case 0xc0ae: goto source_c0ae_m0x1;
        case 0xc0fd: goto source_c0fd_m0x1;
        case 0xc133: goto source_c133_m0x1;
        case 0xc138: goto source_c138_m0x1;
        case 0xc15d: goto source_c15d_m0x1;
        case 0xc1ae: goto source_c1ae_m0x1;
        case 0xc200: goto source_c200_m0x1;
        default: return unsupported(s);
        }
source_c133_m0x1: /* $80:C133 [M=0 X=1] LDX   #$08 */
        IMM(0xc133, 2);
        set_x(s, 0x0008u);
        goto source_c135_m0x1;
source_c135_m0x1: /* $80:C135 [M=0 X=1] JMP   $C434 */
        JUMP(0xc135);
        goto source_c434_m0x1;
source_c138_m0x1: /* $80:C138 [M=0 X=1] LDX   #$08 */
        IMM(0xc138, 2);
        set_x(s, 0x0008u);
        goto source_c13a_m0x1;
source_c13a_m0x1: /* $80:C13A [M=0 X=1] STZ   $1C */
        WRITE(0xc13a, 2, 0x1cu, 2, false, 0);
        goto source_c13c_m0x1;
source_c13c_m0x1: /* $80:C13C [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc13c, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c13e_m0x1;
source_c13e_m0x1: /* $80:C13E [M=0 X=1] AND   #$FF */
        IMM(0xc13e, 3);
        set_a(s, A & 0x00ffu);
        goto source_c141_m0x1;
source_c141_m0x1: /* $80:C141 [M=0 X=1] ASL   A */
        IMP(0xc141);
        accumulator_change(s, FB30_ASL);
        goto source_c142_m0x1;
source_c142_m0x1: /* $80:C142 [M=0 X=1] ASL   A */
        IMP(0xc142);
        accumulator_change(s, FB30_ASL);
        goto source_c143_m0x1;
source_c143_m0x1: /* $80:C143 [M=0 X=1] ASL   A */
        IMP(0xc143);
        accumulator_change(s, FB30_ASL);
        goto source_c144_m0x1;
source_c144_m0x1: /* $80:C144 [M=0 X=1] ASL   A */
        IMP(0xc144);
        accumulator_change(s, FB30_ASL);
        goto source_c145_m0x1;
source_c145_m0x1: /* $80:C145 [M=0 X=1] ORA   $1D */
        READ(0xc145, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c147_m0x1;
source_c147_m0x1: /* $80:C147 [M=0 X=1] STA   $1D */
        WRITE(0xc147, 2, 0x1du, 2, false, A);
        goto source_c149_m0x1;
source_c149_m0x1: /* $80:C149 [M=0 X=1] LDA   $1E */
        READ(0xc149, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c14b_m0x1;
source_c14b_m0x1: /* $80:C14B [M=0 X=1] JMP   ($0560) */
        READ(0xc14b, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c14e_m0x1: /* $80:C14E [M=0 X=1] ASL   A */
        IMP(0xc14e);
        accumulator_change(s, FB30_ASL);
        goto source_c14f_m0x1;
source_c14f_m0x1: /* $80:C14F [M=0 X=1] ASL   A */
        IMP(0xc14f);
        accumulator_change(s, FB30_ASL);
        goto source_c150_m0x1;
source_c150_m0x1: /* $80:C150 [M=0 X=1] ASL   A */
        IMP(0xc150);
        accumulator_change(s, FB30_ASL);
        goto source_c151_m0x1;
source_c151_m0x1: /* $80:C151 [M=0 X=1] SEP   #$20 */
        MODE(0xc151);
        status_set(s, P | 0x20u);
        goto source_c153_m1x1;
source_c153_m1x1: /* $80:C153 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc153, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c155_m1x1;
source_c155_m1x1: /* $80:C155 [M=1 X=1] REP   #$20 */
        MODE(0xc155);
        status_set(s, P & 0xdfu);
        goto source_c157_m0x1;
source_c157_m0x1: /* $80:C157 [M=0 X=1] INC   $0C */
        CHANGE(0xc157, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c159_m0x1;
source_c159_m0x1: /* $80:C159 [M=0 X=1] ASL   A */
        IMP(0xc159);
        accumulator_change(s, FB30_ASL);
        goto source_c15a_m0x1;
source_c15a_m0x1: /* $80:C15A [M=0 X=1] ASL   A */
        IMP(0xc15a);
        accumulator_change(s, FB30_ASL);
        goto source_c15b_m0x1;
source_c15b_m0x1: /* $80:C15B [M=0 X=1] ASL   A */
        IMP(0xc15b);
        accumulator_change(s, FB30_ASL);
        goto source_c15c_m0x1;
source_c15c_m0x1: /* $80:C15C [M=0 X=1] ASL   A */
        IMP(0xc15c);
        accumulator_change(s, FB30_ASL);
        goto source_c15d_m0x1;
source_c15d_m0x1: /* $80:C15D [M=0 X=1] ASL   A */
        IMP(0xc15d);
        accumulator_change(s, FB30_ASL);
        goto source_c15e_m0x1;
source_c15e_m0x1: /* $80:C15E [M=0 X=1] LDX   $0300,Y */
        READ(0xc15e, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_c161_m0x1;
source_c161_m0x1: /* $80:C161 [M=0 X=1] STX   $2180 */
        WRITE(0xc161, 3, DB(0x2180u), 1, false, X);
        goto source_c164_m0x1;
source_c164_m0x1: /* $80:C164 [M=0 X=1] STX   $08 */
        WRITE(0xc164, 2, 0x08u, 1, false, X);
        goto source_c166_m0x1;
source_c166_m0x1: /* $80:C166 [M=0 X=1] STA   $1E */
        WRITE(0xc166, 2, 0x1eu, 2, false, A);
        goto source_c168_m0x1;
source_c168_m0x1: /* $80:C168 [M=0 X=1] LDY   $1F */
        READ(0xc168, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c16a_m0x1;
source_c16a_m0x1: /* $80:C16A [M=0 X=1] LDX   $0400,Y */
        READ(0xc16a, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_c16d_m0x1;
source_c16d_m0x1: /* $80:C16D [M=0 X=1] JMP   ($C170,X) */
        READ(0xc16d, 3, (0x800000u | (uint16_t)(0xc170u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfc8: goto source_bfc8_m0x1;
        case 0xc014: goto source_c014_m0x1;
        case 0xc061: goto source_c061_m0x1;
        case 0xc0af: goto source_c0af_m0x1;
        case 0xc0fe: goto source_c0fe_m0x1;
        case 0xc14e: goto source_c14e_m0x1;
        case 0xc184: goto source_c184_m0x1;
        case 0xc189: goto source_c189_m0x1;
        case 0xc1af: goto source_c1af_m0x1;
        case 0xc201: goto source_c201_m0x1;
        default: return unsupported(s);
        }
source_c184_m0x1: /* $80:C184 [M=0 X=1] LDX   #$06 */
        IMM(0xc184, 2);
        set_x(s, 0x0006u);
        goto source_c186_m0x1;
source_c186_m0x1: /* $80:C186 [M=0 X=1] JMP   $C434 */
        JUMP(0xc186);
        goto source_c434_m0x1;
source_c189_m0x1: /* $80:C189 [M=0 X=1] LDX   #$06 */
        IMM(0xc189, 2);
        set_x(s, 0x0006u);
        goto source_c18b_m0x1;
source_c18b_m0x1: /* $80:C18B [M=0 X=1] STZ   $1C */
        WRITE(0xc18b, 2, 0x1cu, 2, false, 0);
        goto source_c18d_m0x1;
source_c18d_m0x1: /* $80:C18D [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc18d, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c18f_m0x1;
source_c18f_m0x1: /* $80:C18F [M=0 X=1] AND   #$FF */
        IMM(0xc18f, 3);
        set_a(s, A & 0x00ffu);
        goto source_c192_m0x1;
source_c192_m0x1: /* $80:C192 [M=0 X=1] ASL   A */
        IMP(0xc192);
        accumulator_change(s, FB30_ASL);
        goto source_c193_m0x1;
source_c193_m0x1: /* $80:C193 [M=0 X=1] ASL   A */
        IMP(0xc193);
        accumulator_change(s, FB30_ASL);
        goto source_c194_m0x1;
source_c194_m0x1: /* $80:C194 [M=0 X=1] ASL   A */
        IMP(0xc194);
        accumulator_change(s, FB30_ASL);
        goto source_c195_m0x1;
source_c195_m0x1: /* $80:C195 [M=0 X=1] ASL   A */
        IMP(0xc195);
        accumulator_change(s, FB30_ASL);
        goto source_c196_m0x1;
source_c196_m0x1: /* $80:C196 [M=0 X=1] ASL   A */
        IMP(0xc196);
        accumulator_change(s, FB30_ASL);
        goto source_c197_m0x1;
source_c197_m0x1: /* $80:C197 [M=0 X=1] ORA   $1D */
        READ(0xc197, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c199_m0x1;
source_c199_m0x1: /* $80:C199 [M=0 X=1] STA   $1D */
        WRITE(0xc199, 2, 0x1du, 2, false, A);
        goto source_c19b_m0x1;
source_c19b_m0x1: /* $80:C19B [M=0 X=1] LDA   $1E */
        READ(0xc19b, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c19d_m0x1;
source_c19d_m0x1: /* $80:C19D [M=0 X=1] JMP   ($0560) */
        READ(0xc19d, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c1a0_m0x1: /* $80:C1A0 [M=0 X=1] ASL   A */
        IMP(0xc1a0);
        accumulator_change(s, FB30_ASL);
        goto source_c1a1_m0x1;
source_c1a1_m0x1: /* $80:C1A1 [M=0 X=1] ASL   A */
        IMP(0xc1a1);
        accumulator_change(s, FB30_ASL);
        goto source_c1a2_m0x1;
source_c1a2_m0x1: /* $80:C1A2 [M=0 X=1] SEP   #$20 */
        MODE(0xc1a2);
        status_set(s, P | 0x20u);
        goto source_c1a4_m1x1;
source_c1a4_m1x1: /* $80:C1A4 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc1a4, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c1a6_m1x1;
source_c1a6_m1x1: /* $80:C1A6 [M=1 X=1] REP   #$20 */
        MODE(0xc1a6);
        status_set(s, P & 0xdfu);
        goto source_c1a8_m0x1;
source_c1a8_m0x1: /* $80:C1A8 [M=0 X=1] INC   $0C */
        CHANGE(0xc1a8, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c1aa_m0x1;
source_c1aa_m0x1: /* $80:C1AA [M=0 X=1] ASL   A */
        IMP(0xc1aa);
        accumulator_change(s, FB30_ASL);
        goto source_c1ab_m0x1;
source_c1ab_m0x1: /* $80:C1AB [M=0 X=1] ASL   A */
        IMP(0xc1ab);
        accumulator_change(s, FB30_ASL);
        goto source_c1ac_m0x1;
source_c1ac_m0x1: /* $80:C1AC [M=0 X=1] ASL   A */
        IMP(0xc1ac);
        accumulator_change(s, FB30_ASL);
        goto source_c1ad_m0x1;
source_c1ad_m0x1: /* $80:C1AD [M=0 X=1] ASL   A */
        IMP(0xc1ad);
        accumulator_change(s, FB30_ASL);
        goto source_c1ae_m0x1;
source_c1ae_m0x1: /* $80:C1AE [M=0 X=1] ASL   A */
        IMP(0xc1ae);
        accumulator_change(s, FB30_ASL);
        goto source_c1af_m0x1;
source_c1af_m0x1: /* $80:C1AF [M=0 X=1] ASL   A */
        IMP(0xc1af);
        accumulator_change(s, FB30_ASL);
        goto source_c1b0_m0x1;
source_c1b0_m0x1: /* $80:C1B0 [M=0 X=1] LDX   $0300,Y */
        READ(0xc1b0, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_c1b3_m0x1;
source_c1b3_m0x1: /* $80:C1B3 [M=0 X=1] STX   $2180 */
        WRITE(0xc1b3, 3, DB(0x2180u), 1, false, X);
        goto source_c1b6_m0x1;
source_c1b6_m0x1: /* $80:C1B6 [M=0 X=1] STX   $08 */
        WRITE(0xc1b6, 2, 0x08u, 1, false, X);
        goto source_c1b8_m0x1;
source_c1b8_m0x1: /* $80:C1B8 [M=0 X=1] STA   $1E */
        WRITE(0xc1b8, 2, 0x1eu, 2, false, A);
        goto source_c1ba_m0x1;
source_c1ba_m0x1: /* $80:C1BA [M=0 X=1] LDY   $1F */
        READ(0xc1ba, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c1bc_m0x1;
source_c1bc_m0x1: /* $80:C1BC [M=0 X=1] LDX   $0400,Y */
        READ(0xc1bc, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_c1bf_m0x1;
source_c1bf_m0x1: /* $80:C1BF [M=0 X=1] JMP   ($C1C2,X) */
        READ(0xc1bf, 3, (0x800000u | (uint16_t)(0xc1c2u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfc9: goto source_bfc9_m0x1;
        case 0xc015: goto source_c015_m0x1;
        case 0xc062: goto source_c062_m0x1;
        case 0xc0b0: goto source_c0b0_m0x1;
        case 0xc0ff: goto source_c0ff_m0x1;
        case 0xc14f: goto source_c14f_m0x1;
        case 0xc1a0: goto source_c1a0_m0x1;
        case 0xc1d6: goto source_c1d6_m0x1;
        case 0xc1db: goto source_c1db_m0x1;
        case 0xc202: goto source_c202_m0x1;
        default: return unsupported(s);
        }
source_c1d6_m0x1: /* $80:C1D6 [M=0 X=1] LDX   #$04 */
        IMM(0xc1d6, 2);
        set_x(s, 0x0004u);
        goto source_c1d8_m0x1;
source_c1d8_m0x1: /* $80:C1D8 [M=0 X=1] JMP   $C434 */
        JUMP(0xc1d8);
        goto source_c434_m0x1;
source_c1db_m0x1: /* $80:C1DB [M=0 X=1] LDX   #$04 */
        IMM(0xc1db, 2);
        set_x(s, 0x0004u);
        goto source_c1dd_m0x1;
source_c1dd_m0x1: /* $80:C1DD [M=0 X=1] STZ   $1C */
        WRITE(0xc1dd, 2, 0x1cu, 2, false, 0);
        goto source_c1df_m0x1;
source_c1df_m0x1: /* $80:C1DF [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc1df, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c1e1_m0x1;
source_c1e1_m0x1: /* $80:C1E1 [M=0 X=1] AND   #$FF */
        IMM(0xc1e1, 3);
        set_a(s, A & 0x00ffu);
        goto source_c1e4_m0x1;
source_c1e4_m0x1: /* $80:C1E4 [M=0 X=1] ASL   A */
        IMP(0xc1e4);
        accumulator_change(s, FB30_ASL);
        goto source_c1e5_m0x1;
source_c1e5_m0x1: /* $80:C1E5 [M=0 X=1] ASL   A */
        IMP(0xc1e5);
        accumulator_change(s, FB30_ASL);
        goto source_c1e6_m0x1;
source_c1e6_m0x1: /* $80:C1E6 [M=0 X=1] ASL   A */
        IMP(0xc1e6);
        accumulator_change(s, FB30_ASL);
        goto source_c1e7_m0x1;
source_c1e7_m0x1: /* $80:C1E7 [M=0 X=1] ASL   A */
        IMP(0xc1e7);
        accumulator_change(s, FB30_ASL);
        goto source_c1e8_m0x1;
source_c1e8_m0x1: /* $80:C1E8 [M=0 X=1] ASL   A */
        IMP(0xc1e8);
        accumulator_change(s, FB30_ASL);
        goto source_c1e9_m0x1;
source_c1e9_m0x1: /* $80:C1E9 [M=0 X=1] ASL   A */
        IMP(0xc1e9);
        accumulator_change(s, FB30_ASL);
        goto source_c1ea_m0x1;
source_c1ea_m0x1: /* $80:C1EA [M=0 X=1] ORA   $1D */
        READ(0xc1ea, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c1ec_m0x1;
source_c1ec_m0x1: /* $80:C1EC [M=0 X=1] STA   $1D */
        WRITE(0xc1ec, 2, 0x1du, 2, false, A);
        goto source_c1ee_m0x1;
source_c1ee_m0x1: /* $80:C1EE [M=0 X=1] LDA   $1E */
        READ(0xc1ee, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c1f0_m0x1;
source_c1f0_m0x1: /* $80:C1F0 [M=0 X=1] JMP   ($0560) */
        READ(0xc1f0, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c1f3_m0x1: /* $80:C1F3 [M=0 X=1] ASL   A */
        IMP(0xc1f3);
        accumulator_change(s, FB30_ASL);
        goto source_c1f4_m0x1;
source_c1f4_m0x1: /* $80:C1F4 [M=0 X=1] SEP   #$20 */
        MODE(0xc1f4);
        status_set(s, P | 0x20u);
        goto source_c1f6_m1x1;
source_c1f6_m1x1: /* $80:C1F6 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc1f6, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c1f8_m1x1;
source_c1f8_m1x1: /* $80:C1F8 [M=1 X=1] REP   #$20 */
        MODE(0xc1f8);
        status_set(s, P & 0xdfu);
        goto source_c1fa_m0x1;
source_c1fa_m0x1: /* $80:C1FA [M=0 X=1] INC   $0C */
        CHANGE(0xc1fa, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c1fc_m0x1;
source_c1fc_m0x1: /* $80:C1FC [M=0 X=1] ASL   A */
        IMP(0xc1fc);
        accumulator_change(s, FB30_ASL);
        goto source_c1fd_m0x1;
source_c1fd_m0x1: /* $80:C1FD [M=0 X=1] ASL   A */
        IMP(0xc1fd);
        accumulator_change(s, FB30_ASL);
        goto source_c1fe_m0x1;
source_c1fe_m0x1: /* $80:C1FE [M=0 X=1] ASL   A */
        IMP(0xc1fe);
        accumulator_change(s, FB30_ASL);
        goto source_c1ff_m0x1;
source_c1ff_m0x1: /* $80:C1FF [M=0 X=1] ASL   A */
        IMP(0xc1ff);
        accumulator_change(s, FB30_ASL);
        goto source_c200_m0x1;
source_c200_m0x1: /* $80:C200 [M=0 X=1] ASL   A */
        IMP(0xc200);
        accumulator_change(s, FB30_ASL);
        goto source_c201_m0x1;
source_c201_m0x1: /* $80:C201 [M=0 X=1] ASL   A */
        IMP(0xc201);
        accumulator_change(s, FB30_ASL);
        goto source_c202_m0x1;
source_c202_m0x1: /* $80:C202 [M=0 X=1] ASL   A */
        IMP(0xc202);
        accumulator_change(s, FB30_ASL);
        goto source_c203_m0x1;
source_c203_m0x1: /* $80:C203 [M=0 X=1] LDX   $0300,Y */
        READ(0xc203, 3, DB(0x0300u + Y), 1, index_idle(s, 0x0300u, Y));
        set_x(s, B.read_value);
        goto source_c206_m0x1;
source_c206_m0x1: /* $80:C206 [M=0 X=1] STX   $2180 */
        WRITE(0xc206, 3, DB(0x2180u), 1, false, X);
        goto source_c209_m0x1;
source_c209_m0x1: /* $80:C209 [M=0 X=1] STX   $08 */
        WRITE(0xc209, 2, 0x08u, 1, false, X);
        goto source_c20b_m0x1;
source_c20b_m0x1: /* $80:C20B [M=0 X=1] STA   $1E */
        WRITE(0xc20b, 2, 0x1eu, 2, false, A);
        goto source_c20d_m0x1;
source_c20d_m0x1: /* $80:C20D [M=0 X=1] LDY   $1F */
        READ(0xc20d, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c20f_m0x1;
source_c20f_m0x1: /* $80:C20F [M=0 X=1] LDX   $0400,Y */
        READ(0xc20f, 3, DB(0x0400u + Y), 1, index_idle(s, 0x0400u, Y));
        set_x(s, B.read_value);
        goto source_c212_m0x1;
source_c212_m0x1: /* $80:C212 [M=0 X=1] JMP   ($C215,X) */
        READ(0xc212, 3, (0x800000u | (uint16_t)(0xc215u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfca: goto source_bfca_m0x1;
        case 0xc016: goto source_c016_m0x1;
        case 0xc063: goto source_c063_m0x1;
        case 0xc0b1: goto source_c0b1_m0x1;
        case 0xc100: goto source_c100_m0x1;
        case 0xc150: goto source_c150_m0x1;
        case 0xc1a1: goto source_c1a1_m0x1;
        case 0xc1f3: goto source_c1f3_m0x1;
        case 0xc229: goto source_c229_m0x1;
        case 0xc22e: goto source_c22e_m0x1;
        default: return unsupported(s);
        }
source_c229_m0x1: /* $80:C229 [M=0 X=1] LDX   #$02 */
        IMM(0xc229, 2);
        set_x(s, 0x0002u);
        goto source_c22b_m0x1;
source_c22b_m0x1: /* $80:C22B [M=0 X=1] JMP   $C434 */
        JUMP(0xc22b);
        goto source_c434_m0x1;
source_c22e_m0x1: /* $80:C22E [M=0 X=1] LDX   #$02 */
        IMM(0xc22e, 2);
        set_x(s, 0x0002u);
        goto source_c230_m0x1;
source_c230_m0x1: /* $80:C230 [M=0 X=1] STZ   $1C */
        WRITE(0xc230, 2, 0x1cu, 2, false, 0);
        goto source_c232_m0x1;
source_c232_m0x1: /* $80:C232 [M=0 X=1] LDA   ($0C) */
        INDIRECT(0xc232, 0x0c, 2);
        set_a(s, B.read_value);
        goto source_c234_m0x1;
source_c234_m0x1: /* $80:C234 [M=0 X=1] AND   #$FF */
        IMM(0xc234, 3);
        set_a(s, A & 0x00ffu);
        goto source_c237_m0x1;
source_c237_m0x1: /* $80:C237 [M=0 X=1] ASL   A */
        IMP(0xc237);
        accumulator_change(s, FB30_ASL);
        goto source_c238_m0x1;
source_c238_m0x1: /* $80:C238 [M=0 X=1] ASL   A */
        IMP(0xc238);
        accumulator_change(s, FB30_ASL);
        goto source_c239_m0x1;
source_c239_m0x1: /* $80:C239 [M=0 X=1] ASL   A */
        IMP(0xc239);
        accumulator_change(s, FB30_ASL);
        goto source_c23a_m0x1;
source_c23a_m0x1: /* $80:C23A [M=0 X=1] ASL   A */
        IMP(0xc23a);
        accumulator_change(s, FB30_ASL);
        goto source_c23b_m0x1;
source_c23b_m0x1: /* $80:C23B [M=0 X=1] ASL   A */
        IMP(0xc23b);
        accumulator_change(s, FB30_ASL);
        goto source_c23c_m0x1;
source_c23c_m0x1: /* $80:C23C [M=0 X=1] ASL   A */
        IMP(0xc23c);
        accumulator_change(s, FB30_ASL);
        goto source_c23d_m0x1;
source_c23d_m0x1: /* $80:C23D [M=0 X=1] ASL   A */
        IMP(0xc23d);
        accumulator_change(s, FB30_ASL);
        goto source_c23e_m0x1;
source_c23e_m0x1: /* $80:C23E [M=0 X=1] ORA   $1D */
        READ(0xc23e, 2, 0x1du, 2, false);
        set_a(s, A | B.read_value);
        goto source_c240_m0x1;
source_c240_m0x1: /* $80:C240 [M=0 X=1] STA   $1D */
        WRITE(0xc240, 2, 0x1du, 2, false, A);
        goto source_c242_m0x1;
source_c242_m0x1: /* $80:C242 [M=0 X=1] LDA   $1E */
        READ(0xc242, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c244_m0x1;
source_c244_m0x1: /* $80:C244 [M=0 X=1] JMP   ($0560) */
        READ(0xc244, 3, 0x0560u, 2, false);
        switch (B.read_value) {
        case 0xc27d: goto source_c27d_m0x1;
        case 0xc280: goto source_c280_m0x1;
        case 0xc288: goto source_c288_m0x1;
        case 0xc295: goto source_c295_m0x1;
        case 0xc2a7: goto source_c2a7_m0x1;
        case 0xc2be: goto source_c2be_m0x1;
        case 0xc2da: goto source_c2da_m0x1;
        case 0xc2fb: goto source_c2fb_m0x1;
        default: return unsupported(s);
        }
source_c247_m0x1: /* $80:C247 [M=0 X=1] LSR   A */
        IMP(0xc247);
        accumulator_change(s, FB30_LSR);
        goto source_c248_m0x1;
source_c248_m0x1: /* $80:C248 [M=0 X=1] LSR   A */
        IMP(0xc248);
        accumulator_change(s, FB30_LSR);
        goto source_c249_m0x1;
source_c249_m0x1: /* $80:C249 [M=0 X=1] LSR   A */
        IMP(0xc249);
        accumulator_change(s, FB30_LSR);
        goto source_c24a_m0x1;
source_c24a_m0x1: /* $80:C24A [M=0 X=1] LSR   A */
        IMP(0xc24a);
        accumulator_change(s, FB30_LSR);
        goto source_c24b_m0x1;
source_c24b_m0x1: /* $80:C24B [M=0 X=1] LSR   A */
        IMP(0xc24b);
        accumulator_change(s, FB30_LSR);
        goto source_c24c_m0x1;
source_c24c_m0x1: /* $80:C24C [M=0 X=1] LSR   A */
        IMP(0xc24c);
        accumulator_change(s, FB30_LSR);
        goto source_c24d_m0x1;
source_c24d_m0x1: /* $80:C24D [M=0 X=1] LSR   A */
        IMP(0xc24d);
        accumulator_change(s, FB30_LSR);
        goto source_c24e_m0x1;
source_c24e_m0x1: /* $80:C24E [M=0 X=1] SEC */
        IMP(0xc24e);
        P = (uint8_t)(P | 1u);
        goto source_c24f_m0x1;
source_c24f_m0x1: /* $80:C24F [M=0 X=1] SBC   $0530 */
        READ(0xc24f, 3, DB(0x0530u), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c252_m0x1;
source_c252_m0x1: /* $80:C252 [M=0 X=1] TAY */
        IMP(0xc252);
        set_y(s, A);
        goto source_c253_m0x1;
source_c253_m0x1: /* $80:C253 [M=0 X=1] SEP   #$20 */
        MODE(0xc253);
        status_set(s, P | 0x20u);
        goto source_c255_m1x1;
source_c255_m1x1: /* $80:C255 [M=1 X=1] LDA   $0100,Y */
        READ(0xc255, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c258_m1x1;
source_c258_m1x1: /* $80:C258 [M=1 X=1] LDY   #$01 */
        IMM(0xc258, 2);
        set_y(s, 0x0001u);
        goto source_c25a_m1x1;
source_c25a_m1x1: /* $80:C25A [M=1 X=1] CMP   $25 */
        READ(0xc25a, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c25c_m1x1;
source_c25c_m1x1: /* $80:C25C [M=1 X=1] BEQ   $C27A */
        BRANCH(0xc25c, (P & 2u) != 0);
        if (B.branch) { goto source_c27a_m1x1; }
        goto source_c25e_m1x1;
source_c25e_m1x1: /* $80:C25E [M=1 X=1] JMP   $C3A0 */
        JUMP(0xc25e);
        goto source_c3a0_m1x1;
source_c261_m0x1: /* $80:C261 [M=0 X=1] LSR   A */
        IMP(0xc261);
        accumulator_change(s, FB30_LSR);
        goto source_c262_m0x1;
source_c262_m0x1: /* $80:C262 [M=0 X=1] LSR   A */
        IMP(0xc262);
        accumulator_change(s, FB30_LSR);
        goto source_c263_m0x1;
source_c263_m0x1: /* $80:C263 [M=0 X=1] LSR   A */
        IMP(0xc263);
        accumulator_change(s, FB30_LSR);
        goto source_c264_m0x1;
source_c264_m0x1: /* $80:C264 [M=0 X=1] LSR   A */
        IMP(0xc264);
        accumulator_change(s, FB30_LSR);
        goto source_c265_m0x1;
source_c265_m0x1: /* $80:C265 [M=0 X=1] LSR   A */
        IMP(0xc265);
        accumulator_change(s, FB30_LSR);
        goto source_c266_m0x1;
source_c266_m0x1: /* $80:C266 [M=0 X=1] LSR   A */
        IMP(0xc266);
        accumulator_change(s, FB30_LSR);
        goto source_c267_m0x1;
source_c267_m0x1: /* $80:C267 [M=0 X=1] SEC */
        IMP(0xc267);
        P = (uint8_t)(P | 1u);
        goto source_c268_m0x1;
source_c268_m0x1: /* $80:C268 [M=0 X=1] SBC   $0532 */
        READ(0xc268, 3, DB(0x0532u), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c26b_m0x1;
source_c26b_m0x1: /* $80:C26B [M=0 X=1] TAY */
        IMP(0xc26b);
        set_y(s, A);
        goto source_c26c_m0x1;
source_c26c_m0x1: /* $80:C26C [M=0 X=1] SEP   #$20 */
        MODE(0xc26c);
        status_set(s, P | 0x20u);
        goto source_c26e_m1x1;
source_c26e_m1x1: /* $80:C26E [M=1 X=1] LDA   $0100,Y */
        READ(0xc26e, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c271_m1x1;
source_c271_m1x1: /* $80:C271 [M=1 X=1] LDY   #$02 */
        IMM(0xc271, 2);
        set_y(s, 0x0002u);
        goto source_c273_m1x1;
source_c273_m1x1: /* $80:C273 [M=1 X=1] CMP   $25 */
        READ(0xc273, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c275_m1x1;
source_c275_m1x1: /* $80:C275 [M=1 X=1] BEQ   $C27A */
        BRANCH(0xc275, (P & 2u) != 0);
        if (B.branch) { goto source_c27a_m1x1; }
        goto source_c277_m1x1;
source_c277_m1x1: /* $80:C277 [M=1 X=1] JMP   $C3A0 */
        JUMP(0xc277);
        goto source_c3a0_m1x1;
source_c27a_m1x1: /* $80:C27A [M=1 X=1] JMP   $C417 */
        JUMP(0xc27a);
        goto source_c417_m1x1;
source_c27d_m0x1: /* $80:C27D [M=0 X=1] JMP   $C247 */
        JUMP(0xc27d);
        goto source_c247_m0x1;
source_c280_m0x1: /* $80:C280 [M=0 X=1] CMP   $0550 */
        READ(0xc280, 3, DB(0x0550u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c283_m0x1;
source_c283_m0x1: /* $80:C283 [M=0 X=1] BCC   $C247 */
        BRANCH(0xc283, (P & 1u) == 0);
        if (B.branch) { goto source_c247_m0x1; }
        goto source_c285_m0x1;
source_c285_m0x1: /* $80:C285 [M=0 X=1] JMP   $C261 */
        JUMP(0xc285);
        goto source_c261_m0x1;
source_c288_m0x1: /* $80:C288 [M=0 X=1] CMP   $0550 */
        READ(0xc288, 3, DB(0x0550u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c28b_m0x1;
source_c28b_m0x1: /* $80:C28B [M=0 X=1] BCC   $C247 */
        BRANCH(0xc28b, (P & 1u) == 0);
        if (B.branch) { goto source_c247_m0x1; }
        goto source_c28d_m0x1;
source_c28d_m0x1: /* $80:C28D [M=0 X=1] CMP   $0552 */
        READ(0xc28d, 3, DB(0x0552u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c290_m0x1;
source_c290_m0x1: /* $80:C290 [M=0 X=1] BCC   $C261 */
        BRANCH(0xc290, (P & 1u) == 0);
        if (B.branch) { goto source_c261_m0x1; }
        goto source_c292_m0x1;
source_c292_m0x1: /* $80:C292 [M=0 X=1] JMP   $C321 */
        JUMP(0xc292);
        goto source_c321_m0x1;
source_c295_m0x1: /* $80:C295 [M=0 X=1] CMP   $0550 */
        READ(0xc295, 3, DB(0x0550u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c298_m0x1;
source_c298_m0x1: /* $80:C298 [M=0 X=1] BCC   $C247 */
        BRANCH(0xc298, (P & 1u) == 0);
        if (B.branch) { goto source_c247_m0x1; }
        goto source_c29a_m0x1;
source_c29a_m0x1: /* $80:C29A [M=0 X=1] CMP   $0552 */
        READ(0xc29a, 3, DB(0x0552u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c29d_m0x1;
source_c29d_m0x1: /* $80:C29D [M=0 X=1] BCC   $C261 */
        BRANCH(0xc29d, (P & 1u) == 0);
        if (B.branch) { goto source_c261_m0x1; }
        goto source_c29f_m0x1;
source_c29f_m0x1: /* $80:C29F [M=0 X=1] CMP   $0554 */
        READ(0xc29f, 3, DB(0x0554u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2a2_m0x1;
source_c2a2_m0x1: /* $80:C2A2 [M=0 X=1] BCC   $C321 */
        BRANCH(0xc2a2, (P & 1u) == 0);
        if (B.branch) { goto source_c321_m0x1; }
        goto source_c2a4_m0x1;
source_c2a4_m0x1: /* $80:C2A4 [M=0 X=1] JMP   $C338 */
        JUMP(0xc2a4);
        goto source_c338_m0x1;
source_c2a7_m0x1: /* $80:C2A7 [M=0 X=1] CMP   $0550 */
        READ(0xc2a7, 3, DB(0x0550u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2aa_m0x1;
source_c2aa_m0x1: /* $80:C2AA [M=0 X=1] BCC   $C247 */
        BRANCH(0xc2aa, (P & 1u) == 0);
        if (B.branch) { goto source_c247_m0x1; }
        goto source_c2ac_m0x1;
source_c2ac_m0x1: /* $80:C2AC [M=0 X=1] CMP   $0552 */
        READ(0xc2ac, 3, DB(0x0552u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2af_m0x1;
source_c2af_m0x1: /* $80:C2AF [M=0 X=1] BCC   $C261 */
        BRANCH(0xc2af, (P & 1u) == 0);
        if (B.branch) { goto source_c261_m0x1; }
        goto source_c2b1_m0x1;
source_c2b1_m0x1: /* $80:C2B1 [M=0 X=1] CMP   $0554 */
        READ(0xc2b1, 3, DB(0x0554u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2b4_m0x1;
source_c2b4_m0x1: /* $80:C2B4 [M=0 X=1] BCC   $C321 */
        BRANCH(0xc2b4, (P & 1u) == 0);
        if (B.branch) { goto source_c321_m0x1; }
        goto source_c2b6_m0x1;
source_c2b6_m0x1: /* $80:C2B6 [M=0 X=1] CMP   $0556 */
        READ(0xc2b6, 3, DB(0x0556u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2b9_m0x1;
source_c2b9_m0x1: /* $80:C2B9 [M=0 X=1] BCC   $C338 */
        BRANCH(0xc2b9, (P & 1u) == 0);
        if (B.branch) { goto source_c338_m0x1; }
        goto source_c2bb_m0x1;
source_c2bb_m0x1: /* $80:C2BB [M=0 X=1] JMP   $C354 */
        JUMP(0xc2bb);
        goto source_c354_m0x1;
source_c2be_m0x1: /* $80:C2BE [M=0 X=1] CMP   $0550 */
        READ(0xc2be, 3, DB(0x0550u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2c1_m0x1;
source_c2c1_m0x1: /* $80:C2C1 [M=0 X=1] BCC   $C247 */
        BRANCH(0xc2c1, (P & 1u) == 0);
        if (B.branch) { goto source_c247_m0x1; }
        goto source_c2c3_m0x1;
source_c2c3_m0x1: /* $80:C2C3 [M=0 X=1] CMP   $0552 */
        READ(0xc2c3, 3, DB(0x0552u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2c6_m0x1;
source_c2c6_m0x1: /* $80:C2C6 [M=0 X=1] BCC   $C261 */
        BRANCH(0xc2c6, (P & 1u) == 0);
        if (B.branch) { goto source_c261_m0x1; }
        goto source_c2c8_m0x1;
source_c2c8_m0x1: /* $80:C2C8 [M=0 X=1] CMP   $0554 */
        READ(0xc2c8, 3, DB(0x0554u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2cb_m0x1;
source_c2cb_m0x1: /* $80:C2CB [M=0 X=1] BCC   $C321 */
        BRANCH(0xc2cb, (P & 1u) == 0);
        if (B.branch) { goto source_c321_m0x1; }
        goto source_c2cd_m0x1;
source_c2cd_m0x1: /* $80:C2CD [M=0 X=1] CMP   $0556 */
        READ(0xc2cd, 3, DB(0x0556u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2d0_m0x1;
source_c2d0_m0x1: /* $80:C2D0 [M=0 X=1] BCC   $C338 */
        BRANCH(0xc2d0, (P & 1u) == 0);
        if (B.branch) { goto source_c338_m0x1; }
        goto source_c2d2_m0x1;
source_c2d2_m0x1: /* $80:C2D2 [M=0 X=1] CMP   $0558 */
        READ(0xc2d2, 3, DB(0x0558u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2d5_m0x1;
source_c2d5_m0x1: /* $80:C2D5 [M=0 X=1] BCC   $C354 */
        BRANCH(0xc2d5, (P & 1u) == 0);
        if (B.branch) { goto source_c354_m0x1; }
        goto source_c2d7_m0x1;
source_c2d7_m0x1: /* $80:C2D7 [M=0 X=1] JMP   $C369 */
        JUMP(0xc2d7);
        goto source_c369_m0x1;
source_c2da_m0x1: /* $80:C2DA [M=0 X=1] CMP   $0550 */
        READ(0xc2da, 3, DB(0x0550u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2dd_m0x1;
source_c2dd_m0x1: /* $80:C2DD [M=0 X=1] BCC   $C27D */
        BRANCH(0xc2dd, (P & 1u) == 0);
        if (B.branch) { goto source_c27d_m0x1; }
        goto source_c2df_m0x1;
source_c2df_m0x1: /* $80:C2DF [M=0 X=1] CMP   $0552 */
        READ(0xc2df, 3, DB(0x0552u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2e2_m0x1;
source_c2e2_m0x1: /* $80:C2E2 [M=0 X=1] BCC   $C285 */
        BRANCH(0xc2e2, (P & 1u) == 0);
        if (B.branch) { goto source_c285_m0x1; }
        goto source_c2e4_m0x1;
source_c2e4_m0x1: /* $80:C2E4 [M=0 X=1] CMP   $0554 */
        READ(0xc2e4, 3, DB(0x0554u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2e7_m0x1;
source_c2e7_m0x1: /* $80:C2E7 [M=0 X=1] BCC   $C321 */
        BRANCH(0xc2e7, (P & 1u) == 0);
        if (B.branch) { goto source_c321_m0x1; }
        goto source_c2e9_m0x1;
source_c2e9_m0x1: /* $80:C2E9 [M=0 X=1] CMP   $0556 */
        READ(0xc2e9, 3, DB(0x0556u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2ec_m0x1;
source_c2ec_m0x1: /* $80:C2EC [M=0 X=1] BCC   $C338 */
        BRANCH(0xc2ec, (P & 1u) == 0);
        if (B.branch) { goto source_c338_m0x1; }
        goto source_c2ee_m0x1;
source_c2ee_m0x1: /* $80:C2EE [M=0 X=1] CMP   $0558 */
        READ(0xc2ee, 3, DB(0x0558u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2f1_m0x1;
source_c2f1_m0x1: /* $80:C2F1 [M=0 X=1] BCC   $C354 */
        BRANCH(0xc2f1, (P & 1u) == 0);
        if (B.branch) { goto source_c354_m0x1; }
        goto source_c2f3_m0x1;
source_c2f3_m0x1: /* $80:C2F3 [M=0 X=1] CMP   $055A */
        READ(0xc2f3, 3, DB(0x055au), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2f6_m0x1;
source_c2f6_m0x1: /* $80:C2F6 [M=0 X=1] BCC   $C369 */
        BRANCH(0xc2f6, (P & 1u) == 0);
        if (B.branch) { goto source_c369_m0x1; }
        goto source_c2f8_m0x1;
source_c2f8_m0x1: /* $80:C2F8 [M=0 X=1] JMP   $C37D */
        JUMP(0xc2f8);
        goto source_c37d_m0x1;
source_c2fb_m0x1: /* $80:C2FB [M=0 X=1] CMP   $0550 */
        READ(0xc2fb, 3, DB(0x0550u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c2fe_m0x1;
source_c2fe_m0x1: /* $80:C2FE [M=0 X=1] BCC   $C351 */
        BRANCH(0xc2fe, (P & 1u) == 0);
        if (B.branch) { goto source_c351_m0x1; }
        goto source_c300_m0x1;
source_c300_m0x1: /* $80:C300 [M=0 X=1] CMP   $0552 */
        READ(0xc300, 3, DB(0x0552u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c303_m0x1;
source_c303_m0x1: /* $80:C303 [M=0 X=1] BCC   $C285 */
        BRANCH(0xc303, (P & 1u) == 0);
        if (B.branch) { goto source_c285_m0x1; }
        goto source_c305_m0x1;
source_c305_m0x1: /* $80:C305 [M=0 X=1] CMP   $0554 */
        READ(0xc305, 3, DB(0x0554u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c308_m0x1;
source_c308_m0x1: /* $80:C308 [M=0 X=1] BCC   $C321 */
        BRANCH(0xc308, (P & 1u) == 0);
        if (B.branch) { goto source_c321_m0x1; }
        goto source_c30a_m0x1;
source_c30a_m0x1: /* $80:C30A [M=0 X=1] CMP   $0556 */
        READ(0xc30a, 3, DB(0x0556u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c30d_m0x1;
source_c30d_m0x1: /* $80:C30D [M=0 X=1] BCC   $C338 */
        BRANCH(0xc30d, (P & 1u) == 0);
        if (B.branch) { goto source_c338_m0x1; }
        goto source_c30f_m0x1;
source_c30f_m0x1: /* $80:C30F [M=0 X=1] CMP   $0558 */
        READ(0xc30f, 3, DB(0x0558u), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c312_m0x1;
source_c312_m0x1: /* $80:C312 [M=0 X=1] BCC   $C354 */
        BRANCH(0xc312, (P & 1u) == 0);
        if (B.branch) { goto source_c354_m0x1; }
        goto source_c314_m0x1;
source_c314_m0x1: /* $80:C314 [M=0 X=1] CMP   $055A */
        READ(0xc314, 3, DB(0x055au), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c317_m0x1;
source_c317_m0x1: /* $80:C317 [M=0 X=1] BCC   $C369 */
        BRANCH(0xc317, (P & 1u) == 0);
        if (B.branch) { goto source_c369_m0x1; }
        goto source_c319_m0x1;
source_c319_m0x1: /* $80:C319 [M=0 X=1] CMP   $055C */
        READ(0xc319, 3, DB(0x055cu), 2, false);
        compare(s, A, B.read_value, 2);
        goto source_c31c_m0x1;
source_c31c_m0x1: /* $80:C31C [M=0 X=1] BCC   $C37D */
        BRANCH(0xc31c, (P & 1u) == 0);
        if (B.branch) { goto source_c37d_m0x1; }
        goto source_c31e_m0x1;
source_c31e_m0x1: /* $80:C31E [M=0 X=1] JMP   $C390 */
        JUMP(0xc31e);
        goto source_c390_m0x1;
source_c321_m0x1: /* $80:C321 [M=0 X=1] LSR   A */
        IMP(0xc321);
        accumulator_change(s, FB30_LSR);
        goto source_c322_m0x1;
source_c322_m0x1: /* $80:C322 [M=0 X=1] LSR   A */
        IMP(0xc322);
        accumulator_change(s, FB30_LSR);
        goto source_c323_m0x1;
source_c323_m0x1: /* $80:C323 [M=0 X=1] LSR   A */
        IMP(0xc323);
        accumulator_change(s, FB30_LSR);
        goto source_c324_m0x1;
source_c324_m0x1: /* $80:C324 [M=0 X=1] LSR   A */
        IMP(0xc324);
        accumulator_change(s, FB30_LSR);
        goto source_c325_m0x1;
source_c325_m0x1: /* $80:C325 [M=0 X=1] LSR   A */
        IMP(0xc325);
        accumulator_change(s, FB30_LSR);
        goto source_c326_m0x1;
source_c326_m0x1: /* $80:C326 [M=0 X=1] SEC */
        IMP(0xc326);
        P = (uint8_t)(P | 1u);
        goto source_c327_m0x1;
source_c327_m0x1: /* $80:C327 [M=0 X=1] SBC   $0534 */
        READ(0xc327, 3, DB(0x0534u), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c32a_m0x1;
source_c32a_m0x1: /* $80:C32A [M=0 X=1] TAY */
        IMP(0xc32a);
        set_y(s, A);
        goto source_c32b_m0x1;
source_c32b_m0x1: /* $80:C32B [M=0 X=1] SEP   #$20 */
        MODE(0xc32b);
        status_set(s, P | 0x20u);
        goto source_c32d_m1x1;
source_c32d_m1x1: /* $80:C32D [M=1 X=1] LDA   $0100,Y */
        READ(0xc32d, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c330_m1x1;
source_c330_m1x1: /* $80:C330 [M=1 X=1] LDY   #$03 */
        IMM(0xc330, 2);
        set_y(s, 0x0003u);
        goto source_c332_m1x1;
source_c332_m1x1: /* $80:C332 [M=1 X=1] CMP   $25 */
        READ(0xc332, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c334_m1x1;
source_c334_m1x1: /* $80:C334 [M=1 X=1] BEQ   $C34E */
        BRANCH(0xc334, (P & 2u) != 0);
        if (B.branch) { goto source_c34e_m1x1; }
        goto source_c336_m1x1;
source_c336_m1x1: /* $80:C336 [M=1 X=1] BRA   $C3A0 */
        BRANCH(0xc336, true);
        goto source_c3a0_m1x1;
source_c338_m0x1: /* $80:C338 [M=0 X=1] LSR   A */
        IMP(0xc338);
        accumulator_change(s, FB30_LSR);
        goto source_c339_m0x1;
source_c339_m0x1: /* $80:C339 [M=0 X=1] LSR   A */
        IMP(0xc339);
        accumulator_change(s, FB30_LSR);
        goto source_c33a_m0x1;
source_c33a_m0x1: /* $80:C33A [M=0 X=1] LSR   A */
        IMP(0xc33a);
        accumulator_change(s, FB30_LSR);
        goto source_c33b_m0x1;
source_c33b_m0x1: /* $80:C33B [M=0 X=1] LSR   A */
        IMP(0xc33b);
        accumulator_change(s, FB30_LSR);
        goto source_c33c_m0x1;
source_c33c_m0x1: /* $80:C33C [M=0 X=1] SEC */
        IMP(0xc33c);
        P = (uint8_t)(P | 1u);
        goto source_c33d_m0x1;
source_c33d_m0x1: /* $80:C33D [M=0 X=1] SBC   $0536 */
        READ(0xc33d, 3, DB(0x0536u), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c340_m0x1;
source_c340_m0x1: /* $80:C340 [M=0 X=1] TAY */
        IMP(0xc340);
        set_y(s, A);
        goto source_c341_m0x1;
source_c341_m0x1: /* $80:C341 [M=0 X=1] SEP   #$20 */
        MODE(0xc341);
        status_set(s, P | 0x20u);
        goto source_c343_m1x1;
source_c343_m1x1: /* $80:C343 [M=1 X=1] LDA   $0100,Y */
        READ(0xc343, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c346_m1x1;
source_c346_m1x1: /* $80:C346 [M=1 X=1] LDY   #$04 */
        IMM(0xc346, 2);
        set_y(s, 0x0004u);
        goto source_c348_m1x1;
source_c348_m1x1: /* $80:C348 [M=1 X=1] CMP   $25 */
        READ(0xc348, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c34a_m1x1;
source_c34a_m1x1: /* $80:C34A [M=1 X=1] BEQ   $C34E */
        BRANCH(0xc34a, (P & 2u) != 0);
        if (B.branch) { goto source_c34e_m1x1; }
        goto source_c34c_m1x1;
source_c34c_m1x1: /* $80:C34C [M=1 X=1] BRA   $C3A0 */
        BRANCH(0xc34c, true);
        goto source_c3a0_m1x1;
source_c34e_m1x1: /* $80:C34E [M=1 X=1] JMP   $C417 */
        JUMP(0xc34e);
        goto source_c417_m1x1;
source_c351_m0x1: /* $80:C351 [M=0 X=1] JMP   $C247 */
        JUMP(0xc351);
        goto source_c247_m0x1;
source_c354_m0x1: /* $80:C354 [M=0 X=1] LSR   A */
        IMP(0xc354);
        accumulator_change(s, FB30_LSR);
        goto source_c355_m0x1;
source_c355_m0x1: /* $80:C355 [M=0 X=1] LSR   A */
        IMP(0xc355);
        accumulator_change(s, FB30_LSR);
        goto source_c356_m0x1;
source_c356_m0x1: /* $80:C356 [M=0 X=1] LSR   A */
        IMP(0xc356);
        accumulator_change(s, FB30_LSR);
        goto source_c357_m0x1;
source_c357_m0x1: /* $80:C357 [M=0 X=1] SEC */
        IMP(0xc357);
        P = (uint8_t)(P | 1u);
        goto source_c358_m0x1;
source_c358_m0x1: /* $80:C358 [M=0 X=1] SBC   $0538 */
        READ(0xc358, 3, DB(0x0538u), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c35b_m0x1;
source_c35b_m0x1: /* $80:C35B [M=0 X=1] TAY */
        IMP(0xc35b);
        set_y(s, A);
        goto source_c35c_m0x1;
source_c35c_m0x1: /* $80:C35C [M=0 X=1] SEP   #$20 */
        MODE(0xc35c);
        status_set(s, P | 0x20u);
        goto source_c35e_m1x1;
source_c35e_m1x1: /* $80:C35E [M=1 X=1] LDA   $0100,Y */
        READ(0xc35e, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c361_m1x1;
source_c361_m1x1: /* $80:C361 [M=1 X=1] LDY   #$05 */
        IMM(0xc361, 2);
        set_y(s, 0x0005u);
        goto source_c363_m1x1;
source_c363_m1x1: /* $80:C363 [M=1 X=1] CMP   $25 */
        READ(0xc363, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c365_m1x1;
source_c365_m1x1: /* $80:C365 [M=1 X=1] BEQ   $C34E */
        BRANCH(0xc365, (P & 2u) != 0);
        if (B.branch) { goto source_c34e_m1x1; }
        goto source_c367_m1x1;
source_c367_m1x1: /* $80:C367 [M=1 X=1] BRA   $C3A0 */
        BRANCH(0xc367, true);
        goto source_c3a0_m1x1;
source_c369_m0x1: /* $80:C369 [M=0 X=1] LSR   A */
        IMP(0xc369);
        accumulator_change(s, FB30_LSR);
        goto source_c36a_m0x1;
source_c36a_m0x1: /* $80:C36A [M=0 X=1] LSR   A */
        IMP(0xc36a);
        accumulator_change(s, FB30_LSR);
        goto source_c36b_m0x1;
source_c36b_m0x1: /* $80:C36B [M=0 X=1] SEC */
        IMP(0xc36b);
        P = (uint8_t)(P | 1u);
        goto source_c36c_m0x1;
source_c36c_m0x1: /* $80:C36C [M=0 X=1] SBC   $053A */
        READ(0xc36c, 3, DB(0x053au), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c36f_m0x1;
source_c36f_m0x1: /* $80:C36F [M=0 X=1] TAY */
        IMP(0xc36f);
        set_y(s, A);
        goto source_c370_m0x1;
source_c370_m0x1: /* $80:C370 [M=0 X=1] SEP   #$20 */
        MODE(0xc370);
        status_set(s, P | 0x20u);
        goto source_c372_m1x1;
source_c372_m1x1: /* $80:C372 [M=1 X=1] LDA   $0100,Y */
        READ(0xc372, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c375_m1x1;
source_c375_m1x1: /* $80:C375 [M=1 X=1] LDY   #$06 */
        IMM(0xc375, 2);
        set_y(s, 0x0006u);
        goto source_c377_m1x1;
source_c377_m1x1: /* $80:C377 [M=1 X=1] CMP   $25 */
        READ(0xc377, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c379_m1x1;
source_c379_m1x1: /* $80:C379 [M=1 X=1] BEQ   $C34E */
        BRANCH(0xc379, (P & 2u) != 0);
        if (B.branch) { goto source_c34e_m1x1; }
        goto source_c37b_m1x1;
source_c37b_m1x1: /* $80:C37B [M=1 X=1] BRA   $C3A0 */
        BRANCH(0xc37b, true);
        goto source_c3a0_m1x1;
source_c37d_m0x1: /* $80:C37D [M=0 X=1] LSR   A */
        IMP(0xc37d);
        accumulator_change(s, FB30_LSR);
        goto source_c37e_m0x1;
source_c37e_m0x1: /* $80:C37E [M=0 X=1] SEC */
        IMP(0xc37e);
        P = (uint8_t)(P | 1u);
        goto source_c37f_m0x1;
source_c37f_m0x1: /* $80:C37F [M=0 X=1] SBC   $053C */
        READ(0xc37f, 3, DB(0x053cu), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c382_m0x1;
source_c382_m0x1: /* $80:C382 [M=0 X=1] TAY */
        IMP(0xc382);
        set_y(s, A);
        goto source_c383_m0x1;
source_c383_m0x1: /* $80:C383 [M=0 X=1] SEP   #$20 */
        MODE(0xc383);
        status_set(s, P | 0x20u);
        goto source_c385_m1x1;
source_c385_m1x1: /* $80:C385 [M=1 X=1] LDA   $0100,Y */
        READ(0xc385, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c388_m1x1;
source_c388_m1x1: /* $80:C388 [M=1 X=1] LDY   #$07 */
        IMM(0xc388, 2);
        set_y(s, 0x0007u);
        goto source_c38a_m1x1;
source_c38a_m1x1: /* $80:C38A [M=1 X=1] CMP   $25 */
        READ(0xc38a, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c38c_m1x1;
source_c38c_m1x1: /* $80:C38C [M=1 X=1] BEQ   $C34E */
        BRANCH(0xc38c, (P & 2u) != 0);
        if (B.branch) { goto source_c34e_m1x1; }
        goto source_c38e_m1x1;
source_c38e_m1x1: /* $80:C38E [M=1 X=1] BRA   $C3A0 */
        BRANCH(0xc38e, true);
        goto source_c3a0_m1x1;
source_c390_m0x1: /* $80:C390 [M=0 X=1] SEC */
        IMP(0xc390);
        P = (uint8_t)(P | 1u);
        goto source_c391_m0x1;
source_c391_m0x1: /* $80:C391 [M=0 X=1] SBC   $053E */
        READ(0xc391, 3, DB(0x053eu), 2, false);
        add_sub(s, B.read_value, true);
        goto source_c394_m0x1;
source_c394_m0x1: /* $80:C394 [M=0 X=1] TAY */
        IMP(0xc394);
        set_y(s, A);
        goto source_c395_m0x1;
source_c395_m0x1: /* $80:C395 [M=0 X=1] SEP   #$20 */
        MODE(0xc395);
        status_set(s, P | 0x20u);
        goto source_c397_m1x1;
source_c397_m1x1: /* $80:C397 [M=1 X=1] LDA   $0100,Y */
        READ(0xc397, 3, DB(0x0100u + Y), 1, index_idle(s, 0x0100u, Y));
        set_a(s, B.read_value);
        goto source_c39a_m1x1;
source_c39a_m1x1: /* $80:C39A [M=1 X=1] LDY   #$08 */
        IMM(0xc39a, 2);
        set_y(s, 0x0008u);
        goto source_c39c_m1x1;
source_c39c_m1x1: /* $80:C39C [M=1 X=1] CMP   $25 */
        READ(0xc39c, 2, 0x25u, 1, false);
        compare(s, A, B.read_value, 1);
        goto source_c39e_m1x1;
source_c39e_m1x1: /* $80:C39E [M=1 X=1] BEQ   $C417 */
        BRANCH(0xc39e, (P & 2u) != 0);
        if (B.branch) { goto source_c417_m1x1; }
        goto source_c3a0_m1x1;
source_c3a0_m1x1: /* $80:C3A0 [M=1 X=1] STA   $2180 */
        WRITE(0xc3a0, 3, DB(0x2180u), 1, false, A);
        goto source_c3a3_m1x1;
source_c3a3_m1x1: /* $80:C3A3 [M=1 X=1] STA   $08 */
        WRITE(0xc3a3, 2, 0x08u, 1, false, A);
        goto source_c3a5_m1x1;
source_c3a5_m1x1: /* $80:C3A5 [M=1 X=1] REP   #$20 */
        MODE(0xc3a5);
        status_set(s, P & 0xdfu);
        goto source_c3a7_m0x1;
source_c3a7_m0x1: /* $80:C3A7 [M=0 X=1] INC   $0C */
        CHANGE(0xc3a7, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c3a9_m0x1;
source_c3a9_m0x1: /* $80:C3A9 [M=0 X=1] LDA   $1D */
        READ(0xc3a9, 2, 0x1du, 2, false);
        set_a(s, B.read_value);
        goto source_c3ab_m0x1;
source_c3ab_m0x1: /* $80:C3AB [M=0 X=1] JMP   ($C3AC,X) */
        READ(0xc3ab, 3, (0x800000u | (uint16_t)(0xc3acu + X)), 2, true);
        switch (B.read_value) {
        case 0xc3be: goto source_c3be_m0x1;
        case 0xc3c2: goto source_c3c2_m0x1;
        case 0xc3c6: goto source_c3c6_m0x1;
        case 0xc3ca: goto source_c3ca_m0x1;
        case 0xc3ce: goto source_c3ce_m0x1;
        case 0xc3d2: goto source_c3d2_m0x1;
        case 0xc3d6: goto source_c3d6_m0x1;
        case 0xc3da: goto source_c3da_m0x1;
        default: return unsupported(s);
        }
source_c3be_m0x1: /* $80:C3BE [M=0 X=1] ASL   A */
        IMP(0xc3be);
        accumulator_change(s, FB30_ASL);
        goto source_c3bf_m0x1;
source_c3bf_m0x1: /* $80:C3BF [M=0 X=1] DEY */
        IMP(0xc3bf);
        set_y(s, Y - 1u);
        goto source_c3c0_m0x1;
source_c3c0_m0x1: /* $80:C3C0 [M=0 X=1] BEQ   $C411 */
        BRANCH(0xc3c0, (P & 2u) != 0);
        if (B.branch) { goto source_c411_m0x1; }
        goto source_c3c2_m0x1;
source_c3c2_m0x1: /* $80:C3C2 [M=0 X=1] ASL   A */
        IMP(0xc3c2);
        accumulator_change(s, FB30_ASL);
        goto source_c3c3_m0x1;
source_c3c3_m0x1: /* $80:C3C3 [M=0 X=1] DEY */
        IMP(0xc3c3);
        set_y(s, Y - 1u);
        goto source_c3c4_m0x1;
source_c3c4_m0x1: /* $80:C3C4 [M=0 X=1] BEQ   $C40E */
        BRANCH(0xc3c4, (P & 2u) != 0);
        if (B.branch) { goto source_c40e_m0x1; }
        goto source_c3c6_m0x1;
source_c3c6_m0x1: /* $80:C3C6 [M=0 X=1] ASL   A */
        IMP(0xc3c6);
        accumulator_change(s, FB30_ASL);
        goto source_c3c7_m0x1;
source_c3c7_m0x1: /* $80:C3C7 [M=0 X=1] DEY */
        IMP(0xc3c7);
        set_y(s, Y - 1u);
        goto source_c3c8_m0x1;
source_c3c8_m0x1: /* $80:C3C8 [M=0 X=1] BEQ   $C40B */
        BRANCH(0xc3c8, (P & 2u) != 0);
        if (B.branch) { goto source_c40b_m0x1; }
        goto source_c3ca_m0x1;
source_c3ca_m0x1: /* $80:C3CA [M=0 X=1] ASL   A */
        IMP(0xc3ca);
        accumulator_change(s, FB30_ASL);
        goto source_c3cb_m0x1;
source_c3cb_m0x1: /* $80:C3CB [M=0 X=1] DEY */
        IMP(0xc3cb);
        set_y(s, Y - 1u);
        goto source_c3cc_m0x1;
source_c3cc_m0x1: /* $80:C3CC [M=0 X=1] BEQ   $C408 */
        BRANCH(0xc3cc, (P & 2u) != 0);
        if (B.branch) { goto source_c408_m0x1; }
        goto source_c3ce_m0x1;
source_c3ce_m0x1: /* $80:C3CE [M=0 X=1] ASL   A */
        IMP(0xc3ce);
        accumulator_change(s, FB30_ASL);
        goto source_c3cf_m0x1;
source_c3cf_m0x1: /* $80:C3CF [M=0 X=1] DEY */
        IMP(0xc3cf);
        set_y(s, Y - 1u);
        goto source_c3d0_m0x1;
source_c3d0_m0x1: /* $80:C3D0 [M=0 X=1] BEQ   $C405 */
        BRANCH(0xc3d0, (P & 2u) != 0);
        if (B.branch) { goto source_c405_m0x1; }
        goto source_c3d2_m0x1;
source_c3d2_m0x1: /* $80:C3D2 [M=0 X=1] ASL   A */
        IMP(0xc3d2);
        accumulator_change(s, FB30_ASL);
        goto source_c3d3_m0x1;
source_c3d3_m0x1: /* $80:C3D3 [M=0 X=1] DEY */
        IMP(0xc3d3);
        set_y(s, Y - 1u);
        goto source_c3d4_m0x1;
source_c3d4_m0x1: /* $80:C3D4 [M=0 X=1] BEQ   $C402 */
        BRANCH(0xc3d4, (P & 2u) != 0);
        if (B.branch) { goto source_c402_m0x1; }
        goto source_c3d6_m0x1;
source_c3d6_m0x1: /* $80:C3D6 [M=0 X=1] ASL   A */
        IMP(0xc3d6);
        accumulator_change(s, FB30_ASL);
        goto source_c3d7_m0x1;
source_c3d7_m0x1: /* $80:C3D7 [M=0 X=1] DEY */
        IMP(0xc3d7);
        set_y(s, Y - 1u);
        goto source_c3d8_m0x1;
source_c3d8_m0x1: /* $80:C3D8 [M=0 X=1] BEQ   $C3FF */
        BRANCH(0xc3d8, (P & 2u) != 0);
        if (B.branch) { goto source_c3ff_m0x1; }
        goto source_c3da_m0x1;
source_c3da_m0x1: /* $80:C3DA [M=0 X=1] ASL   A */
        IMP(0xc3da);
        accumulator_change(s, FB30_ASL);
        goto source_c3db_m0x1;
source_c3db_m0x1: /* $80:C3DB [M=0 X=1] SEP   #$20 */
        MODE(0xc3db);
        status_set(s, P | 0x20u);
        goto source_c3dd_m1x1;
source_c3dd_m1x1: /* $80:C3DD [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc3dd, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c3df_m1x1;
source_c3df_m1x1: /* $80:C3DF [M=1 X=1] REP   #$20 */
        MODE(0xc3df);
        status_set(s, P & 0xdfu);
        goto source_c3e1_m0x1;
source_c3e1_m0x1: /* $80:C3E1 [M=0 X=1] INC   $0C */
        CHANGE(0xc3e1, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c3e3_m0x1;
source_c3e3_m0x1: /* $80:C3E3 [M=0 X=1] DEY */
        IMP(0xc3e3);
        set_y(s, Y - 1u);
        goto source_c3e4_m0x1;
source_c3e4_m0x1: /* $80:C3E4 [M=0 X=1] BEQ   $C414 */
        BRANCH(0xc3e4, (P & 2u) != 0);
        if (B.branch) { goto source_c414_m0x1; }
        goto source_c3e6_m0x1;
source_c3e6_m0x1: /* $80:C3E6 [M=0 X=1] ASL   A */
        IMP(0xc3e6);
        accumulator_change(s, FB30_ASL);
        goto source_c3e7_m0x1;
source_c3e7_m0x1: /* $80:C3E7 [M=0 X=1] DEY */
        IMP(0xc3e7);
        set_y(s, Y - 1u);
        goto source_c3e8_m0x1;
source_c3e8_m0x1: /* $80:C3E8 [M=0 X=1] BEQ   $C411 */
        BRANCH(0xc3e8, (P & 2u) != 0);
        if (B.branch) { goto source_c411_m0x1; }
        goto source_c3ea_m0x1;
source_c3ea_m0x1: /* $80:C3EA [M=0 X=1] ASL   A */
        IMP(0xc3ea);
        accumulator_change(s, FB30_ASL);
        goto source_c3eb_m0x1;
source_c3eb_m0x1: /* $80:C3EB [M=0 X=1] DEY */
        IMP(0xc3eb);
        set_y(s, Y - 1u);
        goto source_c3ec_m0x1;
source_c3ec_m0x1: /* $80:C3EC [M=0 X=1] BEQ   $C40E */
        BRANCH(0xc3ec, (P & 2u) != 0);
        if (B.branch) { goto source_c40e_m0x1; }
        goto source_c3ee_m0x1;
source_c3ee_m0x1: /* $80:C3EE [M=0 X=1] ASL   A */
        IMP(0xc3ee);
        accumulator_change(s, FB30_ASL);
        goto source_c3ef_m0x1;
source_c3ef_m0x1: /* $80:C3EF [M=0 X=1] DEY */
        IMP(0xc3ef);
        set_y(s, Y - 1u);
        goto source_c3f0_m0x1;
source_c3f0_m0x1: /* $80:C3F0 [M=0 X=1] BEQ   $C40B */
        BRANCH(0xc3f0, (P & 2u) != 0);
        if (B.branch) { goto source_c40b_m0x1; }
        goto source_c3f2_m0x1;
source_c3f2_m0x1: /* $80:C3F2 [M=0 X=1] ASL   A */
        IMP(0xc3f2);
        accumulator_change(s, FB30_ASL);
        goto source_c3f3_m0x1;
source_c3f3_m0x1: /* $80:C3F3 [M=0 X=1] DEY */
        IMP(0xc3f3);
        set_y(s, Y - 1u);
        goto source_c3f4_m0x1;
source_c3f4_m0x1: /* $80:C3F4 [M=0 X=1] BEQ   $C408 */
        BRANCH(0xc3f4, (P & 2u) != 0);
        if (B.branch) { goto source_c408_m0x1; }
        goto source_c3f6_m0x1;
source_c3f6_m0x1: /* $80:C3F6 [M=0 X=1] ASL   A */
        IMP(0xc3f6);
        accumulator_change(s, FB30_ASL);
        goto source_c3f7_m0x1;
source_c3f7_m0x1: /* $80:C3F7 [M=0 X=1] DEY */
        IMP(0xc3f7);
        set_y(s, Y - 1u);
        goto source_c3f8_m0x1;
source_c3f8_m0x1: /* $80:C3F8 [M=0 X=1] BEQ   $C405 */
        BRANCH(0xc3f8, (P & 2u) != 0);
        if (B.branch) { goto source_c405_m0x1; }
        goto source_c3fa_m0x1;
source_c3fa_m0x1: /* $80:C3FA [M=0 X=1] ASL   A */
        IMP(0xc3fa);
        accumulator_change(s, FB30_ASL);
        goto source_c3fb_m0x1;
source_c3fb_m0x1: /* $80:C3FB [M=0 X=1] DEY */
        IMP(0xc3fb);
        set_y(s, Y - 1u);
        goto source_c3fc_m0x1;
source_c3fc_m0x1: /* $80:C3FC [M=0 X=1] BEQ   $C402 */
        BRANCH(0xc3fc, (P & 2u) != 0);
        if (B.branch) { goto source_c402_m0x1; }
        goto source_c3fe_m0x1;
source_c3fe_m0x1: /* $80:C3FE [M=0 X=1] ASL   A */
        IMP(0xc3fe);
        accumulator_change(s, FB30_ASL);
        goto source_c3ff_m0x1;
source_c3ff_m0x1: /* $80:C3FF [M=0 X=1] JMP   $C20B */
        JUMP(0xc3ff);
        goto source_c20b_m0x1;
source_c402_m0x1: /* $80:C402 [M=0 X=1] JMP   $C1B8 */
        JUMP(0xc402);
        goto source_c1b8_m0x1;
source_c405_m0x1: /* $80:C405 [M=0 X=1] JMP   $C166 */
        JUMP(0xc405);
        goto source_c166_m0x1;
source_c408_m0x1: /* $80:C408 [M=0 X=1] JMP   $C115 */
        JUMP(0xc408);
        goto source_c115_m0x1;
source_c40b_m0x1: /* $80:C40B [M=0 X=1] JMP   $C0C5 */
        JUMP(0xc40b);
        goto source_c0c5_m0x1;
source_c40e_m0x1: /* $80:C40E [M=0 X=1] JMP   $C076 */
        JUMP(0xc40e);
        goto source_c076_m0x1;
source_c411_m0x1: /* $80:C411 [M=0 X=1] JMP   $C028 */
        JUMP(0xc411);
        goto source_c028_m0x1;
source_c414_m0x1: /* $80:C414 [M=0 X=1] JMP   $BFDB */
        JUMP(0xc414);
        goto source_bfdb_m0x1;
source_c417_m1x1: /* $80:C417 [M=1 X=1] REP   #$20 */
        MODE(0xc417);
        status_set(s, P & 0xdfu);
        goto source_c419_m0x1;
source_c419_m0x1: /* $80:C419 [M=0 X=1] INC   $0C */
        CHANGE(0xc419, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c41b_m0x1;
source_c41b_m0x1: /* $80:C41B [M=0 X=1] LDA   $1D */
        READ(0xc41b, 2, 0x1du, 2, false);
        set_a(s, B.read_value);
        goto source_c41d_m0x1;
source_c41d_m0x1: /* $80:C41D [M=0 X=1] JSR   $C594 */
        PUSH(0xc41d, 3, 0xc41f, 2);
        goto source_c594_m0x1;
source_c420_m0x1: /* $80:C420 [M=0 X=1] STA   $1E */
        WRITE(0xc420, 2, 0x1eu, 2, false, A);
        goto source_c422_m0x1;
source_c422_m0x1: /* $80:C422 [M=0 X=1] JSR   $C4EA */
        PUSH(0xc422, 3, 0xc424, 2);
        goto source_c4ea_m0x1;
source_c425_m0x1: /* $80:C425 [M=0 X=1] BEQ   $C44D */
        BRANCH(0xc425, (P & 2u) != 0);
        if (B.branch) { goto source_c44d_m0x1; }
        goto source_c427_m0x1;
source_c427_m0x1: /* $80:C427 [M=0 X=1] LDY   $08 */
        READ(0xc427, 2, 0x08u, 1, false);
        set_y(s, B.read_value);
        goto source_c429_m0x1;
source_c429_m0x1: /* $80:C429 [M=0 X=1] STY   $2180 */
        WRITE(0xc429, 3, DB(0x2180u), 1, false, Y);
        goto source_c42c_m0x1;
source_c42c_m0x1: /* $80:C42C [M=0 X=1] DEC   A */
        IMP(0xc42c);
        accumulator_change(s, FB30_DEC);
        goto source_c42d_m0x1;
source_c42d_m0x1: /* $80:C42D [M=0 X=1] BNE   $C429 */
        BRANCH(0xc42d, (P & 2u) == 0);
        if (B.branch) { goto source_c429_m0x1; }
        goto source_c42f_m0x1;
source_c42f_m0x1: /* $80:C42F [M=0 X=1] LDA   $1E */
        READ(0xc42f, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c431_m0x1;
source_c431_m0x1: /* $80:C431 [M=0 X=1] JMP   ($BFB1,X) */
        READ(0xc431, 3, (0x800000u | (uint16_t)(0xbfb1u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfdb: goto source_bfdb_m0x1;
        case 0xc028: goto source_c028_m0x1;
        case 0xc076: goto source_c076_m0x1;
        case 0xc0c5: goto source_c0c5_m0x1;
        case 0xc115: goto source_c115_m0x1;
        case 0xc166: goto source_c166_m0x1;
        case 0xc1b8: goto source_c1b8_m0x1;
        case 0xc20b: goto source_c20b_m0x1;
        default: return unsupported(s);
        }
source_c434_m0x1: /* $80:C434 [M=0 X=1] LDY   $26 */
        READ(0xc434, 2, 0x26u, 1, false);
        set_y(s, B.read_value);
        goto source_c436_m0x1;
source_c436_m0x1: /* $80:C436 [M=0 X=1] JSR   $C594 */
        PUSH(0xc436, 3, 0xc438, 2);
        goto source_c594_m0x1;
source_c439_m0x1: /* $80:C439 [M=0 X=1] STA   $1E */
        WRITE(0xc439, 2, 0x1eu, 2, false, A);
        goto source_c43b_m0x1;
source_c43b_m0x1: /* $80:C43B [M=0 X=1] JSR   $C4EA */
        PUSH(0xc43b, 3, 0xc43d, 2);
        goto source_c4ea_m0x1;
source_c43e_m0x1: /* $80:C43E [M=0 X=1] BEQ   $C44D */
        BRANCH(0xc43e, (P & 2u) != 0);
        if (B.branch) { goto source_c44d_m0x1; }
        goto source_c440_m0x1;
source_c440_m0x1: /* $80:C440 [M=0 X=1] LDY   $08 */
        READ(0xc440, 2, 0x08u, 1, false);
        set_y(s, B.read_value);
        goto source_c442_m0x1;
source_c442_m0x1: /* $80:C442 [M=0 X=1] STY   $2180 */
        WRITE(0xc442, 3, DB(0x2180u), 1, false, Y);
        goto source_c445_m0x1;
source_c445_m0x1: /* $80:C445 [M=0 X=1] DEC   A */
        IMP(0xc445);
        accumulator_change(s, FB30_DEC);
        goto source_c446_m0x1;
source_c446_m0x1: /* $80:C446 [M=0 X=1] BNE   $C442 */
        BRANCH(0xc446, (P & 2u) == 0);
        if (B.branch) { goto source_c442_m0x1; }
        goto source_c448_m0x1;
source_c448_m0x1: /* $80:C448 [M=0 X=1] LDA   $1E */
        READ(0xc448, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c44a_m0x1;
source_c44a_m0x1: /* $80:C44A [M=0 X=1] JMP   ($BFB1,X) */
        READ(0xc44a, 3, (0x800000u | (uint16_t)(0xbfb1u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfdb: goto source_bfdb_m0x1;
        case 0xc028: goto source_c028_m0x1;
        case 0xc076: goto source_c076_m0x1;
        case 0xc0c5: goto source_c0c5_m0x1;
        case 0xc115: goto source_c115_m0x1;
        case 0xc166: goto source_c166_m0x1;
        case 0xc1b8: goto source_c1b8_m0x1;
        case 0xc20b: goto source_c20b_m0x1;
        default: return unsupported(s);
        }
source_c44d_m0x1: /* $80:C44D [M=0 X=1] LDA   $1E */
        READ(0xc44d, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c44f_m0x1;
source_c44f_m0x1: /* $80:C44F [M=0 X=1] BMI   $C467 */
        BRANCH(0xc44f, (P & 0x80u) != 0);
        if (B.branch) { goto source_c467_m0x1; }
        goto source_c451_m0x1;
source_c451_m0x1: /* $80:C451 [M=0 X=1] LDY   #$01 */
        IMM(0xc451, 2);
        set_y(s, 0x0001u);
        goto source_c453_m0x1;
source_c453_m0x1: /* $80:C453 [M=0 X=1] JSR   $C594 */
        PUSH(0xc453, 3, 0xc455, 2);
        goto source_c594_m0x1;
source_c456_m0x1: /* $80:C456 [M=0 X=1] STA   $1E */
        WRITE(0xc456, 2, 0x1eu, 2, false, A);
        goto source_c458_m0x1;
source_c458_m0x1: /* $80:C458 [M=0 X=1] LDY   $1F */
        READ(0xc458, 2, 0x1fu, 1, false);
        set_y(s, B.read_value);
        goto source_c45a_m0x1;
source_c45a_m0x1: /* $80:C45A [M=0 X=1] STY   $2180 */
        WRITE(0xc45a, 3, DB(0x2180u), 1, false, Y);
        goto source_c45d_m0x1;
source_c45d_m0x1: /* $80:C45D [M=0 X=1] STY   $08 */
        WRITE(0xc45d, 2, 0x08u, 1, false, Y);
        goto source_c45f_m0x1;
source_c45f_m0x1: /* $80:C45F [M=0 X=1] LDY   #$08 */
        IMM(0xc45f, 2);
        set_y(s, 0x0008u);
        goto source_c461_m0x1;
source_c461_m0x1: /* $80:C461 [M=0 X=1] JSR   $C594 */
        PUSH(0xc461, 3, 0xc463, 2);
        goto source_c594_m0x1;
source_c464_m0x1: /* $80:C464 [M=0 X=1] JMP   ($BFB1,X) */
        READ(0xc464, 3, (0x800000u | (uint16_t)(0xbfb1u + X)), 2, true);
        switch (B.read_value) {
        case 0xbfdb: goto source_bfdb_m0x1;
        case 0xc028: goto source_c028_m0x1;
        case 0xc076: goto source_c076_m0x1;
        case 0xc0c5: goto source_c0c5_m0x1;
        case 0xc115: goto source_c115_m0x1;
        case 0xc166: goto source_c166_m0x1;
        case 0xc1b8: goto source_c1b8_m0x1;
        case 0xc20b: goto source_c20b_m0x1;
        default: return unsupported(s);
        }
source_c467_m0x1: /* $80:C467 [M=0 X=1] RTS */
        PULL(0xc467, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c468_m0x1: /* $80:C468 [M=0 X=1] STZ   $21 */
        WRITE(0xc468, 2, 0x21u, 2, false, 0);
        goto source_c46a_m0x1;
source_c46a_m0x1: /* $80:C46A [M=0 X=1] LDA   $1E */
        READ(0xc46a, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c46c_m0x1;
source_c46c_m0x1: /* $80:C46C [M=0 X=1] ASL   A */
        IMP(0xc46c);
        accumulator_change(s, FB30_ASL);
        goto source_c46d_m0x1;
source_c46d_m0x1: /* $80:C46D [M=0 X=1] DEY */
        IMP(0xc46d);
        set_y(s, Y - 1u);
        goto source_c46e_m0x1;
source_c46e_m0x1: /* $80:C46E [M=0 X=1] BEQ   $C483 */
        BRANCH(0xc46e, (P & 2u) != 0);
        if (B.branch) { goto source_c483_m0x1; }
        goto source_c470_m0x1;
source_c470_m0x1: /* $80:C470 [M=0 X=1] BCC   $C4AA */
        BRANCH(0xc470, (P & 1u) == 0);
        if (B.branch) { goto source_c4aa_m0x1; }
        goto source_c472_m0x1;
source_c472_m0x1: /* $80:C472 [M=0 X=1] ASL   A */
        IMP(0xc472);
        accumulator_change(s, FB30_ASL);
        goto source_c473_m0x1;
source_c473_m0x1: /* $80:C473 [M=0 X=1] ROL   $21 */
        CHANGE(0xc473, 2, 0x21u, 2, false, FB30_ROL);
        goto source_c475_m0x1;
source_c475_m0x1: /* $80:C475 [M=0 X=1] DEY */
        IMP(0xc475);
        set_y(s, Y - 1u);
        goto source_c476_m0x1;
source_c476_m0x1: /* $80:C476 [M=0 X=1] BEQ   $C48F */
        BRANCH(0xc476, (P & 2u) != 0);
        if (B.branch) { goto source_c48f_m0x1; }
        goto source_c478_m0x1;
source_c478_m0x1: /* $80:C478 [M=0 X=1] ASL   A */
        IMP(0xc478);
        accumulator_change(s, FB30_ASL);
        goto source_c479_m0x1;
source_c479_m0x1: /* $80:C479 [M=0 X=1] ROL   $21 */
        CHANGE(0xc479, 2, 0x21u, 2, false, FB30_ROL);
        goto source_c47b_m0x1;
source_c47b_m0x1: /* $80:C47B [M=0 X=1] DEY */
        IMP(0xc47b);
        set_y(s, Y - 1u);
        goto source_c47c_m0x1;
source_c47c_m0x1: /* $80:C47C [M=0 X=1] BEQ   $C49B */
        BRANCH(0xc47c, (P & 2u) != 0);
        if (B.branch) { goto source_c49b_m0x1; }
        goto source_c47e_m0x1;
source_c47e_m0x1: /* $80:C47E [M=0 X=1] STA   $1E */
        WRITE(0xc47e, 2, 0x1eu, 2, false, A);
        goto source_c480_m0x1;
source_c480_m0x1: /* $80:C480 [M=0 X=1] LDA   $21 */
        READ(0xc480, 2, 0x21u, 2, false);
        set_a(s, B.read_value);
        goto source_c482_m0x1;
source_c482_m0x1: /* $80:C482 [M=0 X=1] RTS */
        PULL(0xc482, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c483_m0x1: /* $80:C483 [M=0 X=1] SEP   #$20 */
        MODE(0xc483);
        status_set(s, P | 0x20u);
        goto source_c485_m1x1;
source_c485_m1x1: /* $80:C485 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc485, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c487_m1x1;
source_c487_m1x1: /* $80:C487 [M=1 X=1] REP   #$20 */
        MODE(0xc487);
        status_set(s, P & 0xdfu);
        goto source_c489_m0x1;
source_c489_m0x1: /* $80:C489 [M=0 X=1] INC   $0C */
        CHANGE(0xc489, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c48b_m0x1;
source_c48b_m0x1: /* $80:C48B [M=0 X=1] LDY   #$08 */
        IMM(0xc48b, 2);
        set_y(s, 0x0008u);
        goto source_c48d_m0x1;
source_c48d_m0x1: /* $80:C48D [M=0 X=1] BRA   $C470 */
        BRANCH(0xc48d, true);
        goto source_c470_m0x1;
source_c48f_m0x1: /* $80:C48F [M=0 X=1] SEP   #$20 */
        MODE(0xc48f);
        status_set(s, P | 0x20u);
        goto source_c491_m1x1;
source_c491_m1x1: /* $80:C491 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc491, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c493_m1x1;
source_c493_m1x1: /* $80:C493 [M=1 X=1] REP   #$20 */
        MODE(0xc493);
        status_set(s, P & 0xdfu);
        goto source_c495_m0x1;
source_c495_m0x1: /* $80:C495 [M=0 X=1] INC   $0C */
        CHANGE(0xc495, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c497_m0x1;
source_c497_m0x1: /* $80:C497 [M=0 X=1] LDY   #$08 */
        IMM(0xc497, 2);
        set_y(s, 0x0008u);
        goto source_c499_m0x1;
source_c499_m0x1: /* $80:C499 [M=0 X=1] BRA   $C478 */
        BRANCH(0xc499, true);
        goto source_c478_m0x1;
source_c49b_m0x1: /* $80:C49B [M=0 X=1] SEP   #$20 */
        MODE(0xc49b);
        status_set(s, P | 0x20u);
        goto source_c49d_m1x1;
source_c49d_m1x1: /* $80:C49D [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc49d, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c49f_m1x1;
source_c49f_m1x1: /* $80:C49F [M=1 X=1] REP   #$20 */
        MODE(0xc49f);
        status_set(s, P & 0xdfu);
        goto source_c4a1_m0x1;
source_c4a1_m0x1: /* $80:C4A1 [M=0 X=1] INC   $0C */
        CHANGE(0xc4a1, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c4a3_m0x1;
source_c4a3_m0x1: /* $80:C4A3 [M=0 X=1] LDY   #$08 */
        IMM(0xc4a3, 2);
        set_y(s, 0x0008u);
        goto source_c4a5_m0x1;
source_c4a5_m0x1: /* $80:C4A5 [M=0 X=1] STA   $1E */
        WRITE(0xc4a5, 2, 0x1eu, 2, false, A);
        goto source_c4a7_m0x1;
source_c4a7_m0x1: /* $80:C4A7 [M=0 X=1] LDA   $21 */
        READ(0xc4a7, 2, 0x21u, 2, false);
        set_a(s, B.read_value);
        goto source_c4a9_m0x1;
source_c4a9_m0x1: /* $80:C4A9 [M=0 X=1] RTS */
        PULL(0xc4a9, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c4aa_m0x1: /* $80:C4AA [M=0 X=1] STX   $00 */
        WRITE(0xc4aa, 2, 0x00u, 1, false, X);
        goto source_c4ac_m0x1;
source_c4ac_m0x1: /* $80:C4AC [M=0 X=1] LDX   #$02 */
        IMM(0xc4ac, 2);
        set_x(s, 0x0002u);
        goto source_c4ae_m0x1;
source_c4ae_m0x1: /* $80:C4AE [M=0 X=1] ASL   A */
        IMP(0xc4ae);
        accumulator_change(s, FB30_ASL);
        goto source_c4af_m0x1;
source_c4af_m0x1: /* $80:C4AF [M=0 X=1] DEY */
        IMP(0xc4af);
        set_y(s, Y - 1u);
        goto source_c4b0_m0x1;
source_c4b0_m0x1: /* $80:C4B0 [M=0 X=1] BEQ   $C4D2 */
        BRANCH(0xc4b0, (P & 2u) != 0);
        if (B.branch) { goto source_c4d2_m0x1; }
        goto source_c4b2_m0x1;
source_c4b2_m0x1: /* $80:C4B2 [M=0 X=1] INX */
        IMP(0xc4b2);
        set_x(s, X + 1u);
        goto source_c4b3_m0x1;
source_c4b3_m0x1: /* $80:C4B3 [M=0 X=1] BCC   $C4AE */
        BRANCH(0xc4b3, (P & 1u) == 0);
        if (B.branch) { goto source_c4ae_m0x1; }
        goto source_c4b5_m0x1;
source_c4b5_m0x1: /* $80:C4B5 [M=0 X=1] STX   $04 */
        WRITE(0xc4b5, 2, 0x04u, 1, false, X);
        goto source_c4b7_m0x1;
source_c4b7_m0x1: /* $80:C4B7 [M=0 X=1] ASL   A */
        IMP(0xc4b7);
        accumulator_change(s, FB30_ASL);
        goto source_c4b8_m0x1;
source_c4b8_m0x1: /* $80:C4B8 [M=0 X=1] ROL   $21 */
        CHANGE(0xc4b8, 2, 0x21u, 2, false, FB30_ROL);
        goto source_c4ba_m0x1;
source_c4ba_m0x1: /* $80:C4BA [M=0 X=1] DEY */
        IMP(0xc4ba);
        set_y(s, Y - 1u);
        goto source_c4bb_m0x1;
source_c4bb_m0x1: /* $80:C4BB [M=0 X=1] BEQ   $C4DE */
        BRANCH(0xc4bb, (P & 2u) != 0);
        if (B.branch) { goto source_c4de_m0x1; }
        goto source_c4bd_m0x1;
source_c4bd_m0x1: /* $80:C4BD [M=0 X=1] DEX */
        IMP(0xc4bd);
        set_x(s, X - 1u);
        goto source_c4be_m0x1;
source_c4be_m0x1: /* $80:C4BE [M=0 X=1] BNE   $C4B7 */
        BRANCH(0xc4be, (P & 2u) == 0);
        if (B.branch) { goto source_c4b7_m0x1; }
        goto source_c4c0_m0x1;
source_c4c0_m0x1: /* $80:C4C0 [M=0 X=1] STA   $1E */
        WRITE(0xc4c0, 2, 0x1eu, 2, false, A);
        goto source_c4c2_m0x1;
source_c4c2_m0x1: /* $80:C4C2 [M=0 X=1] ASL   $04 */
        CHANGE(0xc4c2, 2, 0x04u, 2, false, FB30_ASL);
        goto source_c4c4_m0x1;
source_c4c4_m0x1: /* $80:C4C4 [M=0 X=1] LDX   $04 */
        READ(0xc4c4, 2, 0x04u, 1, false);
        set_x(s, B.read_value);
        goto source_c4c6_m0x1;
source_c4c6_m0x1: /* $80:C4C6 [M=0 X=1] LDA   $80C56E,X */
        READ(0xc4c6, 4, (0x80c56eu + X), 2, false);
        set_a(s, B.read_value);
        goto source_c4ca_m0x1;
source_c4ca_m0x1: /* $80:C4CA [M=0 X=1] LDX   $00 */
        READ(0xc4ca, 2, 0x00u, 1, false);
        set_x(s, B.read_value);
        goto source_c4cc_m0x1;
source_c4cc_m0x1: /* $80:C4CC [M=0 X=1] CLC */
        IMP(0xc4cc);
        P = (uint8_t)(P & 0xfeu);
        goto source_c4cd_m0x1;
source_c4cd_m0x1: /* $80:C4CD [M=0 X=1] ADC   $21 */
        READ(0xc4cd, 2, 0x21u, 2, false);
        add_sub(s, B.read_value, false);
        goto source_c4cf_m0x1;
source_c4cf_m0x1: /* $80:C4CF [M=0 X=1] STA   $21 */
        WRITE(0xc4cf, 2, 0x21u, 2, false, A);
        goto source_c4d1_m0x1;
source_c4d1_m0x1: /* $80:C4D1 [M=0 X=1] RTS */
        PULL(0xc4d1, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c4d2_m0x1: /* $80:C4D2 [M=0 X=1] SEP   #$20 */
        MODE(0xc4d2);
        status_set(s, P | 0x20u);
        goto source_c4d4_m1x1;
source_c4d4_m1x1: /* $80:C4D4 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc4d4, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c4d6_m1x1;
source_c4d6_m1x1: /* $80:C4D6 [M=1 X=1] REP   #$20 */
        MODE(0xc4d6);
        status_set(s, P & 0xdfu);
        goto source_c4d8_m0x1;
source_c4d8_m0x1: /* $80:C4D8 [M=0 X=1] INC   $0C */
        CHANGE(0xc4d8, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c4da_m0x1;
source_c4da_m0x1: /* $80:C4DA [M=0 X=1] LDY   #$08 */
        IMM(0xc4da, 2);
        set_y(s, 0x0008u);
        goto source_c4dc_m0x1;
source_c4dc_m0x1: /* $80:C4DC [M=0 X=1] BRA   $C4B2 */
        BRANCH(0xc4dc, true);
        goto source_c4b2_m0x1;
source_c4de_m0x1: /* $80:C4DE [M=0 X=1] SEP   #$20 */
        MODE(0xc4de);
        status_set(s, P | 0x20u);
        goto source_c4e0_m1x1;
source_c4e0_m1x1: /* $80:C4E0 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc4e0, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c4e2_m1x1;
source_c4e2_m1x1: /* $80:C4E2 [M=1 X=1] REP   #$20 */
        MODE(0xc4e2);
        status_set(s, P & 0xdfu);
        goto source_c4e4_m0x1;
source_c4e4_m0x1: /* $80:C4E4 [M=0 X=1] INC   $0C */
        CHANGE(0xc4e4, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c4e6_m0x1;
source_c4e6_m0x1: /* $80:C4E6 [M=0 X=1] LDY   #$08 */
        IMM(0xc4e6, 2);
        set_y(s, 0x0008u);
        goto source_c4e8_m0x1;
source_c4e8_m0x1: /* $80:C4E8 [M=0 X=1] BRA   $C4BD */
        BRANCH(0xc4e8, true);
        goto source_c4bd_m0x1;
source_c4ea_m0x1: /* $80:C4EA [M=0 X=1] STZ   $21 */
        WRITE(0xc4ea, 2, 0x21u, 2, false, 0);
        goto source_c4ec_m0x1;
source_c4ec_m0x1: /* $80:C4EC [M=0 X=1] LDA   $1E */
        READ(0xc4ec, 2, 0x1eu, 2, false);
        set_a(s, B.read_value);
        goto source_c4ee_m0x1;
source_c4ee_m0x1: /* $80:C4EE [M=0 X=1] ASL   A */
        IMP(0xc4ee);
        accumulator_change(s, FB30_ASL);
        goto source_c4ef_m0x1;
source_c4ef_m0x1: /* $80:C4EF [M=0 X=1] DEX */
        IMP(0xc4ef);
        set_x(s, X - 1u);
        goto source_c4f0_m0x1;
source_c4f0_m0x1: /* $80:C4F0 [M=0 X=1] DEX */
        IMP(0xc4f0);
        set_x(s, X - 1u);
        goto source_c4f1_m0x1;
source_c4f1_m0x1: /* $80:C4F1 [M=0 X=1] BEQ   $C508 */
        BRANCH(0xc4f1, (P & 2u) != 0);
        if (B.branch) { goto source_c508_m0x1; }
        goto source_c4f3_m0x1;
source_c4f3_m0x1: /* $80:C4F3 [M=0 X=1] BCC   $C52F */
        BRANCH(0xc4f3, (P & 1u) == 0);
        if (B.branch) { goto source_c52f_m0x1; }
        goto source_c4f5_m0x1;
source_c4f5_m0x1: /* $80:C4F5 [M=0 X=1] ASL   A */
        IMP(0xc4f5);
        accumulator_change(s, FB30_ASL);
        goto source_c4f6_m0x1;
source_c4f6_m0x1: /* $80:C4F6 [M=0 X=1] ROL   $21 */
        CHANGE(0xc4f6, 2, 0x21u, 2, false, FB30_ROL);
        goto source_c4f8_m0x1;
source_c4f8_m0x1: /* $80:C4F8 [M=0 X=1] DEX */
        IMP(0xc4f8);
        set_x(s, X - 1u);
        goto source_c4f9_m0x1;
source_c4f9_m0x1: /* $80:C4F9 [M=0 X=1] DEX */
        IMP(0xc4f9);
        set_x(s, X - 1u);
        goto source_c4fa_m0x1;
source_c4fa_m0x1: /* $80:C4FA [M=0 X=1] BEQ   $C514 */
        BRANCH(0xc4fa, (P & 2u) != 0);
        if (B.branch) { goto source_c514_m0x1; }
        goto source_c4fc_m0x1;
source_c4fc_m0x1: /* $80:C4FC [M=0 X=1] ASL   A */
        IMP(0xc4fc);
        accumulator_change(s, FB30_ASL);
        goto source_c4fd_m0x1;
source_c4fd_m0x1: /* $80:C4FD [M=0 X=1] ROL   $21 */
        CHANGE(0xc4fd, 2, 0x21u, 2, false, FB30_ROL);
        goto source_c4ff_m0x1;
source_c4ff_m0x1: /* $80:C4FF [M=0 X=1] DEX */
        IMP(0xc4ff);
        set_x(s, X - 1u);
        goto source_c500_m0x1;
source_c500_m0x1: /* $80:C500 [M=0 X=1] DEX */
        IMP(0xc500);
        set_x(s, X - 1u);
        goto source_c501_m0x1;
source_c501_m0x1: /* $80:C501 [M=0 X=1] BEQ   $C520 */
        BRANCH(0xc501, (P & 2u) != 0);
        if (B.branch) { goto source_c520_m0x1; }
        goto source_c503_m0x1;
source_c503_m0x1: /* $80:C503 [M=0 X=1] STA   $1E */
        WRITE(0xc503, 2, 0x1eu, 2, false, A);
        goto source_c505_m0x1;
source_c505_m0x1: /* $80:C505 [M=0 X=1] LDA   $21 */
        READ(0xc505, 2, 0x21u, 2, false);
        set_a(s, B.read_value);
        goto source_c507_m0x1;
source_c507_m0x1: /* $80:C507 [M=0 X=1] RTS */
        PULL(0xc507, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c508_m0x1: /* $80:C508 [M=0 X=1] SEP   #$20 */
        MODE(0xc508);
        status_set(s, P | 0x20u);
        goto source_c50a_m1x1;
source_c50a_m1x1: /* $80:C50A [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc50a, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c50c_m1x1;
source_c50c_m1x1: /* $80:C50C [M=1 X=1] REP   #$20 */
        MODE(0xc50c);
        status_set(s, P & 0xdfu);
        goto source_c50e_m0x1;
source_c50e_m0x1: /* $80:C50E [M=0 X=1] INC   $0C */
        CHANGE(0xc50e, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c510_m0x1;
source_c510_m0x1: /* $80:C510 [M=0 X=1] LDX   #$10 */
        IMM(0xc510, 2);
        set_x(s, 0x0010u);
        goto source_c512_m0x1;
source_c512_m0x1: /* $80:C512 [M=0 X=1] BRA   $C4F3 */
        BRANCH(0xc512, true);
        goto source_c4f3_m0x1;
source_c514_m0x1: /* $80:C514 [M=0 X=1] SEP   #$20 */
        MODE(0xc514);
        status_set(s, P | 0x20u);
        goto source_c516_m1x1;
source_c516_m1x1: /* $80:C516 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc516, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c518_m1x1;
source_c518_m1x1: /* $80:C518 [M=1 X=1] REP   #$20 */
        MODE(0xc518);
        status_set(s, P & 0xdfu);
        goto source_c51a_m0x1;
source_c51a_m0x1: /* $80:C51A [M=0 X=1] INC   $0C */
        CHANGE(0xc51a, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c51c_m0x1;
source_c51c_m0x1: /* $80:C51C [M=0 X=1] LDX   #$10 */
        IMM(0xc51c, 2);
        set_x(s, 0x0010u);
        goto source_c51e_m0x1;
source_c51e_m0x1: /* $80:C51E [M=0 X=1] BRA   $C4FC */
        BRANCH(0xc51e, true);
        goto source_c4fc_m0x1;
source_c520_m0x1: /* $80:C520 [M=0 X=1] SEP   #$20 */
        MODE(0xc520);
        status_set(s, P | 0x20u);
        goto source_c522_m1x1;
source_c522_m1x1: /* $80:C522 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc522, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c524_m1x1;
source_c524_m1x1: /* $80:C524 [M=1 X=1] REP   #$20 */
        MODE(0xc524);
        status_set(s, P & 0xdfu);
        goto source_c526_m0x1;
source_c526_m0x1: /* $80:C526 [M=0 X=1] INC   $0C */
        CHANGE(0xc526, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c528_m0x1;
source_c528_m0x1: /* $80:C528 [M=0 X=1] LDX   #$10 */
        IMM(0xc528, 2);
        set_x(s, 0x0010u);
        goto source_c52a_m0x1;
source_c52a_m0x1: /* $80:C52A [M=0 X=1] STA   $1E */
        WRITE(0xc52a, 2, 0x1eu, 2, false, A);
        goto source_c52c_m0x1;
source_c52c_m0x1: /* $80:C52C [M=0 X=1] LDA   $21 */
        READ(0xc52c, 2, 0x21u, 2, false);
        set_a(s, B.read_value);
        goto source_c52e_m0x1;
source_c52e_m0x1: /* $80:C52E [M=0 X=1] RTS */
        PULL(0xc52e, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c52f_m0x1: /* $80:C52F [M=0 X=1] LDY   #$02 */
        IMM(0xc52f, 2);
        set_y(s, 0x0002u);
        goto source_c531_m0x1;
source_c531_m0x1: /* $80:C531 [M=0 X=1] ASL   A */
        IMP(0xc531);
        accumulator_change(s, FB30_ASL);
        goto source_c532_m0x1;
source_c532_m0x1: /* $80:C532 [M=0 X=1] DEX */
        IMP(0xc532);
        set_x(s, X - 1u);
        goto source_c533_m0x1;
source_c533_m0x1: /* $80:C533 [M=0 X=1] DEX */
        IMP(0xc533);
        set_x(s, X - 1u);
        goto source_c534_m0x1;
source_c534_m0x1: /* $80:C534 [M=0 X=1] BEQ   $C55A */
        BRANCH(0xc534, (P & 2u) != 0);
        if (B.branch) { goto source_c55a_m0x1; }
        goto source_c536_m0x1;
source_c536_m0x1: /* $80:C536 [M=0 X=1] INY */
        IMP(0xc536);
        set_y(s, Y + 1u);
        goto source_c537_m0x1;
source_c537_m0x1: /* $80:C537 [M=0 X=1] BCC   $C531 */
        BRANCH(0xc537, (P & 1u) == 0);
        if (B.branch) { goto source_c531_m0x1; }
        goto source_c539_m0x1;
source_c539_m0x1: /* $80:C539 [M=0 X=1] STY   $14 */
        WRITE(0xc539, 2, 0x14u, 1, false, Y);
        goto source_c53b_m0x1;
source_c53b_m0x1: /* $80:C53B [M=0 X=1] ASL   A */
        IMP(0xc53b);
        accumulator_change(s, FB30_ASL);
        goto source_c53c_m0x1;
source_c53c_m0x1: /* $80:C53C [M=0 X=1] ROL   $21 */
        CHANGE(0xc53c, 2, 0x21u, 2, false, FB30_ROL);
        goto source_c53e_m0x1;
source_c53e_m0x1: /* $80:C53E [M=0 X=1] DEX */
        IMP(0xc53e);
        set_x(s, X - 1u);
        goto source_c53f_m0x1;
source_c53f_m0x1: /* $80:C53F [M=0 X=1] DEX */
        IMP(0xc53f);
        set_x(s, X - 1u);
        goto source_c540_m0x1;
source_c540_m0x1: /* $80:C540 [M=0 X=1] BEQ   $C566 */
        BRANCH(0xc540, (P & 2u) != 0);
        if (B.branch) { goto source_c566_m0x1; }
        goto source_c542_m0x1;
source_c542_m0x1: /* $80:C542 [M=0 X=1] DEY */
        IMP(0xc542);
        set_y(s, Y - 1u);
        goto source_c543_m0x1;
source_c543_m0x1: /* $80:C543 [M=0 X=1] BNE   $C53B */
        BRANCH(0xc543, (P & 2u) == 0);
        if (B.branch) { goto source_c53b_m0x1; }
        goto source_c545_m0x1;
source_c545_m0x1: /* $80:C545 [M=0 X=1] STA   $1E */
        WRITE(0xc545, 2, 0x1eu, 2, false, A);
        goto source_c547_m0x1;
source_c547_m0x1: /* $80:C547 [M=0 X=1] TXY */
        IMP(0xc547);
        set_y(s, X);
        goto source_c548_m0x1;
source_c548_m0x1: /* $80:C548 [M=0 X=1] LDA   $14 */
        READ(0xc548, 2, 0x14u, 2, false);
        set_a(s, B.read_value);
        goto source_c54a_m0x1;
source_c54a_m0x1: /* $80:C54A [M=0 X=1] AND   #$FF */
        IMM(0xc54a, 3);
        set_a(s, A & 0x00ffu);
        goto source_c54d_m0x1;
source_c54d_m0x1: /* $80:C54D [M=0 X=1] ASL   A */
        IMP(0xc54d);
        accumulator_change(s, FB30_ASL);
        goto source_c54e_m0x1;
source_c54e_m0x1: /* $80:C54E [M=0 X=1] TAX */
        IMP(0xc54e);
        set_x(s, A);
        goto source_c54f_m0x1;
source_c54f_m0x1: /* $80:C54F [M=0 X=1] LDA   $80C56E,X */
        READ(0xc54f, 4, (0x80c56eu + X), 2, false);
        set_a(s, B.read_value);
        goto source_c553_m0x1;
source_c553_m0x1: /* $80:C553 [M=0 X=1] TYX */
        IMP(0xc553);
        set_x(s, Y);
        goto source_c554_m0x1;
source_c554_m0x1: /* $80:C554 [M=0 X=1] CLC */
        IMP(0xc554);
        P = (uint8_t)(P & 0xfeu);
        goto source_c555_m0x1;
source_c555_m0x1: /* $80:C555 [M=0 X=1] ADC   $21 */
        READ(0xc555, 2, 0x21u, 2, false);
        add_sub(s, B.read_value, false);
        goto source_c557_m0x1;
source_c557_m0x1: /* $80:C557 [M=0 X=1] STA   $21 */
        WRITE(0xc557, 2, 0x21u, 2, false, A);
        goto source_c559_m0x1;
source_c559_m0x1: /* $80:C559 [M=0 X=1] RTS */
        PULL(0xc559, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c55a_m0x1: /* $80:C55A [M=0 X=1] SEP   #$20 */
        MODE(0xc55a);
        status_set(s, P | 0x20u);
        goto source_c55c_m1x1;
source_c55c_m1x1: /* $80:C55C [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc55c, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c55e_m1x1;
source_c55e_m1x1: /* $80:C55E [M=1 X=1] REP   #$20 */
        MODE(0xc55e);
        status_set(s, P & 0xdfu);
        goto source_c560_m0x1;
source_c560_m0x1: /* $80:C560 [M=0 X=1] INC   $0C */
        CHANGE(0xc560, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c562_m0x1;
source_c562_m0x1: /* $80:C562 [M=0 X=1] LDX   #$10 */
        IMM(0xc562, 2);
        set_x(s, 0x0010u);
        goto source_c564_m0x1;
source_c564_m0x1: /* $80:C564 [M=0 X=1] BRA   $C536 */
        BRANCH(0xc564, true);
        goto source_c536_m0x1;
source_c566_m0x1: /* $80:C566 [M=0 X=1] SEP   #$20 */
        MODE(0xc566);
        status_set(s, P | 0x20u);
        goto source_c568_m1x1;
source_c568_m1x1: /* $80:C568 [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc568, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c56a_m1x1;
source_c56a_m1x1: /* $80:C56A [M=1 X=1] REP   #$20 */
        MODE(0xc56a);
        status_set(s, P & 0xdfu);
        goto source_c56c_m0x1;
source_c56c_m0x1: /* $80:C56C [M=0 X=1] INC   $0C */
        CHANGE(0xc56c, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c56e_m0x1;
source_c56e_m0x1: /* $80:C56E [M=0 X=1] LDX   #$10 */
        IMM(0xc56e, 2);
        set_x(s, 0x0010u);
        goto source_c570_m0x1;
source_c570_m0x1: /* $80:C570 [M=0 X=1] BRA   $C542 */
        BRANCH(0xc570, true);
        goto source_c542_m0x1;
source_c594_m0x1: /* $80:C594 [M=0 X=1] ASL   A */
        IMP(0xc594);
        accumulator_change(s, FB30_ASL);
        goto source_c595_m0x1;
source_c595_m0x1: /* $80:C595 [M=0 X=1] DEX */
        IMP(0xc595);
        set_x(s, X - 1u);
        goto source_c596_m0x1;
source_c596_m0x1: /* $80:C596 [M=0 X=1] DEX */
        IMP(0xc596);
        set_x(s, X - 1u);
        goto source_c597_m0x1;
source_c597_m0x1: /* $80:C597 [M=0 X=1] BEQ   $C59D */
        BRANCH(0xc597, (P & 2u) != 0);
        if (B.branch) { goto source_c59d_m0x1; }
        goto source_c599_m0x1;
source_c599_m0x1: /* $80:C599 [M=0 X=1] DEY */
        IMP(0xc599);
        set_y(s, Y - 1u);
        goto source_c59a_m0x1;
source_c59a_m0x1: /* $80:C59A [M=0 X=1] BNE   $C594 */
        BRANCH(0xc59a, (P & 2u) == 0);
        if (B.branch) { goto source_c594_m0x1; }
        goto source_c59c_m0x1;
source_c59c_m0x1: /* $80:C59C [M=0 X=1] RTS */
        PULL(0xc59c, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c59d_m0x1: /* $80:C59D [M=0 X=1] SEP   #$20 */
        MODE(0xc59d);
        status_set(s, P | 0x20u);
        goto source_c59f_m1x1;
source_c59f_m1x1: /* $80:C59F [M=1 X=1] LDA   ($0C) */
        INDIRECT(0xc59f, 0x0c, 1);
        set_a(s, B.read_value);
        goto source_c5a1_m1x1;
source_c5a1_m1x1: /* $80:C5A1 [M=1 X=1] REP   #$20 */
        MODE(0xc5a1);
        status_set(s, P & 0xdfu);
        goto source_c5a3_m0x1;
source_c5a3_m0x1: /* $80:C5A3 [M=0 X=1] INC   $0C */
        CHANGE(0xc5a3, 2, 0x0cu, 2, false, FB30_INC);
        goto source_c5a5_m0x1;
source_c5a5_m0x1: /* $80:C5A5 [M=0 X=1] LDX   #$10 */
        IMM(0xc5a5, 2);
        set_x(s, 0x0010u);
        goto source_c5a7_m0x1;
source_c5a7_m0x1: /* $80:C5A7 [M=0 X=1] DEY */
        IMP(0xc5a7);
        set_y(s, Y - 1u);
        goto source_c5a8_m0x1;
source_c5a8_m0x1: /* $80:C5A8 [M=0 X=1] BNE   $C594 */
        BRANCH(0xc5a8, (P & 2u) == 0);
        if (B.branch) { goto source_c594_m0x1; }
        goto source_c5aa_m0x1;
source_c5aa_m0x1: /* $80:C5AA [M=0 X=1] RTS */
        PULL(0xc5aa, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xbea2: goto source_bea2_m0x1;
        case 0xbef5: goto source_bef5_m0x1;
        case 0xc420: goto source_c420_m0x1;
        case 0xc425: goto source_c425_m0x1;
        case 0xc439: goto source_c439_m0x1;
        case 0xc43e: goto source_c43e_m0x1;
        case 0xc456: goto source_c456_m0x1;
        case 0xc464: goto source_c464_m0x1;
        case 0xc66a: goto source_c66a_m0x1;
        default: return unsupported(s);
        }
source_c62b_m0x0: /* $80:C62B [M=0 X=0] PHP */
        PUSH(0xc62b, 1, P, 1);
        goto source_c62c_m0x0;
source_c62c_m0x0: /* $80:C62C [M=0 X=0] PHB */
        PUSH(0xc62c, 1, B.registers.data_bank, 1);
        goto source_c62d_m0x0;
source_c62d_m0x0: /* $80:C62D [M=0 X=0] REP   #$30 */
        MODE(0xc62d);
        status_set(s, P & 0xcfu);
        goto source_c62f_m0x0;
source_c62f_m0x0: /* $80:C62F [M=0 X=0] JSL   $8086DA */
        SUSPEND(jsl_queue(s));
        B.registers.stack_pointer -= 3u;
        goto source_86da_m0x0;
source_c633_m0x0: /* $80:C633 [M=0 X=0] LDA   $11 */
        READ(0xc633, 2, 0x11u, 2, false);
        set_a(s, B.read_value);
        goto source_c635_m0x0;
source_c635_m0x0: /* $80:C635 [M=0 X=0] STA   $2182 */
        WRITE(0xc635, 3, DB(0x2182u), 2, false, A);
        goto source_c638_m0x0;
source_c638_m0x0: /* $80:C638 [M=0 X=0] LDA   $10 */
        READ(0xc638, 2, 0x10u, 2, false);
        set_a(s, B.read_value);
        goto source_c63a_m0x0;
source_c63a_m0x0: /* $80:C63A [M=0 X=0] STA   $2181 */
        WRITE(0xc63a, 3, DB(0x2181u), 2, false, A);
        goto source_c63d_m0x0;
source_c63d_m0x0: /* $80:C63D [M=0 X=0] LDA   $0D */
        READ(0xc63d, 2, 0x0du, 2, false);
        set_a(s, B.read_value);
        goto source_c63f_m0x0;
source_c63f_m0x0: /* $80:C63F [M=0 X=0] PHA */
        PUSH(0xc63f, 1, A, 2);
        goto source_c640_m0x0;
source_c640_m0x0: /* $80:C640 [M=0 X=0] PLB */
        PULL(0xc640, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_c641_m0x0;
source_c641_m0x0: /* $80:C641 [M=0 X=0] PLB */
        PULL(0xc641, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_c642_m0x0;
source_c642_m0x0: /* $80:C642 [M=0 X=0] LDY   $0C */
        READ(0xc642, 2, 0x0cu, 2, false);
        set_y(s, B.read_value);
        goto source_c644_m0x0;
source_c644_m0x0: /* $80:C644 [M=0 X=0] LDA   $0003,Y */
        READ(0xc644, 3, DB(0x0003u + Y), 2, index_idle(s, 0x0003u, Y));
        set_a(s, B.read_value);
        goto source_c647_m0x0;
source_c647_m0x0: /* $80:C647 [M=0 X=0] XBA */
        SUSPEND(implied(s, 0xc647, 1, 2));
        A = (uint16_t)((A << 8) | (A >> 8));
        nz(s, A, 1);
        goto source_c648_m0x0;
source_c648_m0x0: /* $80:C648 [M=0 X=0] PHA */
        PUSH(0xc648, 1, A, 2);
        goto source_c649_m0x0;
source_c649_m0x0: /* $80:C649 [M=0 X=0] LDA   $0000,Y */
        READ(0xc649, 3, DB(0x0000u + Y), 2, index_idle(s, 0x0000u, Y));
        set_a(s, B.read_value);
        goto source_c64c_m0x0;
source_c64c_m0x0: /* $80:C64C [M=0 X=0] INY */
        IMP(0xc64c);
        set_y(s, Y + 1u);
        goto source_c64d_m0x0;
source_c64d_m0x0: /* $80:C64D [M=0 X=0] INY */
        IMP(0xc64d);
        set_y(s, Y + 1u);
        goto source_c64e_m0x0;
source_c64e_m0x0: /* $80:C64E [M=0 X=0] CMP   #$FB10 */
        IMM(0xc64e, 3);
        compare(s, A, 0xfb10u, 2);
        goto source_c651_m0x0;
source_c651_m0x0: /* $80:C651 [M=0 X=0] BNE   $C658 */
        BRANCH(0xc651, (P & 2u) == 0);
        if (B.branch) { goto source_c658_m0x0; }
        goto source_c653_m0x0;
source_c653_m0x0: /* $80:C653 [M=0 X=0] JSR   $BB93 */
        return unsupported(s);
source_c658_m0x0: /* $80:C658 [M=0 X=0] CMP   #$FB46 */
        IMM(0xc658, 3);
        compare(s, A, 0xfb46u, 2);
        goto source_c65b_m0x0;
source_c65b_m0x0: /* $80:C65B [M=0 X=0] BNE   $C662 */
        BRANCH(0xc65b, (P & 2u) == 0);
        if (B.branch) { goto source_c662_m0x0; }
        goto source_c65d_m0x0;
source_c65d_m0x0: /* $80:C65D [M=0 X=0] JSR   $BD1B */
        return unsupported(s);
source_c662_m0x0: /* $80:C662 [M=0 X=0] CMP   #$FB30 */
        IMM(0xc662, 3);
        compare(s, A, 0xfb30u, 2);
        goto source_c665_m0x0;
source_c665_m0x0: /* $80:C665 [M=0 X=0] BNE   $C66C */
        BRANCH(0xc665, (P & 2u) == 0);
        if (B.branch) { goto source_c66c_m0x0; }
        goto source_c667_m0x0;
source_c667_m0x0: /* $80:C667 [M=0 X=0] JSR   $BE6B */
        PUSH(0xc667, 3, 0xc669, 2);
        goto source_be6b_m0x0;
source_c66a_m0x1: /* $80:C66A [M=0 X=1] BRA   $C67D */
        BRANCH(0xc66a, true);
        goto source_c67d_m0x1;
source_c66c_m0x0: /* $80:C66C [M=0 X=0] CMP   #$FB7A */
        IMM(0xc66c, 3);
        compare(s, A, 0xfb7au, 2);
        goto source_c66f_m0x0;
source_c66f_m0x0: /* $80:C66F [M=0 X=0] BNE   $C676 */
        BRANCH(0xc66f, (P & 2u) == 0);
        if (B.branch) { goto source_c676_m0x0; }
        goto source_c671_m0x0;
source_c671_m0x0: /* $80:C671 [M=0 X=0] JSR   $C5AB */
        return unsupported(s);
source_c676_m0x0: /* $80:C676 [M=0 X=0] PLX */
        PULL(0xc676, 2, false);
        set_x(s, B.read_value);
        goto source_c677_m0x0;
source_c677_m0x0: /* $80:C677 [M=0 X=0] LDX   #$01 */
        IMM(0xc677, 3);
        set_x(s, 0x0001u);
        goto source_c67a_m0x0;
source_c67a_m0x0: /* $80:C67A [M=0 X=0] PLB */
        PULL(0xc67a, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_c67b_m0x0;
source_c67b_m0x0: /* $80:C67B [M=0 X=0] PLP */
        PULL(0xc67b, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_c67c_m0x0;
source_c67c_m0x0: /* $80:C67C [M=0 X=0] RTL */
        return unsupported(s);
source_c67d_m0x1: /* $80:C67D [M=0 X=1] REP   #$30 */
        MODE(0xc67d);
        status_set(s, P & 0xcfu);
        goto source_c67f_m0x0;
source_c67f_m0x0: /* $80:C67F [M=0 X=0] PLX */
        PULL(0xc67f, 2, false);
        set_x(s, B.read_value);
        goto source_c680_m0x0;
source_c680_m0x0: /* $80:C680 [M=0 X=0] PLB */
        PULL(0xc680, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_c681_m0x0;
source_c681_m0x0: /* $80:C681 [M=0 X=0] PLP */
        PULL(0xc681, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_c682_m0x0;
source_c682_m0x0: /* $80:C682 [M=0 X=0] RTL */
        B.output_size = X;
        B.status = NBA_CODEC_WORK_DONE;
        return false;
    default: return unsupported(s);
    }
}
