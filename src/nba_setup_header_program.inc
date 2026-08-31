/* Generated from canonical ROM only by tools/generate_setup_header_work.py.
 * No capture, fitted cost, opcode dispatcher or runtime ROM execution.
 * Each label is static C source flow; SUSPEND labels are C continuations. */
static bool advance(NbaSetupHeaderWork *s)
{
    switch (B.resume) {
    case 0: goto source_eec6_m0x0;
source_8a02_m0x0: /* $80:8A02 [M=0 X=0] PHP */
        PUSH(0x8a02, 1, P, 1);
        goto source_8a03_m0x0;
source_8a03_m0x0: /* $80:8A03 [M=0 X=0] SEP   #$20 */
        MODE(0x8a03);
        status_set(s, P | 0x20u);
        goto source_8a05_m1x0;
source_8a05_m1x0: /* $80:8A05 [M=1 X=0] PHA */
        PUSH(0x8a05, 1, A, 1);
        goto source_8a06_m1x0;
source_8a06_m1x0: /* $80:8A06 [M=1 X=0] PHB */
        PUSH(0x8a06, 1, B.registers.data_bank, 1);
        goto source_8a07_m1x0;
source_8a07_m1x0: /* $80:8A07 [M=1 X=0] PLA */
        PULL(0x8a07, 1, false);
        set_a(s, B.read_value);
        goto source_8a08_m1x0;
source_8a08_m1x0: /* $80:8A08 [M=1 X=0] CMP   #$7E */
        IMM(0x8a08, 2);
        compare(s, A, 0x007eu, 1);
        goto source_8a0a_m1x0;
source_8a0a_m1x0: /* $80:8A0A [M=1 X=0] BEQ   $8A10 */
        BRANCH(0x8a0a, (P & 2u) != 0);
        if (B.branch) { goto source_8a10_m1x0; }
        goto source_8a0c_m1x0;
source_8a0c_m1x0: /* $80:8A0C [M=1 X=0] CMP   #$7F */
        IMM(0x8a0c, 2);
        compare(s, A, 0x007fu, 1);
        goto source_8a0e_m1x0;
source_8a0e_m1x0: /* $80:8A0E [M=1 X=0] BNE   $8A12 */
        BRANCH(0x8a0e, (P & 2u) == 0);
        if (B.branch) { goto source_8a12_m1x0; }
        goto source_8a10_m1x0;
source_8a10_m1x0: /* $80:8A10 [M=1 X=0] PHK */
        PUSH(0x8a10, 1, 0x80, 1);
        goto source_8a11_m1x0;
source_8a11_m1x0: /* $80:8A11 [M=1 X=0] PLB */
        PULL(0x8a11, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_8a12_m1x0;
source_8a12_m1x0: /* $80:8A12 [M=1 X=0] PLA */
        PULL(0x8a12, 1, false);
        set_a(s, B.read_value);
        goto source_8a13_m1x0;
source_8a13_m1x0: /* $80:8A13 [M=1 X=0] PLP */
        PULL(0x8a13, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_8a14_m0x0;
source_8a14_m0x0: /* $80:8A14 [M=0 X=0] SEP   #$20 */
        MODE(0x8a14);
        status_set(s, P | 0x20u);
        goto source_8a16_m1x0;
source_8a16_m1x0: /* $80:8A16 [M=1 X=0] LDA   $0562 */
        READ(0x8a16, 3, DB(0x0562u), 1, false);
        set_a(s, B.read_value);
        goto source_8a19_m1x0;
source_8a19_m1x0: /* $80:8A19 [M=1 X=0] BPL   $8A42 */
        BRANCH(0x8a19, (P & 0x80u) == 0);
        if (B.branch) { return unsupported(s); }
        goto source_8a1b_m1x0;
source_8a1b_m1x0: /* $80:8A1B [M=1 X=0] TYA */
        IMP(0x8a1b);
        set_a(s, Y);
        goto source_8a1c_m1x0;
source_8a1c_m1x0: /* $80:8A1C [M=1 X=0] STA   $2121 */
        WRITE(0x8a1c, 3, DB(0x2121u), 1, false, A);
        goto source_8a1f_m1x0;
source_8a1f_m1x0: /* $80:8A1F [M=1 X=0] REP   #$20 */
        MODE(0x8a1f);
        status_set(s, P & 0xdfu);
        goto source_8a21_m0x0;
source_8a21_m0x0: /* $80:8A21 [M=0 X=0] TXA */
        IMP(0x8a21);
        set_a(s, X);
        goto source_8a22_m0x0;
source_8a22_m0x0: /* $80:8A22 [M=0 X=0] SEP   #$10 */
        MODE(0x8a22);
        status_set(s, P | 0x10u);
        goto source_8a24_m0x1;
source_8a24_m0x1: /* $80:8A24 [M=0 X=1] ASL   A */
        IMP(0x8a24);
        accumulator_change(s, HEADER_ASL);
        goto source_8a25_m0x1;
source_8a25_m0x1: /* $80:8A25 [M=0 X=1] BEQ   $8A3F */
        BRANCH(0x8a25, (P & 2u) != 0);
        if (B.branch) { goto source_8a3f_m0x1; }
        goto source_8a27_m0x1;
source_8a27_m0x1: /* $80:8A27 [M=0 X=1] STA   $4315 */
        WRITE(0x8a27, 3, DB(0x4315u), 2, false, A);
        goto source_8a2a_m0x1;
source_8a2a_m0x1: /* $80:8A2A [M=0 X=1] LDA   #$2200 */
        IMM(0x8a2a, 3);
        set_a(s, 0x2200u);
        goto source_8a2d_m0x1;
source_8a2d_m0x1: /* $80:8A2D [M=0 X=1] STA   $4310 */
        WRITE(0x8a2d, 3, DB(0x4310u), 2, false, A);
        goto source_8a30_m0x1;
source_8a30_m0x1: /* $80:8A30 [M=0 X=1] LDA   $0C */
        READ(0x8a30, 2, 0x0cu, 2, false);
        set_a(s, B.read_value);
        goto source_8a32_m0x1;
source_8a32_m0x1: /* $80:8A32 [M=0 X=1] STA   $4312 */
        WRITE(0x8a32, 3, DB(0x4312u), 2, false, A);
        goto source_8a35_m0x1;
source_8a35_m0x1: /* $80:8A35 [M=0 X=1] LDX   $0E */
        READ(0x8a35, 2, 0x0eu, 1, false);
        set_x(s, B.read_value);
        goto source_8a37_m0x1;
source_8a37_m0x1: /* $80:8A37 [M=0 X=1] STX   $4314 */
        WRITE(0x8a37, 3, DB(0x4314u), 1, false, X);
        goto source_8a3a_m0x1;
source_8a3a_m0x1: /* $80:8A3A [M=0 X=1] LDX   #$02 */
        IMM(0x8a3a, 2);
        set_x(s, 0x0002u);
        goto source_8a3c_m0x1;
source_8a3c_m0x1: /* $80:8A3C [M=0 X=1] STX   $420B */
        WRITE(0x8a3c, 3, DB(0x420bu), 1, false, X);
        goto source_8a3f_m0x1;
source_8a3f_m0x1: /* $80:8A3F [M=0 X=1] REP   #$30 */
        MODE(0x8a3f);
        status_set(s, P & 0xcfu);
        goto source_8a41_m0x0;
source_8a41_m0x0: /* $80:8A41 [M=0 X=0] RTL */
        PULL(0x8a41, 3, false);
        if (s->read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xeee4: goto source_eee4_m0x0;
        case 0xef1a: goto source_ef1a_m0x0;
        default: return unsupported(s);
        }
source_8ad2_m0x0: /* $80:8AD2 [M=0 X=0] PHP */
        PUSH(0x8ad2, 1, P, 1);
        goto source_8ad3_m0x0;
source_8ad3_m0x0: /* $80:8AD3 [M=0 X=0] SEP   #$20 */
        MODE(0x8ad3);
        status_set(s, P | 0x20u);
        goto source_8ad5_m1x0;
source_8ad5_m1x0: /* $80:8AD5 [M=1 X=0] PHA */
        PUSH(0x8ad5, 1, A, 1);
        goto source_8ad6_m1x0;
source_8ad6_m1x0: /* $80:8AD6 [M=1 X=0] PHB */
        PUSH(0x8ad6, 1, B.registers.data_bank, 1);
        goto source_8ad7_m1x0;
source_8ad7_m1x0: /* $80:8AD7 [M=1 X=0] PLA */
        PULL(0x8ad7, 1, false);
        set_a(s, B.read_value);
        goto source_8ad8_m1x0;
source_8ad8_m1x0: /* $80:8AD8 [M=1 X=0] CMP   #$7E */
        IMM(0x8ad8, 2);
        compare(s, A, 0x007eu, 1);
        goto source_8ada_m1x0;
source_8ada_m1x0: /* $80:8ADA [M=1 X=0] BEQ   $8AE0 */
        BRANCH(0x8ada, (P & 2u) != 0);
        if (B.branch) { goto source_8ae0_m1x0; }
        goto source_8adc_m1x0;
source_8adc_m1x0: /* $80:8ADC [M=1 X=0] CMP   #$7F */
        IMM(0x8adc, 2);
        compare(s, A, 0x007fu, 1);
        goto source_8ade_m1x0;
source_8ade_m1x0: /* $80:8ADE [M=1 X=0] BNE   $8AE2 */
        BRANCH(0x8ade, (P & 2u) == 0);
        if (B.branch) { goto source_8ae2_m1x0; }
        goto source_8ae0_m1x0;
source_8ae0_m1x0: /* $80:8AE0 [M=1 X=0] PHK */
        PUSH(0x8ae0, 1, 0x80, 1);
        goto source_8ae1_m1x0;
source_8ae1_m1x0: /* $80:8AE1 [M=1 X=0] PLB */
        PULL(0x8ae1, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_8ae2_m1x0;
source_8ae2_m1x0: /* $80:8AE2 [M=1 X=0] PLA */
        PULL(0x8ae2, 1, false);
        set_a(s, B.read_value);
        goto source_8ae3_m1x0;
source_8ae3_m1x0: /* $80:8AE3 [M=1 X=0] PLP */
        PULL(0x8ae3, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_8ae4_m0x0;
source_8ae4_m0x0: /* $80:8AE4 [M=0 X=0] STA   $16 */
        WRITE(0x8ae4, 2, 0x16u, 2, false, A);
        goto source_8ae6_m0x0;
source_8ae6_m0x0: /* $80:8AE6 [M=0 X=0] LDA   $0561 */
        READ(0x8ae6, 3, DB(0x0561u), 2, false);
        set_a(s, B.read_value);
        goto source_8ae9_m0x0;
source_8ae9_m0x0: /* $80:8AE9 [M=0 X=0] BPL   $8B35 */
        BRANCH(0x8ae9, (P & 0x80u) == 0);
        if (B.branch) { return unsupported(s); }
        goto source_8aeb_m0x0;
source_8aeb_m0x0: /* $80:8AEB [M=0 X=0] TXA */
        IMP(0x8aeb);
        set_a(s, X);
        goto source_8aec_m0x0;
source_8aec_m0x0: /* $80:8AEC [M=0 X=0] PHA */
        PUSH(0x8aec, 1, A, 2);
        goto source_8aed_m0x0;
source_8aed_m0x0: /* $80:8AED [M=0 X=0] PHY */
        PUSH(0x8aed, 1, Y, 2);
        goto source_8aee_m0x0;
source_8aee_m0x0: /* $80:8AEE [M=0 X=0] STA   $4315 */
        WRITE(0x8aee, 3, DB(0x4315u), 2, false, A);
        goto source_8af1_m0x0;
source_8af1_m0x0: /* $80:8AF1 [M=0 X=0] STY   $2116 */
        WRITE(0x8af1, 3, DB(0x2116u), 2, false, Y);
        goto source_8af4_m0x0;
source_8af4_m0x0: /* $80:8AF4 [M=0 X=0] LDY   #$16 */
        IMM(0x8af4, 3);
        set_y(s, 0x0016u);
        goto source_8af7_m0x0;
source_8af7_m0x0: /* $80:8AF7 [M=0 X=0] STY   $4312 */
        WRITE(0x8af7, 3, DB(0x4312u), 2, false, Y);
        goto source_8afa_m0x0;
source_8afa_m0x0: /* $80:8AFA [M=0 X=0] LDA   #$1808 */
        IMM(0x8afa, 3);
        set_a(s, 0x1808u);
        goto source_8afd_m0x0;
source_8afd_m0x0: /* $80:8AFD [M=0 X=0] STA   $4310 */
        WRITE(0x8afd, 3, DB(0x4310u), 2, false, A);
        goto source_8b00_m0x0;
source_8b00_m0x0: /* $80:8B00 [M=0 X=0] SEP   #$20 */
        MODE(0x8b00);
        status_set(s, P | 0x20u);
        goto source_8b02_m1x0;
source_8b02_m1x0: /* $80:8B02 [M=1 X=0] STZ   $2115 */
        WRITE(0x8b02, 3, DB(0x2115u), 1, false, 0);
        goto source_8b05_m1x0;
source_8b05_m1x0: /* $80:8B05 [M=1 X=0] STZ   $4314 */
        WRITE(0x8b05, 3, DB(0x4314u), 1, false, 0);
        goto source_8b08_m1x0;
source_8b08_m1x0: /* $80:8B08 [M=1 X=0] LDA   #$02 */
        IMM(0x8b08, 2);
        set_a(s, 0x0002u);
        goto source_8b0a_m1x0;
source_8b0a_m1x0: /* $80:8B0A [M=1 X=0] STA   $420B */
        WRITE(0x8b0a, 3, DB(0x420bu), 1, false, A);
        goto source_8b0d_m1x0;
source_8b0d_m1x0: /* $80:8B0D [M=1 X=0] REP   #$20 */
        MODE(0x8b0d);
        status_set(s, P & 0xdfu);
        goto source_8b0f_m0x0;
source_8b0f_m0x0: /* $80:8B0F [M=0 X=0] PLY */
        PULL(0x8b0f, 2, false);
        set_y(s, B.read_value);
        goto source_8b10_m0x0;
source_8b10_m0x0: /* $80:8B10 [M=0 X=0] PLA */
        PULL(0x8b10, 2, false);
        set_a(s, B.read_value);
        goto source_8b11_m0x0;
source_8b11_m0x0: /* $80:8B11 [M=0 X=0] STA   $4315 */
        WRITE(0x8b11, 3, DB(0x4315u), 2, false, A);
        goto source_8b14_m0x0;
source_8b14_m0x0: /* $80:8B14 [M=0 X=0] STY   $2116 */
        WRITE(0x8b14, 3, DB(0x2116u), 2, false, Y);
        goto source_8b17_m0x0;
source_8b17_m0x0: /* $80:8B17 [M=0 X=0] LDY   #$17 */
        IMM(0x8b17, 3);
        set_y(s, 0x0017u);
        goto source_8b1a_m0x0;
source_8b1a_m0x0: /* $80:8B1A [M=0 X=0] STY   $4312 */
        WRITE(0x8b1a, 3, DB(0x4312u), 2, false, Y);
        goto source_8b1d_m0x0;
source_8b1d_m0x0: /* $80:8B1D [M=0 X=0] LDA   #$1908 */
        IMM(0x8b1d, 3);
        set_a(s, 0x1908u);
        goto source_8b20_m0x0;
source_8b20_m0x0: /* $80:8B20 [M=0 X=0] STA   $4310 */
        WRITE(0x8b20, 3, DB(0x4310u), 2, false, A);
        goto source_8b23_m0x0;
source_8b23_m0x0: /* $80:8B23 [M=0 X=0] SEP   #$20 */
        MODE(0x8b23);
        status_set(s, P | 0x20u);
        goto source_8b25_m1x0;
source_8b25_m1x0: /* $80:8B25 [M=1 X=0] LDA   #$80 */
        IMM(0x8b25, 2);
        set_a(s, 0x0080u);
        goto source_8b27_m1x0;
source_8b27_m1x0: /* $80:8B27 [M=1 X=0] STA   $2115 */
        WRITE(0x8b27, 3, DB(0x2115u), 1, false, A);
        goto source_8b2a_m1x0;
source_8b2a_m1x0: /* $80:8B2A [M=1 X=0] STZ   $4314 */
        WRITE(0x8b2a, 3, DB(0x4314u), 1, false, 0);
        goto source_8b2d_m1x0;
source_8b2d_m1x0: /* $80:8B2D [M=1 X=0] LDA   #$02 */
        IMM(0x8b2d, 2);
        set_a(s, 0x0002u);
        goto source_8b2f_m1x0;
source_8b2f_m1x0: /* $80:8B2F [M=1 X=0] STA   $420B */
        WRITE(0x8b2f, 3, DB(0x420bu), 1, false, A);
        goto source_8b32_m1x0;
source_8b32_m1x0: /* $80:8B32 [M=1 X=0] REP   #$20 */
        MODE(0x8b32);
        status_set(s, P & 0xdfu);
        goto source_8b34_m0x0;
source_8b34_m0x0: /* $80:8B34 [M=0 X=0] RTL */
        PULL(0x8b34, 3, false);
        if (s->read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xeee4: goto source_eee4_m0x0;
        case 0xef1a: goto source_ef1a_m0x0;
        default: return unsupported(s);
        }
source_eec6_m0x0: /* $80:EEC6 [M=0 X=0] REP   #$30 */
        MODE(0xeec6);
        status_set(s, P & 0xcfu);
        goto source_eec8_m0x0;
source_eec8_m0x0: /* $80:EEC8 [M=0 X=0] LDA   #$00 */
        IMM(0xeec8, 3);
        set_a(s, 0x0000u);
        goto source_eecb_m0x0;
source_eecb_m0x0: /* $80:EECB [M=0 X=0] LDX   #$08 */
        IMM(0xeecb, 3);
        set_x(s, 0x0008u);
        goto source_eece_m0x0;
source_eece_m0x0: /* $80:EECE [M=0 X=0] STA   $15D9,X */
        WRITE(0xeece, 3, DB(0x15d9u + X), 2, true, A);
        goto source_eed1_m0x0;
source_eed1_m0x0: /* $80:EED1 [M=0 X=0] DEX */
        IMP(0xeed1);
        set_x(s, X - 1u);
        goto source_eed2_m0x0;
source_eed2_m0x0: /* $80:EED2 [M=0 X=0] DEX */
        IMP(0xeed2);
        set_x(s, X - 1u);
        goto source_eed3_m0x0;
source_eed3_m0x0: /* $80:EED3 [M=0 X=0] BPL   $EECE */
        BRANCH(0xeed3, (P & 0x80u) == 0);
        if (B.branch) { goto source_eece_m0x0; }
        goto source_eed5_m0x0;
source_eed5_m0x0: /* $80:EED5 [M=0 X=0] LDX   #$1000 */
        IMM(0xeed5, 3);
        set_x(s, 0x1000u);
        goto source_eed8_m0x0;
source_eed8_m0x0: /* $80:EED8 [M=0 X=0] LDA   $7E7E66 */
        READ(0xeed8, 4, 0x7e7e66u, 2, false);
        set_a(s, B.read_value);
        goto source_eedc_m0x0;
source_eedc_m0x0: /* $80:EEDC [M=0 X=0] TAY */
        IMP(0xeedc);
        set_y(s, A);
        goto source_eedd_m0x0;
source_eedd_m0x0: /* $80:EEDD [M=0 X=0] LDA   #$00 */
        IMM(0xeedd, 3);
        set_a(s, 0x0000u);
        goto source_eee0_m0x0;
source_eee0_m0x0: /* $80:EEE0 [M=0 X=0] JSL   $808AD2 */
        SUSPEND(jsl_call(s, 0xeee0));
        B.registers.stack_pointer -= 3u;
        goto source_8ad2_m0x0;
source_eee4_m0x0: /* $80:EEE4 [M=0 X=0] LDA   $16B1 */
        READ(0xeee4, 3, DB(0x16b1u), 2, false);
        set_a(s, B.read_value);
        goto source_eee7_m0x0;
source_eee7_m0x0: /* $80:EEE7 [M=0 X=0] CMP   #$23 */
        IMM(0xeee7, 3);
        compare(s, A, 0x0023u, 2);
        goto source_eeea_m0x0;
source_eeea_m0x0: /* $80:EEEA [M=0 X=0] BCC   $EEEF */
        BRANCH(0xeeea, (P & 1u) == 0);
        if (B.branch) { goto source_eeef_m0x0; }
        goto source_eeec_m0x0;
source_eeec_m0x0: /* $80:EEEC [M=0 X=0] JMP   $EF8E */
        JUMP(0xeeec);
        goto source_ef8e_m0x0;
source_eeef_m0x0: /* $80:EEEF [M=0 X=0] ASL   A */
        IMP(0xeeef);
        accumulator_change(s, HEADER_ASL);
        goto source_eef0_m0x0;
source_eef0_m0x0: /* $80:EEF0 [M=0 X=0] ASL   A */
        IMP(0xeef0);
        accumulator_change(s, HEADER_ASL);
        goto source_eef1_m0x0;
source_eef1_m0x0: /* $80:EEF1 [M=0 X=0] TAX */
        IMP(0xeef1);
        set_x(s, A);
        goto source_eef2_m0x0;
source_eef2_m0x0: /* $80:EEF2 [M=0 X=0] LDA   $80DEDE,X */
        READ(0xeef2, 4, (0x80dedeu + X), 2, false);
        set_a(s, B.read_value);
        goto source_eef6_m0x0;
source_eef6_m0x0: /* $80:EEF6 [M=0 X=0] STA   $0C */
        WRITE(0xeef6, 2, 0x0cu, 2, false, A);
        goto source_eef8_m0x0;
source_eef8_m0x0: /* $80:EEF8 [M=0 X=0] LDA   $80DEE0,X */
        READ(0xeef8, 4, (0x80dee0u + X), 2, false);
        set_a(s, B.read_value);
        goto source_eefc_m0x0;
source_eefc_m0x0: /* $80:EEFC [M=0 X=0] STA   $0E */
        WRITE(0xeefc, 2, 0x0eu, 2, false, A);
        goto source_eefe_m0x0;
source_eefe_m0x0: /* $80:EEFE [M=0 X=0] SEP   #$30 */
        MODE(0xeefe);
        status_set(s, P | 0x30u);
        goto source_ef00_m1x1;
source_ef00_m1x1: /* $80:EF00 [M=1 X=1] LDA   #$8F */
        IMM(0xef00, 2);
        set_a(s, 0x008fu);
        goto source_ef02_m1x1;
source_ef02_m1x1: /* $80:EF02 [M=1 X=1] STA   $002100 */
        WRITE(0xef02, 4, 0x002100u, 1, false, A);
        goto source_ef06_m1x1;
source_ef06_m1x1: /* $80:EF06 [M=1 X=1] REP   #$30 */
        MODE(0xef06);
        status_set(s, P & 0xcfu);
        goto source_ef08_m0x0;
source_ef08_m0x0: /* $80:EF08 [M=0 X=0] LDA   #$72 */
        IMM(0xef08, 3);
        set_a(s, 0x0072u);
        goto source_ef0b_m0x0;
source_ef0b_m0x0: /* $80:EF0B [M=0 X=0] CLC */
        IMP(0xef0b);
        P = (uint8_t)(P & 0xfeu);
        goto source_ef0c_m0x0;
source_ef0c_m0x0: /* $80:EF0C [M=0 X=0] ADC   $0C */
        READ(0xef0c, 2, 0x0cu, 2, false);
        add_sub(s, B.read_value, false);
        goto source_ef0e_m0x0;
source_ef0e_m0x0: /* $80:EF0E [M=0 X=0] STA   $0C */
        WRITE(0xef0e, 2, 0x0cu, 2, false, A);
        goto source_ef10_m0x0;
source_ef10_m0x0: /* $80:EF10 [M=0 X=0] LDX   #$07 */
        IMM(0xef10, 3);
        set_x(s, 0x0007u);
        goto source_ef13_m0x0;
source_ef13_m0x0: /* $80:EF13 [M=0 X=0] LDY   #$59 */
        IMM(0xef13, 3);
        set_y(s, 0x0059u);
        goto source_ef16_m0x0;
source_ef16_m0x0: /* $80:EF16 [M=0 X=0] JSL   $808A02 */
        SUSPEND(jsl_call(s, 0xef16));
        B.registers.stack_pointer -= 3u;
        goto source_8a02_m0x0;
source_ef1a_m0x0: /* $80:EF1A [M=0 X=0] JSL   $8086B0 */
        B.status = NBA_CODEC_WORK_DONE;
        return false;
source_ef8e_m0x0: /* $80:EF8E [M=0 X=0] LDA   #$00 */
        IMM(0xef8e, 3);
        set_a(s, 0x0000u);
        goto source_ef91_m0x0;
source_ef91_m0x0: /* $80:EF91 [M=0 X=0] JMP   $EEEF */
        JUMP(0xef91);
        goto source_eeef_m0x0;
    default: return unsupported(s);
    }
}
