/* Generated from canonical ROM only by tools/generate_setup_producer_work.py.
 * No capture, fitted cost, opcode dispatcher or runtime ROM execution.
 * Each label is static C source flow; SUSPEND labels are C continuations. */
static bool advance(NbaSetupProducerWork *s)
{
    switch (B.resume) {
    case 0: goto source_ec68_m0x0;
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
        if (L.read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xec89: goto source_ec89_m0x0;
        case 0xec98: goto source_ec98_m0x0;
        case 0xecb0: goto source_ecb0_m0x0;
        case 0xecc3: goto source_ecc3_m0x0;
        case 0xecc7: goto source_ecc7_m0x0;
        case 0xecf0: goto source_ecf0_m0x0;
        case 0xecf4: goto source_ecf4_m0x0;
        case 0xed0c: goto source_ed0c_m0x0;
        case 0xed10: goto source_ed10_m0x0;
        case 0xed2b: goto source_ed2b_m0x0;
        case 0xed3e: goto source_ed3e_m0x0;
        case 0xed56: goto source_ed56_m0x0;
        case 0xed7b: goto source_ed7b_m0x0;
        case 0xed7f: goto source_ed7f_m0x0;
        case 0xed97: goto source_ed97_m0x0;
        case 0xedae: goto source_edae_m0x0;
        case 0xedc6: goto source_edc6_m0x0;
        case 0xeddd: goto source_eddd_m0x0;
        case 0xedf8: goto source_edf8_m0x0;
        default: return unsupported(s);
        }
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
        accumulator_change(s, PRODUCER_ASL);
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
        if (L.read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xec89: goto source_ec89_m0x0;
        case 0xec98: goto source_ec98_m0x0;
        case 0xecb0: goto source_ecb0_m0x0;
        case 0xecc3: goto source_ecc3_m0x0;
        case 0xecc7: goto source_ecc7_m0x0;
        case 0xecf0: goto source_ecf0_m0x0;
        case 0xecf4: goto source_ecf4_m0x0;
        case 0xed0c: goto source_ed0c_m0x0;
        case 0xed10: goto source_ed10_m0x0;
        case 0xed2b: goto source_ed2b_m0x0;
        case 0xed3e: goto source_ed3e_m0x0;
        case 0xed56: goto source_ed56_m0x0;
        case 0xed7b: goto source_ed7b_m0x0;
        case 0xed7f: goto source_ed7f_m0x0;
        case 0xed97: goto source_ed97_m0x0;
        case 0xedae: goto source_edae_m0x0;
        case 0xedc6: goto source_edc6_m0x0;
        case 0xeddd: goto source_eddd_m0x0;
        case 0xedf8: goto source_edf8_m0x0;
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
        if (L.read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xec89: goto source_ec89_m0x0;
        case 0xec98: goto source_ec98_m0x0;
        case 0xecb0: goto source_ecb0_m0x0;
        case 0xecc3: goto source_ecc3_m0x0;
        case 0xecc7: goto source_ecc7_m0x0;
        case 0xecf0: goto source_ecf0_m0x0;
        case 0xecf4: goto source_ecf4_m0x0;
        case 0xed0c: goto source_ed0c_m0x0;
        case 0xed10: goto source_ed10_m0x0;
        case 0xed2b: goto source_ed2b_m0x0;
        case 0xed3e: goto source_ed3e_m0x0;
        case 0xed56: goto source_ed56_m0x0;
        case 0xed7b: goto source_ed7b_m0x0;
        case 0xed7f: goto source_ed7f_m0x0;
        case 0xed97: goto source_ed97_m0x0;
        case 0xedae: goto source_edae_m0x0;
        case 0xedc6: goto source_edc6_m0x0;
        case 0xeddd: goto source_eddd_m0x0;
        case 0xedf8: goto source_edf8_m0x0;
        default: return unsupported(s);
        }
source_8ba1_m0x0: /* $80:8BA1 [M=0 X=0] PHB */
        PUSH(0x8ba1, 1, B.registers.data_bank, 1);
        goto source_8ba2_m0x0;
source_8ba2_m0x0: /* $80:8BA2 [M=0 X=0] PHK */
        PUSH(0x8ba2, 1, 0x80, 1);
        goto source_8ba3_m0x0;
source_8ba3_m0x0: /* $80:8BA3 [M=0 X=0] PLB */
        PULL(0x8ba3, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_8ba4_m0x0;
source_8ba4_m0x0: /* $80:8BA4 [M=0 X=0] LDA   $0561 */
        READ(0x8ba4, 3, DB(0x0561u), 2, false);
        set_a(s, B.read_value);
        goto source_8ba7_m0x0;
source_8ba7_m0x0: /* $80:8BA7 [M=0 X=0] BMI   $8BAF */
        BRANCH(0x8ba7, (P & 0x80u) != 0);
        if (B.branch) { goto source_8baf_m0x0; }
        goto source_8ba9_m0x0;
source_8ba9_m0x0: /* $80:8BA9 [M=0 X=0] JSL   $808BD0 */
        return unsupported(s);
source_8baf_m0x0: /* $80:8BAF [M=0 X=0] STX   $4315 */
        WRITE(0x8baf, 3, DB(0x4315u), 2, false, X);
        goto source_8bb2_m0x0;
source_8bb2_m0x0: /* $80:8BB2 [M=0 X=0] LDA   $0C */
        READ(0x8bb2, 2, 0x0cu, 2, false);
        set_a(s, B.read_value);
        goto source_8bb4_m0x0;
source_8bb4_m0x0: /* $80:8BB4 [M=0 X=0] STA   $4312 */
        WRITE(0x8bb4, 3, DB(0x4312u), 2, false, A);
        goto source_8bb7_m0x0;
source_8bb7_m0x0: /* $80:8BB7 [M=0 X=0] LDA   #$1801 */
        IMM(0x8bb7, 3);
        set_a(s, 0x1801u);
        goto source_8bba_m0x0;
source_8bba_m0x0: /* $80:8BBA [M=0 X=0] STA   $4310 */
        WRITE(0x8bba, 3, DB(0x4310u), 2, false, A);
        goto source_8bbd_m0x0;
source_8bbd_m0x0: /* $80:8BBD [M=0 X=0] STY   $2116 */
        WRITE(0x8bbd, 3, DB(0x2116u), 2, false, Y);
        goto source_8bc0_m0x0;
source_8bc0_m0x0: /* $80:8BC0 [M=0 X=0] SEP   #$20 */
        MODE(0x8bc0);
        status_set(s, P | 0x20u);
        goto source_8bc2_m1x0;
source_8bc2_m1x0: /* $80:8BC2 [M=1 X=0] LDA   $0E */
        READ(0x8bc2, 2, 0x0eu, 1, false);
        set_a(s, B.read_value);
        goto source_8bc4_m1x0;
source_8bc4_m1x0: /* $80:8BC4 [M=1 X=0] STA   $4314 */
        WRITE(0x8bc4, 3, DB(0x4314u), 1, false, A);
        goto source_8bc7_m1x0;
source_8bc7_m1x0: /* $80:8BC7 [M=1 X=0] LDA   #$02 */
        IMM(0x8bc7, 2);
        set_a(s, 0x0002u);
        goto source_8bc9_m1x0;
source_8bc9_m1x0: /* $80:8BC9 [M=1 X=0] STA   $420B */
        WRITE(0x8bc9, 3, DB(0x420bu), 1, false, A);
        goto source_8bcc_m1x0;
source_8bcc_m1x0: /* $80:8BCC [M=1 X=0] REP   #$20 */
        MODE(0x8bcc);
        status_set(s, P & 0xdfu);
        goto source_8bce_m0x0;
source_8bce_m0x0: /* $80:8BCE [M=0 X=0] PLB */
        PULL(0x8bce, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_8bcf_m0x0;
source_8bcf_m0x0: /* $80:8BCF [M=0 X=0] RTL */
        PULL(0x8bcf, 3, false);
        if (L.read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xec89: goto source_ec89_m0x0;
        case 0xec98: goto source_ec98_m0x0;
        case 0xecb0: goto source_ecb0_m0x0;
        case 0xecc3: goto source_ecc3_m0x0;
        case 0xecc7: goto source_ecc7_m0x0;
        case 0xecf0: goto source_ecf0_m0x0;
        case 0xecf4: goto source_ecf4_m0x0;
        case 0xed0c: goto source_ed0c_m0x0;
        case 0xed10: goto source_ed10_m0x0;
        case 0xed2b: goto source_ed2b_m0x0;
        case 0xed3e: goto source_ed3e_m0x0;
        case 0xed56: goto source_ed56_m0x0;
        case 0xed7b: goto source_ed7b_m0x0;
        case 0xed7f: goto source_ed7f_m0x0;
        case 0xed97: goto source_ed97_m0x0;
        case 0xedae: goto source_edae_m0x0;
        case 0xedc6: goto source_edc6_m0x0;
        case 0xeddd: goto source_eddd_m0x0;
        case 0xedf8: goto source_edf8_m0x0;
        default: return unsupported(s);
        }
source_8cd0_m0x0: /* $80:8CD0 [M=0 X=0] PHP */
        PUSH(0x8cd0, 1, P, 1);
        goto source_8cd1_m0x0;
source_8cd1_m0x0: /* $80:8CD1 [M=0 X=0] SEP   #$20 */
        MODE(0x8cd1);
        status_set(s, P | 0x20u);
        goto source_8cd3_m1x0;
source_8cd3_m1x0: /* $80:8CD3 [M=1 X=0] PHA */
        PUSH(0x8cd3, 1, A, 1);
        goto source_8cd4_m1x0;
source_8cd4_m1x0: /* $80:8CD4 [M=1 X=0] PHB */
        PUSH(0x8cd4, 1, B.registers.data_bank, 1);
        goto source_8cd5_m1x0;
source_8cd5_m1x0: /* $80:8CD5 [M=1 X=0] PLA */
        PULL(0x8cd5, 1, false);
        set_a(s, B.read_value);
        goto source_8cd6_m1x0;
source_8cd6_m1x0: /* $80:8CD6 [M=1 X=0] CMP   #$7E */
        IMM(0x8cd6, 2);
        compare(s, A, 0x007eu, 1);
        goto source_8cd8_m1x0;
source_8cd8_m1x0: /* $80:8CD8 [M=1 X=0] BEQ   $8CDE */
        BRANCH(0x8cd8, (P & 2u) != 0);
        if (B.branch) { goto source_8cde_m1x0; }
        goto source_8cda_m1x0;
source_8cda_m1x0: /* $80:8CDA [M=1 X=0] CMP   #$7F */
        IMM(0x8cda, 2);
        compare(s, A, 0x007fu, 1);
        goto source_8cdc_m1x0;
source_8cdc_m1x0: /* $80:8CDC [M=1 X=0] BNE   $8CE0 */
        BRANCH(0x8cdc, (P & 2u) == 0);
        if (B.branch) { goto source_8ce0_m1x0; }
        goto source_8cde_m1x0;
source_8cde_m1x0: /* $80:8CDE [M=1 X=0] PHK */
        PUSH(0x8cde, 1, 0x80, 1);
        goto source_8cdf_m1x0;
source_8cdf_m1x0: /* $80:8CDF [M=1 X=0] PLB */
        PULL(0x8cdf, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_8ce0_m1x0;
source_8ce0_m1x0: /* $80:8CE0 [M=1 X=0] PLA */
        PULL(0x8ce0, 1, false);
        set_a(s, B.read_value);
        goto source_8ce1_m1x0;
source_8ce1_m1x0: /* $80:8CE1 [M=1 X=0] PLP */
        PULL(0x8ce1, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_8ce2_m0x0;
source_8ce2_m0x0: /* $80:8CE2 [M=0 X=0] LDA   $0561 */
        READ(0x8ce2, 3, DB(0x0561u), 2, false);
        set_a(s, B.read_value);
        goto source_8ce5_m0x0;
source_8ce5_m0x0: /* $80:8CE5 [M=0 X=0] BMI   $8CEA */
        BRANCH(0x8ce5, (P & 0x80u) != 0);
        if (B.branch) { goto source_8cea_m0x0; }
        goto source_8ce7_m0x0;
source_8ce7_m0x0: /* $80:8CE7 [M=0 X=0] JMP   $8D99 */
        JUMP(0x8ce7);
        return unsupported(s);
source_8cea_m0x0: /* $80:8CEA [M=0 X=0] STX   $20 */
        WRITE(0x8cea, 2, 0x20u, 2, false, X);
        goto source_8cec_m0x0;
source_8cec_m0x0: /* $80:8CEC [M=0 X=0] STY   $22 */
        WRITE(0x8cec, 2, 0x22u, 2, false, Y);
        goto source_8cee_m0x0;
source_8cee_m0x0: /* $80:8CEE [M=0 X=0] LDY   #$00 */
        IMM(0x8cee, 3);
        set_y(s, 0x0000u);
        goto source_8cf1_m0x0;
source_8cf1_m0x0: /* $80:8CF1 [M=0 X=0] LDA   [$0C],Y */
        LONG_INDIRECT(0x8cf1, 0x0c, 2, false, 0);
        set_a(s, B.read_value);
        goto source_8cf3_m0x0;
source_8cf3_m0x0: /* $80:8CF3 [M=0 X=0] STA   $1C */
        WRITE(0x8cf3, 2, 0x1cu, 2, false, A);
        goto source_8cf5_m0x0;
source_8cf5_m0x0: /* $80:8CF5 [M=0 X=0] INY */
        IMP(0x8cf5);
        set_y(s, Y + 1u);
        goto source_8cf6_m0x0;
source_8cf6_m0x0: /* $80:8CF6 [M=0 X=0] INY */
        IMP(0x8cf6);
        set_y(s, Y + 1u);
        goto source_8cf7_m0x0;
source_8cf7_m0x0: /* $80:8CF7 [M=0 X=0] LDA   [$0C],Y */
        LONG_INDIRECT(0x8cf7, 0x0c, 2, false, 0);
        set_a(s, B.read_value);
        goto source_8cf9_m0x0;
source_8cf9_m0x0: /* $80:8CF9 [M=0 X=0] STA   $1E */
        WRITE(0x8cf9, 2, 0x1eu, 2, false, A);
        goto source_8cfb_m0x0;
source_8cfb_m0x0: /* $80:8CFB [M=0 X=0] LDA   $0C */
        READ(0x8cfb, 2, 0x0cu, 2, false);
        set_a(s, B.read_value);
        goto source_8cfd_m0x0;
source_8cfd_m0x0: /* $80:8CFD [M=0 X=0] CLC */
        IMP(0x8cfd);
        P = (uint8_t)(P & 0xfeu);
        goto source_8cfe_m0x0;
source_8cfe_m0x0: /* $80:8CFE [M=0 X=0] ADC   #$06 */
        IMM(0x8cfe, 3);
        add_sub(s, 0x0006u, false);
        goto source_8d01_m0x0;
source_8d01_m0x0: /* $80:8D01 [M=0 X=0] TAX */
        IMP(0x8d01);
        set_x(s, A);
        goto source_8d02_m0x0;
source_8d02_m0x0: /* $80:8D02 [M=0 X=0] LDA   $22 */
        READ(0x8d02, 2, 0x22u, 2, false);
        set_a(s, B.read_value);
        goto source_8d04_m0x0;
source_8d04_m0x0: /* $80:8D04 [M=0 X=0] ASL   A */
        IMP(0x8d04);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d05_m0x0;
source_8d05_m0x0: /* $80:8D05 [M=0 X=0] ASL   A */
        IMP(0x8d05);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d06_m0x0;
source_8d06_m0x0: /* $80:8D06 [M=0 X=0] ASL   A */
        IMP(0x8d06);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d07_m0x0;
source_8d07_m0x0: /* $80:8D07 [M=0 X=0] ASL   A */
        IMP(0x8d07);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d08_m0x0;
source_8d08_m0x0: /* $80:8D08 [M=0 X=0] ASL   A */
        IMP(0x8d08);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d09_m0x0;
source_8d09_m0x0: /* $80:8D09 [M=0 X=0] ADC   $05DB */
        READ(0x8d09, 3, DB(0x05dbu), 2, false);
        add_sub(s, B.read_value, false);
        goto source_8d0c_m0x0;
source_8d0c_m0x0: /* $80:8D0C [M=0 X=0] ORA   $20 */
        READ(0x8d0c, 2, 0x20u, 2, false);
        set_a(s, A | B.read_value);
        goto source_8d0e_m0x0;
source_8d0e_m0x0: /* $80:8D0E [M=0 X=0] TAY */
        IMP(0x8d0e);
        set_y(s, A);
        goto source_8d0f_m0x0;
source_8d0f_m0x0: /* $80:8D0F [M=0 X=0] SEP   #$20 */
        MODE(0x8d0f);
        status_set(s, P | 0x20u);
        goto source_8d11_m1x0;
source_8d11_m1x0: /* $80:8D11 [M=1 X=0] LDA   $0E */
        READ(0x8d11, 2, 0x0eu, 1, false);
        set_a(s, B.read_value);
        goto source_8d13_m1x0;
source_8d13_m1x0: /* $80:8D13 [M=1 X=0] STA   $4314 */
        WRITE(0x8d13, 3, DB(0x4314u), 1, false, A);
        goto source_8d16_m1x0;
source_8d16_m1x0: /* $80:8D16 [M=1 X=0] LDA   #$01 */
        IMM(0x8d16, 2);
        set_a(s, 0x0001u);
        goto source_8d18_m1x0;
source_8d18_m1x0: /* $80:8D18 [M=1 X=0] STA   $4310 */
        WRITE(0x8d18, 3, DB(0x4310u), 1, false, A);
        goto source_8d1b_m1x0;
source_8d1b_m1x0: /* $80:8D1B [M=1 X=0] LDA   #$18 */
        IMM(0x8d1b, 2);
        set_a(s, 0x0018u);
        goto source_8d1d_m1x0;
source_8d1d_m1x0: /* $80:8D1D [M=1 X=0] STA   $4311 */
        WRITE(0x8d1d, 3, DB(0x4311u), 1, false, A);
        goto source_8d20_m1x0;
source_8d20_m1x0: /* $80:8D20 [M=1 X=0] REP   #$20 */
        MODE(0x8d20);
        status_set(s, P & 0xdfu);
        goto source_8d22_m0x0;
source_8d22_m0x0: /* $80:8D22 [M=0 X=0] LDA   $1C */
        READ(0x8d22, 2, 0x1cu, 2, false);
        set_a(s, B.read_value);
        goto source_8d24_m0x0;
source_8d24_m0x0: /* $80:8D24 [M=0 X=0] CLC */
        IMP(0x8d24);
        P = (uint8_t)(P & 0xfeu);
        goto source_8d25_m0x0;
source_8d25_m0x0: /* $80:8D25 [M=0 X=0] ADC   $20 */
        READ(0x8d25, 2, 0x20u, 2, false);
        add_sub(s, B.read_value, false);
        goto source_8d27_m0x0;
source_8d27_m0x0: /* $80:8D27 [M=0 X=0] CMP   #$21 */
        IMM(0x8d27, 3);
        compare(s, A, 0x0021u, 2);
        goto source_8d2a_m0x0;
source_8d2a_m0x0: /* $80:8D2A [M=0 X=0] BCS   $8D62 */
        BRANCH(0x8d2a, (P & 1u) != 0);
        if (B.branch) { goto source_8d62_m0x0; }
        goto source_8d2c_m0x0;
source_8d2c_m0x0: /* $80:8D2C [M=0 X=0] LDA   $1C */
        READ(0x8d2c, 2, 0x1cu, 2, false);
        set_a(s, B.read_value);
        goto source_8d2e_m0x0;
source_8d2e_m0x0: /* $80:8D2E [M=0 X=0] ASL   A */
        IMP(0x8d2e);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d2f_m0x0;
source_8d2f_m0x0: /* $80:8D2F [M=0 X=0] STA   $4315 */
        WRITE(0x8d2f, 3, DB(0x4315u), 2, false, A);
        goto source_8d32_m0x0;
source_8d32_m0x0: /* $80:8D32 [M=0 X=0] STX   $4312 */
        WRITE(0x8d32, 3, DB(0x4312u), 2, false, X);
        goto source_8d35_m0x0;
source_8d35_m0x0: /* $80:8D35 [M=0 X=0] STY   $2116 */
        WRITE(0x8d35, 3, DB(0x2116u), 2, false, Y);
        goto source_8d38_m0x0;
source_8d38_m0x0: /* $80:8D38 [M=0 X=0] SEP   #$20 */
        MODE(0x8d38);
        status_set(s, P | 0x20u);
        goto source_8d3a_m1x0;
source_8d3a_m1x0: /* $80:8D3A [M=1 X=0] LDA   #$02 */
        IMM(0x8d3a, 2);
        set_a(s, 0x0002u);
        goto source_8d3c_m1x0;
source_8d3c_m1x0: /* $80:8D3C [M=1 X=0] STA   $420B */
        WRITE(0x8d3c, 3, DB(0x420bu), 1, false, A);
        goto source_8d3f_m1x0;
source_8d3f_m1x0: /* $80:8D3F [M=1 X=0] REP   #$20 */
        MODE(0x8d3f);
        status_set(s, P & 0xdfu);
        goto source_8d41_m0x0;
source_8d41_m0x0: /* $80:8D41 [M=0 X=0] TXA */
        IMP(0x8d41);
        set_a(s, X);
        goto source_8d42_m0x0;
source_8d42_m0x0: /* $80:8D42 [M=0 X=0] CLC */
        IMP(0x8d42);
        P = (uint8_t)(P & 0xfeu);
        goto source_8d43_m0x0;
source_8d43_m0x0: /* $80:8D43 [M=0 X=0] ADC   $1C */
        READ(0x8d43, 2, 0x1cu, 2, false);
        add_sub(s, B.read_value, false);
        goto source_8d45_m0x0;
source_8d45_m0x0: /* $80:8D45 [M=0 X=0] ADC   $1C */
        READ(0x8d45, 2, 0x1cu, 2, false);
        add_sub(s, B.read_value, false);
        goto source_8d47_m0x0;
source_8d47_m0x0: /* $80:8D47 [M=0 X=0] TAX */
        IMP(0x8d47);
        set_x(s, A);
        goto source_8d48_m0x0;
source_8d48_m0x0: /* $80:8D48 [M=0 X=0] TYA */
        IMP(0x8d48);
        set_a(s, Y);
        goto source_8d49_m0x0;
source_8d49_m0x0: /* $80:8D49 [M=0 X=0] CLC */
        IMP(0x8d49);
        P = (uint8_t)(P & 0xfeu);
        goto source_8d4a_m0x0;
source_8d4a_m0x0: /* $80:8D4A [M=0 X=0] ADC   #$20 */
        IMM(0x8d4a, 3);
        add_sub(s, 0x0020u, false);
        goto source_8d4d_m0x0;
source_8d4d_m0x0: /* $80:8D4D [M=0 X=0] TAY */
        IMP(0x8d4d);
        set_y(s, A);
        goto source_8d4e_m0x0;
source_8d4e_m0x0: /* $80:8D4E [M=0 X=0] INC   $22 */
        CHANGE(0x8d4e, 2, 0x22u, 2, false, PRODUCER_INC);
        goto source_8d50_m0x0;
source_8d50_m0x0: /* $80:8D50 [M=0 X=0] LDA   $22 */
        READ(0x8d50, 2, 0x22u, 2, false);
        set_a(s, B.read_value);
        goto source_8d52_m0x0;
source_8d52_m0x0: /* $80:8D52 [M=0 X=0] CMP   #$20 */
        IMM(0x8d52, 3);
        compare(s, A, 0x0020u, 2);
        goto source_8d55_m0x0;
source_8d55_m0x0: /* $80:8D55 [M=0 X=0] BNE   $8D5D */
        BRANCH(0x8d55, (P & 2u) == 0);
        if (B.branch) { goto source_8d5d_m0x0; }
        goto source_8d57_m0x0;
source_8d57_m0x0: /* $80:8D57 [M=0 X=0] TYA */
        IMP(0x8d57);
        set_a(s, Y);
        goto source_8d58_m0x0;
source_8d58_m0x0: /* $80:8D58 [M=0 X=0] CLC */
        IMP(0x8d58);
        P = (uint8_t)(P & 0xfeu);
        goto source_8d59_m0x0;
source_8d59_m0x0: /* $80:8D59 [M=0 X=0] ADC   $05DD */
        READ(0x8d59, 3, DB(0x05ddu), 2, false);
        add_sub(s, B.read_value, false);
        goto source_8d5c_m0x0;
source_8d5c_m0x0: /* $80:8D5C [M=0 X=0] TAY */
        IMP(0x8d5c);
        set_y(s, A);
        goto source_8d5d_m0x0;
source_8d5d_m0x0: /* $80:8D5D [M=0 X=0] DEC   $1E */
        CHANGE(0x8d5d, 2, 0x1eu, 2, false, PRODUCER_DEC);
        goto source_8d5f_m0x0;
source_8d5f_m0x0: /* $80:8D5F [M=0 X=0] BNE   $8D22 */
        BRANCH(0x8d5f, (P & 2u) == 0);
        if (B.branch) { goto source_8d22_m0x0; }
        goto source_8d61_m0x0;
source_8d61_m0x0: /* $80:8D61 [M=0 X=0] RTL */
        PULL(0x8d61, 3, false);
        if (L.read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xec89: goto source_ec89_m0x0;
        case 0xec98: goto source_ec98_m0x0;
        case 0xecb0: goto source_ecb0_m0x0;
        case 0xecc3: goto source_ecc3_m0x0;
        case 0xecc7: goto source_ecc7_m0x0;
        case 0xecf0: goto source_ecf0_m0x0;
        case 0xecf4: goto source_ecf4_m0x0;
        case 0xed0c: goto source_ed0c_m0x0;
        case 0xed10: goto source_ed10_m0x0;
        case 0xed2b: goto source_ed2b_m0x0;
        case 0xed3e: goto source_ed3e_m0x0;
        case 0xed56: goto source_ed56_m0x0;
        case 0xed7b: goto source_ed7b_m0x0;
        case 0xed7f: goto source_ed7f_m0x0;
        case 0xed97: goto source_ed97_m0x0;
        case 0xedae: goto source_edae_m0x0;
        case 0xedc6: goto source_edc6_m0x0;
        case 0xeddd: goto source_eddd_m0x0;
        case 0xedf8: goto source_edf8_m0x0;
        default: return unsupported(s);
        }
source_8d62_m0x0: /* $80:8D62 [M=0 X=0] LDA   #$20 */
        IMM(0x8d62, 3);
        set_a(s, 0x0020u);
        goto source_8d65_m0x0;
source_8d65_m0x0: /* $80:8D65 [M=0 X=0] SBC   $20 */
        READ(0x8d65, 2, 0x20u, 2, false);
        add_sub(s, B.read_value, true);
        goto source_8d67_m0x0;
source_8d67_m0x0: /* $80:8D67 [M=0 X=0] ASL   A */
        IMP(0x8d67);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d68_m0x0;
source_8d68_m0x0: /* $80:8D68 [M=0 X=0] STA   $24 */
        WRITE(0x8d68, 2, 0x24u, 2, false, A);
        goto source_8d6a_m0x0;
source_8d6a_m0x0: /* $80:8D6A [M=0 X=0] STA   $4315 */
        WRITE(0x8d6a, 3, DB(0x4315u), 2, false, A);
        goto source_8d6d_m0x0;
source_8d6d_m0x0: /* $80:8D6D [M=0 X=0] STX   $4312 */
        WRITE(0x8d6d, 3, DB(0x4312u), 2, false, X);
        goto source_8d70_m0x0;
source_8d70_m0x0: /* $80:8D70 [M=0 X=0] STY   $2116 */
        WRITE(0x8d70, 3, DB(0x2116u), 2, false, Y);
        goto source_8d73_m0x0;
source_8d73_m0x0: /* $80:8D73 [M=0 X=0] SEP   #$20 */
        MODE(0x8d73);
        status_set(s, P | 0x20u);
        goto source_8d75_m1x0;
source_8d75_m1x0: /* $80:8D75 [M=1 X=0] LDA   #$02 */
        IMM(0x8d75, 2);
        set_a(s, 0x0002u);
        goto source_8d77_m1x0;
source_8d77_m1x0: /* $80:8D77 [M=1 X=0] STA   $420B */
        WRITE(0x8d77, 3, DB(0x420bu), 1, false, A);
        goto source_8d7a_m1x0;
source_8d7a_m1x0: /* $80:8D7A [M=1 X=0] REP   #$20 */
        MODE(0x8d7a);
        status_set(s, P & 0xdfu);
        goto source_8d7c_m0x0;
source_8d7c_m0x0: /* $80:8D7C [M=0 X=0] LDA   $1C */
        READ(0x8d7c, 2, 0x1cu, 2, false);
        set_a(s, B.read_value);
        goto source_8d7e_m0x0;
source_8d7e_m0x0: /* $80:8D7E [M=0 X=0] ASL   A */
        IMP(0x8d7e);
        accumulator_change(s, PRODUCER_ASL);
        goto source_8d7f_m0x0;
source_8d7f_m0x0: /* $80:8D7F [M=0 X=0] SEC */
        IMP(0x8d7f);
        P = (uint8_t)(P | 1u);
        goto source_8d80_m0x0;
source_8d80_m0x0: /* $80:8D80 [M=0 X=0] SBC   $24 */
        READ(0x8d80, 2, 0x24u, 2, false);
        add_sub(s, B.read_value, true);
        goto source_8d82_m0x0;
source_8d82_m0x0: /* $80:8D82 [M=0 X=0] STA   $4315 */
        WRITE(0x8d82, 3, DB(0x4315u), 2, false, A);
        goto source_8d85_m0x0;
source_8d85_m0x0: /* $80:8D85 [M=0 X=0] TXA */
        IMP(0x8d85);
        set_a(s, X);
        goto source_8d86_m0x0;
source_8d86_m0x0: /* $80:8D86 [M=0 X=0] CLC */
        IMP(0x8d86);
        P = (uint8_t)(P & 0xfeu);
        goto source_8d87_m0x0;
source_8d87_m0x0: /* $80:8D87 [M=0 X=0] ADC   $24 */
        READ(0x8d87, 2, 0x24u, 2, false);
        add_sub(s, B.read_value, false);
        goto source_8d89_m0x0;
source_8d89_m0x0: /* $80:8D89 [M=0 X=0] STA   $4312 */
        WRITE(0x8d89, 3, DB(0x4312u), 2, false, A);
        goto source_8d8c_m0x0;
source_8d8c_m0x0: /* $80:8D8C [M=0 X=0] TYA */
        IMP(0x8d8c);
        set_a(s, Y);
        goto source_8d8d_m0x0;
source_8d8d_m0x0: /* $80:8D8D [M=0 X=0] ADC   #$0400 */
        IMM(0x8d8d, 3);
        add_sub(s, 0x0400u, false);
        goto source_8d90_m0x0;
source_8d90_m0x0: /* $80:8D90 [M=0 X=0] AND   #$FFE0 */
        IMM(0x8d90, 3);
        set_a(s, A & 0xffe0u);
        goto source_8d93_m0x0;
source_8d93_m0x0: /* $80:8D93 [M=0 X=0] STA   $2116 */
        WRITE(0x8d93, 3, DB(0x2116u), 2, false, A);
        goto source_8d96_m0x0;
source_8d96_m0x0: /* $80:8D96 [M=0 X=0] JMP   $8D38 */
        JUMP(0x8d96);
        goto source_8d38_m0x0;
source_c62b_m0x0: /* $80:C62B [M=0 X=0] PHP */
        B.resume = __LINE__; return begin_codec(s); case __LINE__:;
        goto source_c682_m0x0;
source_c682_m0x0: /* $80:C682 [M=0 X=0] RTL */
        PULL(0xc682, 3, false);
        if (L.read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xec89: goto source_ec89_m0x0;
        case 0xec98: goto source_ec98_m0x0;
        case 0xecb0: goto source_ecb0_m0x0;
        case 0xecc3: goto source_ecc3_m0x0;
        case 0xecc7: goto source_ecc7_m0x0;
        case 0xecf0: goto source_ecf0_m0x0;
        case 0xecf4: goto source_ecf4_m0x0;
        case 0xed0c: goto source_ed0c_m0x0;
        case 0xed10: goto source_ed10_m0x0;
        case 0xed2b: goto source_ed2b_m0x0;
        case 0xed3e: goto source_ed3e_m0x0;
        case 0xed56: goto source_ed56_m0x0;
        case 0xed7b: goto source_ed7b_m0x0;
        case 0xed7f: goto source_ed7f_m0x0;
        case 0xed97: goto source_ed97_m0x0;
        case 0xedae: goto source_edae_m0x0;
        case 0xedc6: goto source_edc6_m0x0;
        case 0xeddd: goto source_eddd_m0x0;
        case 0xedf8: goto source_edf8_m0x0;
        default: return unsupported(s);
        }
source_ea4b_m0x0: /* $80:EA4B [M=0 X=0] PHX */
        PUSH(0xea4b, 1, X, 2);
        goto source_ea4c_m0x0;
source_ea4c_m0x0: /* $80:EA4C [M=0 X=0] PHY */
        PUSH(0xea4c, 1, Y, 2);
        goto source_ea4d_m0x0;
source_ea4d_m0x0: /* $80:EA4D [M=0 X=0] STX   $00 */
        WRITE(0xea4d, 2, 0x00u, 2, false, X);
        goto source_ea4f_m0x0;
source_ea4f_m0x0: /* $80:EA4F [M=0 X=0] ASL   A */
        IMP(0xea4f);
        accumulator_change(s, PRODUCER_ASL);
        goto source_ea50_m0x0;
source_ea50_m0x0: /* $80:EA50 [M=0 X=0] ASL   A */
        IMP(0xea50);
        accumulator_change(s, PRODUCER_ASL);
        goto source_ea51_m0x0;
source_ea51_m0x0: /* $80:EA51 [M=0 X=0] XBA */
        SUSPEND(implied(s, 0xea51, 1, 2));
        A = (uint16_t)((A << 8) | (A >> 8));
        nz(s, A, 1);
        goto source_ea52_m0x0;
source_ea52_m0x0: /* $80:EA52 [M=0 X=0] STA   $7E7E60 */
        WRITE(0xea52, 4, 0x7e7e60u, 2, false, A);
        goto source_ea56_m0x0;
source_ea56_m0x0: /* $80:EA56 [M=0 X=0] EOR   #$FFFF */
        IMM(0xea56, 3);
        set_a(s, A ^ 0xffffu);
        goto source_ea59_m0x0;
source_ea59_m0x0: /* $80:EA59 [M=0 X=0] STA   $7E7E64 */
        WRITE(0xea59, 4, 0x7e7e64u, 2, false, A);
        goto source_ea5d_m0x0;
source_ea5d_m0x0: /* $80:EA5D [M=0 X=0] LDY   #$06 */
        IMM(0xea5d, 3);
        set_y(s, 0x0006u);
        goto source_ea60_m0x0;
source_ea60_m0x0: /* $80:EA60 [M=0 X=0] LDA   [$0C],Y */
        LONG_INDIRECT(0xea60, 0x0c, 2, false, 0);
        set_a(s, B.read_value);
        goto source_ea62_m0x0;
source_ea62_m0x0: /* $80:EA62 [M=0 X=0] AND   $7E7E64 */
        READ(0xea62, 4, 0x7e7e64u, 2, false);
        set_a(s, A & B.read_value);
        goto source_ea66_m0x0;
source_ea66_m0x0: /* $80:EA66 [M=0 X=0] ORA   $7E7E60 */
        READ(0xea66, 4, 0x7e7e60u, 2, false);
        set_a(s, A | B.read_value);
        goto source_ea6a_m0x0;
source_ea6a_m0x0: /* $80:EA6A [M=0 X=0] CLC */
        IMP(0xea6a);
        P = (uint8_t)(P & 0xfeu);
        goto source_ea6b_m0x0;
source_ea6b_m0x0: /* $80:EA6B [M=0 X=0] ADC   $166B */
        READ(0xea6b, 3, DB(0x166bu), 2, false);
        add_sub(s, B.read_value, false);
        goto source_ea6e_m0x0;
source_ea6e_m0x0: /* $80:EA6E [M=0 X=0] STA   [$0C],Y */
        LONG_INDIRECT(0xea6e, 0x0c, 2, true, A);
        goto source_ea70_m0x0;
source_ea70_m0x0: /* $80:EA70 [M=0 X=0] INY */
        IMP(0xea70);
        set_y(s, Y + 1u);
        goto source_ea71_m0x0;
source_ea71_m0x0: /* $80:EA71 [M=0 X=0] INY */
        IMP(0xea71);
        set_y(s, Y + 1u);
        goto source_ea72_m0x0;
source_ea72_m0x0: /* $80:EA72 [M=0 X=0] CPY   $00 */
        READ(0xea72, 2, 0x00u, 2, false);
        compare(s, Y, B.read_value, 2);
        goto source_ea74_m0x0;
source_ea74_m0x0: /* $80:EA74 [M=0 X=0] BCC   $EA60 */
        BRANCH(0xea74, (P & 1u) == 0);
        if (B.branch) { goto source_ea60_m0x0; }
        goto source_ea76_m0x0;
source_ea76_m0x0: /* $80:EA76 [M=0 X=0] PLY */
        PULL(0xea76, 2, false);
        set_y(s, B.read_value);
        goto source_ea77_m0x0;
source_ea77_m0x0: /* $80:EA77 [M=0 X=0] PLX */
        PULL(0xea77, 2, false);
        set_x(s, B.read_value);
        goto source_ea78_m0x0;
source_ea78_m0x0: /* $80:EA78 [M=0 X=0] RTL */
        PULL(0xea78, 3, false);
        if (L.read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xec89: goto source_ec89_m0x0;
        case 0xec98: goto source_ec98_m0x0;
        case 0xecb0: goto source_ecb0_m0x0;
        case 0xecc3: goto source_ecc3_m0x0;
        case 0xecc7: goto source_ecc7_m0x0;
        case 0xecf0: goto source_ecf0_m0x0;
        case 0xecf4: goto source_ecf4_m0x0;
        case 0xed0c: goto source_ed0c_m0x0;
        case 0xed10: goto source_ed10_m0x0;
        case 0xed2b: goto source_ed2b_m0x0;
        case 0xed3e: goto source_ed3e_m0x0;
        case 0xed56: goto source_ed56_m0x0;
        case 0xed7b: goto source_ed7b_m0x0;
        case 0xed7f: goto source_ed7f_m0x0;
        case 0xed97: goto source_ed97_m0x0;
        case 0xedae: goto source_edae_m0x0;
        case 0xedc6: goto source_edc6_m0x0;
        case 0xeddd: goto source_eddd_m0x0;
        case 0xedf8: goto source_edf8_m0x0;
        default: return unsupported(s);
        }
source_ec68_m0x0: /* $80:EC68 [M=0 X=0] SEP   #$20 */
        MODE(0xec68);
        status_set(s, P | 0x20u);
        goto source_ec6a_m1x0;
source_ec6a_m1x0: /* $80:EC6A [M=1 X=0] LDA   #$00 */
        IMM(0xec6a, 2);
        set_a(s, 0x0000u);
        goto source_ec6c_m1x0;
source_ec6c_m1x0: /* $80:EC6C [M=1 X=0] STA   $002121 */
        WRITE(0xec6c, 4, 0x002121u, 1, false, A);
        goto source_ec70_m1x0;
source_ec70_m1x0: /* $80:EC70 [M=1 X=0] STA   $002122 */
        WRITE(0xec70, 4, 0x002122u, 1, false, A);
        goto source_ec74_m1x0;
source_ec74_m1x0: /* $80:EC74 [M=1 X=0] STA   $002122 */
        WRITE(0xec74, 4, 0x002122u, 1, false, A);
        goto source_ec78_m1x0;
source_ec78_m1x0: /* $80:EC78 [M=1 X=0] REP   #$30 */
        MODE(0xec78);
        status_set(s, P & 0xcfu);
        goto source_ec7a_m0x0;
source_ec7a_m0x0: /* $80:EC7A [M=0 X=0] LDX   #$2000 */
        IMM(0xec7a, 3);
        set_x(s, 0x2000u);
        goto source_ec7d_m0x0;
source_ec7d_m0x0: /* $80:EC7D [M=0 X=0] LDA   $7E7E6A */
        READ(0xec7d, 4, 0x7e7e6au, 2, false);
        set_a(s, B.read_value);
        goto source_ec81_m0x0;
source_ec81_m0x0: /* $80:EC81 [M=0 X=0] TAY */
        IMP(0xec81);
        set_y(s, A);
        goto source_ec82_m0x0;
source_ec82_m0x0: /* $80:EC82 [M=0 X=0] LDA   #$00 */
        IMM(0xec82, 3);
        set_a(s, 0x0000u);
        goto source_ec85_m0x0;
source_ec85_m0x0: /* $80:EC85 [M=0 X=0] JSL   $808AD2 */
        SUSPEND(jsl_call(s, 0xec85));
        B.registers.stack_pointer -= 3u;
        goto source_8ad2_m0x0;
source_ec89_m0x0: /* $80:EC89 [M=0 X=0] LDX   #$10 */
        IMM(0xec89, 3);
        set_x(s, 0x0010u);
        goto source_ec8c_m0x0;
source_ec8c_m0x0: /* $80:EC8C [M=0 X=0] LDA   $7E7E6E */
        READ(0xec8c, 4, 0x7e7e6eu, 2, false);
        set_a(s, B.read_value);
        goto source_ec90_m0x0;
source_ec90_m0x0: /* $80:EC90 [M=0 X=0] TAY */
        IMP(0xec90);
        set_y(s, A);
        goto source_ec91_m0x0;
source_ec91_m0x0: /* $80:EC91 [M=0 X=0] LDA   #$00 */
        IMM(0xec91, 3);
        set_a(s, 0x0000u);
        goto source_ec94_m0x0;
source_ec94_m0x0: /* $80:EC94 [M=0 X=0] JSL   $808AD2 */
        SUSPEND(jsl_call(s, 0xec94));
        B.registers.stack_pointer -= 3u;
        goto source_8ad2_m0x0;
source_ec98_m0x0: /* $80:EC98 [M=0 X=0] LDA   #$AE */
        IMM(0xec98, 3);
        set_a(s, 0x00aeu);
        goto source_ec9b_m0x0;
source_ec9b_m0x0: /* $80:EC9B [M=0 X=0] STA   $0E */
        WRITE(0xec9b, 2, 0x0eu, 2, false, A);
        goto source_ec9d_m0x0;
source_ec9d_m0x0: /* $80:EC9D [M=0 X=0] LDA   #$A0AF */
        IMM(0xec9d, 3);
        set_a(s, 0xa0afu);
        goto source_eca0_m0x0;
source_eca0_m0x0: /* $80:ECA0 [M=0 X=0] STA   $0C */
        WRITE(0xeca0, 2, 0x0cu, 2, false, A);
        goto source_eca2_m0x0;
source_eca2_m0x0: /* $80:ECA2 [M=0 X=0] LDA   #$7F */
        IMM(0xeca2, 3);
        set_a(s, 0x007fu);
        goto source_eca5_m0x0;
source_eca5_m0x0: /* $80:ECA5 [M=0 X=0] STA   $12 */
        WRITE(0xeca5, 2, 0x12u, 2, false, A);
        goto source_eca7_m0x0;
source_eca7_m0x0: /* $80:ECA7 [M=0 X=0] LDA   #$2000 */
        IMM(0xeca7, 3);
        set_a(s, 0x2000u);
        goto source_ecaa_m0x0;
source_ecaa_m0x0: /* $80:ECAA [M=0 X=0] STA   $10 */
        WRITE(0xecaa, 2, 0x10u, 2, false, A);
        goto source_ecac_m0x0;
source_ecac_m0x0: /* $80:ECAC [M=0 X=0] JSL   $80C62B */
        SUSPEND(jsl_call(s, 0xecac));
        B.registers.stack_pointer -= 3u;
        goto source_c62b_m0x0;
source_ecb0_m0x0: /* $80:ECB0 [M=0 X=0] LDA   #$7F */
        IMM(0xecb0, 3);
        set_a(s, 0x007fu);
        goto source_ecb3_m0x0;
source_ecb3_m0x0: /* $80:ECB3 [M=0 X=0] STA   $0E */
        WRITE(0xecb3, 2, 0x0eu, 2, false, A);
        goto source_ecb5_m0x0;
source_ecb5_m0x0: /* $80:ECB5 [M=0 X=0] LDA   #$2000 */
        IMM(0xecb5, 3);
        set_a(s, 0x2000u);
        goto source_ecb8_m0x0;
source_ecb8_m0x0: /* $80:ECB8 [M=0 X=0] STA   $0C */
        WRITE(0xecb8, 2, 0x0cu, 2, false, A);
        goto source_ecba_m0x0;
source_ecba_m0x0: /* $80:ECBA [M=0 X=0] LDA   $7E7E6A */
        READ(0xecba, 4, 0x7e7e6au, 2, false);
        set_a(s, B.read_value);
        goto source_ecbe_m0x0;
source_ecbe_m0x0: /* $80:ECBE [M=0 X=0] TAY */
        IMP(0xecbe);
        set_y(s, A);
        goto source_ecbf_m0x0;
source_ecbf_m0x0: /* $80:ECBF [M=0 X=0] JSL   $808BA1 */
        SUSPEND(jsl_call(s, 0xecbf));
        B.registers.stack_pointer -= 3u;
        goto source_8ba1_m0x0;
source_ecc3_m0x0: /* $80:ECC3 [M=0 X=0] JSL   $8086DA */
        SUSPEND(jsl_call(s, 0xecc3));
        B.registers.stack_pointer -= 3u;
        goto source_86da_m0x0;
source_ecc7_m0x0: /* $80:ECC7 [M=0 X=0] LDA   $16B1 */
        READ(0xecc7, 3, DB(0x16b1u), 2, false);
        set_a(s, B.read_value);
        goto source_ecca_m0x0;
source_ecca_m0x0: /* $80:ECCA [M=0 X=0] CMP   #$23 */
        IMM(0xecca, 3);
        compare(s, A, 0x0023u, 2);
        goto source_eccd_m0x0;
source_eccd_m0x0: /* $80:ECCD [M=0 X=0] BCC   $ECD2 */
        BRANCH(0xeccd, (P & 1u) == 0);
        if (B.branch) { goto source_ecd2_m0x0; }
        goto source_eccf_m0x0;
source_eccf_m0x0: /* $80:ECCF [M=0 X=0] JMP   $EDF9 */
        JUMP(0xeccf);
        goto source_edf9_m0x0;
source_ecd2_m0x0: /* $80:ECD2 [M=0 X=0] ASL   A */
        IMP(0xecd2);
        accumulator_change(s, PRODUCER_ASL);
        goto source_ecd3_m0x0;
source_ecd3_m0x0: /* $80:ECD3 [M=0 X=0] ASL   A */
        IMP(0xecd3);
        accumulator_change(s, PRODUCER_ASL);
        goto source_ecd4_m0x0;
source_ecd4_m0x0: /* $80:ECD4 [M=0 X=0] TAX */
        IMP(0xecd4);
        set_x(s, A);
        goto source_ecd5_m0x0;
source_ecd5_m0x0: /* $80:ECD5 [M=0 X=0] PHX */
        PUSH(0xecd5, 1, X, 2);
        goto source_ecd6_m0x0;
source_ecd6_m0x0: /* $80:ECD6 [M=0 X=0] LDA   $80DDC6,X */
        READ(0xecd6, 4, (0x80ddc6u + X), 2, false);
        set_a(s, B.read_value);
        goto source_ecda_m0x0;
source_ecda_m0x0: /* $80:ECDA [M=0 X=0] CLC */
        IMP(0xecda);
        P = (uint8_t)(P & 0xfeu);
        goto source_ecdb_m0x0;
source_ecdb_m0x0: /* $80:ECDB [M=0 X=0] ADC   #$60 */
        IMM(0xecdb, 3);
        add_sub(s, 0x0060u, false);
        goto source_ecde_m0x0;
source_ecde_m0x0: /* $80:ECDE [M=0 X=0] STA   $0C */
        WRITE(0xecde, 2, 0x0cu, 2, false, A);
        goto source_ece0_m0x0;
source_ece0_m0x0: /* $80:ECE0 [M=0 X=0] LDA   $80DDC8,X */
        READ(0xece0, 4, (0x80ddc8u + X), 2, false);
        set_a(s, B.read_value);
        goto source_ece4_m0x0;
source_ece4_m0x0: /* $80:ECE4 [M=0 X=0] STA   $0E */
        WRITE(0xece4, 2, 0x0eu, 2, false, A);
        goto source_ece6_m0x0;
source_ece6_m0x0: /* $80:ECE6 [M=0 X=0] LDX   #$50 */
        IMM(0xece6, 3);
        set_x(s, 0x0050u);
        goto source_ece9_m0x0;
source_ece9_m0x0: /* $80:ECE9 [M=0 X=0] LDY   #$30 */
        IMM(0xece9, 3);
        set_y(s, 0x0030u);
        goto source_ecec_m0x0;
source_ecec_m0x0: /* $80:ECEC [M=0 X=0] JSL   $808A02 */
        SUSPEND(jsl_call(s, 0xecec));
        B.registers.stack_pointer -= 3u;
        goto source_8a02_m0x0;
source_ecf0_m0x0: /* $80:ECF0 [M=0 X=0] JSL   $8086DA */
        SUSPEND(jsl_call(s, 0xecf0));
        B.registers.stack_pointer -= 3u;
        goto source_86da_m0x0;
source_ecf4_m0x0: /* $80:ECF4 [M=0 X=0] PLX */
        PULL(0xecf4, 2, false);
        set_x(s, B.read_value);
        goto source_ecf5_m0x0;
source_ecf5_m0x0: /* $80:ECF5 [M=0 X=0] PHX */
        PUSH(0xecf5, 1, X, 2);
        goto source_ecf6_m0x0;
source_ecf6_m0x0: /* $80:ECF6 [M=0 X=0] LDA   $80DF6A,X */
        READ(0xecf6, 4, (0x80df6au + X), 2, false);
        set_a(s, B.read_value);
        goto source_ecfa_m0x0;
source_ecfa_m0x0: /* $80:ECFA [M=0 X=0] STA   $0C */
        WRITE(0xecfa, 2, 0x0cu, 2, false, A);
        goto source_ecfc_m0x0;
source_ecfc_m0x0: /* $80:ECFC [M=0 X=0] LDA   $80DF6C,X */
        READ(0xecfc, 4, (0x80df6cu + X), 2, false);
        set_a(s, B.read_value);
        goto source_ed00_m0x0;
source_ed00_m0x0: /* $80:ED00 [M=0 X=0] STA   $0E */
        WRITE(0xed00, 2, 0x0eu, 2, false, A);
        goto source_ed02_m0x0;
source_ed02_m0x0: /* $80:ED02 [M=0 X=0] LDX   #$07 */
        IMM(0xed02, 3);
        set_x(s, 0x0007u);
        goto source_ed05_m0x0;
source_ed05_m0x0: /* $80:ED05 [M=0 X=0] LDY   #$49 */
        IMM(0xed05, 3);
        set_y(s, 0x0049u);
        goto source_ed08_m0x0;
source_ed08_m0x0: /* $80:ED08 [M=0 X=0] JSL   $808A02 */
        SUSPEND(jsl_call(s, 0xed08));
        B.registers.stack_pointer -= 3u;
        goto source_8a02_m0x0;
source_ed0c_m0x0: /* $80:ED0C [M=0 X=0] JSL   $8086DA */
        SUSPEND(jsl_call(s, 0xed0c));
        B.registers.stack_pointer -= 3u;
        goto source_86da_m0x0;
source_ed10_m0x0: /* $80:ED10 [M=0 X=0] PLX */
        PULL(0xed10, 2, false);
        set_x(s, B.read_value);
        goto source_ed11_m0x0;
source_ed11_m0x0: /* $80:ED11 [M=0 X=0] LDA   $80DE52,X */
        READ(0xed11, 4, (0x80de52u + X), 2, false);
        set_a(s, B.read_value);
        goto source_ed15_m0x0;
source_ed15_m0x0: /* $80:ED15 [M=0 X=0] STA   $0C */
        WRITE(0xed15, 2, 0x0cu, 2, false, A);
        goto source_ed17_m0x0;
source_ed17_m0x0: /* $80:ED17 [M=0 X=0] LDA   $80DE54,X */
        READ(0xed17, 4, (0x80de54u + X), 2, false);
        set_a(s, B.read_value);
        goto source_ed1b_m0x0;
source_ed1b_m0x0: /* $80:ED1B [M=0 X=0] STA   $0E */
        WRITE(0xed1b, 2, 0x0eu, 2, false, A);
        goto source_ed1d_m0x0;
source_ed1d_m0x0: /* $80:ED1D [M=0 X=0] LDA   #$7F */
        IMM(0xed1d, 3);
        set_a(s, 0x007fu);
        goto source_ed20_m0x0;
source_ed20_m0x0: /* $80:ED20 [M=0 X=0] STA   $12 */
        WRITE(0xed20, 2, 0x12u, 2, false, A);
        goto source_ed22_m0x0;
source_ed22_m0x0: /* $80:ED22 [M=0 X=0] LDA   #$2000 */
        IMM(0xed22, 3);
        set_a(s, 0x2000u);
        goto source_ed25_m0x0;
source_ed25_m0x0: /* $80:ED25 [M=0 X=0] STA   $10 */
        WRITE(0xed25, 2, 0x10u, 2, false, A);
        goto source_ed27_m0x0;
source_ed27_m0x0: /* $80:ED27 [M=0 X=0] JSL   $80C62B */
        SUSPEND(jsl_call(s, 0xed27));
        B.registers.stack_pointer -= 3u;
        goto source_c62b_m0x0;
source_ed2b_m0x0: /* $80:ED2B [M=0 X=0] LDA   #$7F */
        IMM(0xed2b, 3);
        set_a(s, 0x007fu);
        goto source_ed2e_m0x0;
source_ed2e_m0x0: /* $80:ED2E [M=0 X=0] STA   $0E */
        WRITE(0xed2e, 2, 0x0eu, 2, false, A);
        goto source_ed30_m0x0;
source_ed30_m0x0: /* $80:ED30 [M=0 X=0] LDA   #$2000 */
        IMM(0xed30, 3);
        set_a(s, 0x2000u);
        goto source_ed33_m0x0;
source_ed33_m0x0: /* $80:ED33 [M=0 X=0] STA   $0C */
        WRITE(0xed33, 2, 0x0cu, 2, false, A);
        goto source_ed35_m0x0;
source_ed35_m0x0: /* $80:ED35 [M=0 X=0] LDA   $7E7E6A */
        READ(0xed35, 4, 0x7e7e6au, 2, false);
        set_a(s, B.read_value);
        goto source_ed39_m0x0;
source_ed39_m0x0: /* $80:ED39 [M=0 X=0] TAY */
        IMP(0xed39);
        set_y(s, A);
        goto source_ed3a_m0x0;
source_ed3a_m0x0: /* $80:ED3A [M=0 X=0] JSL   $808BA1 */
        SUSPEND(jsl_call(s, 0xed3a));
        B.registers.stack_pointer -= 3u;
        goto source_8ba1_m0x0;
source_ed3e_m0x0: /* $80:ED3E [M=0 X=0] LDA   #$AE */
        IMM(0xed3e, 3);
        set_a(s, 0x00aeu);
        goto source_ed41_m0x0;
source_ed41_m0x0: /* $80:ED41 [M=0 X=0] STA   $0E */
        WRITE(0xed41, 2, 0x0eu, 2, false, A);
        goto source_ed43_m0x0;
source_ed43_m0x0: /* $80:ED43 [M=0 X=0] LDA   #$D153 */
        IMM(0xed43, 3);
        set_a(s, 0xd153u);
        goto source_ed46_m0x0;
source_ed46_m0x0: /* $80:ED46 [M=0 X=0] STA   $0C */
        WRITE(0xed46, 2, 0x0cu, 2, false, A);
        goto source_ed48_m0x0;
source_ed48_m0x0: /* $80:ED48 [M=0 X=0] LDA   #$7F */
        IMM(0xed48, 3);
        set_a(s, 0x007fu);
        goto source_ed4b_m0x0;
source_ed4b_m0x0: /* $80:ED4B [M=0 X=0] STA   $12 */
        WRITE(0xed4b, 2, 0x12u, 2, false, A);
        goto source_ed4d_m0x0;
source_ed4d_m0x0: /* $80:ED4D [M=0 X=0] LDA   #$2000 */
        IMM(0xed4d, 3);
        set_a(s, 0x2000u);
        goto source_ed50_m0x0;
source_ed50_m0x0: /* $80:ED50 [M=0 X=0] STA   $10 */
        WRITE(0xed50, 2, 0x10u, 2, false, A);
        goto source_ed52_m0x0;
source_ed52_m0x0: /* $80:ED52 [M=0 X=0] JSL   $80C62B */
        SUSPEND(jsl_call(s, 0xed52));
        B.registers.stack_pointer -= 3u;
        goto source_c62b_m0x0;
source_ed56_m0x0: /* $80:ED56 [M=0 X=0] LDA   #$7F */
        IMM(0xed56, 3);
        set_a(s, 0x007fu);
        goto source_ed59_m0x0;
source_ed59_m0x0: /* $80:ED59 [M=0 X=0] STA   $0E */
        WRITE(0xed59, 2, 0x0eu, 2, false, A);
        goto source_ed5b_m0x0;
source_ed5b_m0x0: /* $80:ED5B [M=0 X=0] LDA   #$2000 */
        IMM(0xed5b, 3);
        set_a(s, 0x2000u);
        goto source_ed5e_m0x0;
source_ed5e_m0x0: /* $80:ED5E [M=0 X=0] STA   $0C */
        WRITE(0xed5e, 2, 0x0cu, 2, false, A);
        goto source_ed60_m0x0;
source_ed60_m0x0: /* $80:ED60 [M=0 X=0] LDA   #$7F */
        IMM(0xed60, 3);
        set_a(s, 0x007fu);
        goto source_ed63_m0x0;
source_ed63_m0x0: /* $80:ED63 [M=0 X=0] STA   $0E */
        WRITE(0xed63, 2, 0x0eu, 2, false, A);
        goto source_ed65_m0x0;
source_ed65_m0x0: /* $80:ED65 [M=0 X=0] LDA   #$2000 */
        IMM(0xed65, 3);
        set_a(s, 0x2000u);
        goto source_ed68_m0x0;
source_ed68_m0x0: /* $80:ED68 [M=0 X=0] STA   $0C */
        WRITE(0xed68, 2, 0x0cu, 2, false, A);
        goto source_ed6a_m0x0;
source_ed6a_m0x0: /* $80:ED6A [M=0 X=0] LDX   #$00 */
        IMM(0xed6a, 3);
        set_x(s, 0x0000u);
        goto source_ed6d_m0x0;
source_ed6d_m0x0: /* $80:ED6D [M=0 X=0] LDY   #$00 */
        IMM(0xed6d, 3);
        set_y(s, 0x0000u);
        goto source_ed70_m0x0;
source_ed70_m0x0: /* $80:ED70 [M=0 X=0] LDA   $7E7E6C */
        READ(0xed70, 4, 0x7e7e6cu, 2, false);
        set_a(s, B.read_value);
        goto source_ed74_m0x0;
source_ed74_m0x0: /* $80:ED74 [M=0 X=0] STA   $05DB */
        WRITE(0xed74, 3, DB(0x05dbu), 2, false, A);
        goto source_ed77_m0x0;
source_ed77_m0x0: /* $80:ED77 [M=0 X=0] JSL   $808CD0 */
        SUSPEND(jsl_call(s, 0xed77));
        B.registers.stack_pointer -= 3u;
        goto source_8cd0_m0x0;
source_ed7b_m0x0: /* $80:ED7B [M=0 X=0] JSL   $8086DA */
        SUSPEND(jsl_call(s, 0xed7b));
        B.registers.stack_pointer -= 3u;
        goto source_86da_m0x0;
source_ed7f_m0x0: /* $80:ED7F [M=0 X=0] LDA   #$A6 */
        IMM(0xed7f, 3);
        set_a(s, 0x00a6u);
        goto source_ed82_m0x0;
source_ed82_m0x0: /* $80:ED82 [M=0 X=0] STA   $0E */
        WRITE(0xed82, 2, 0x0eu, 2, false, A);
        goto source_ed84_m0x0;
source_ed84_m0x0: /* $80:ED84 [M=0 X=0] LDA   #$C5FC */
        IMM(0xed84, 3);
        set_a(s, 0xc5fcu);
        goto source_ed87_m0x0;
source_ed87_m0x0: /* $80:ED87 [M=0 X=0] STA   $0C */
        WRITE(0xed87, 2, 0x0cu, 2, false, A);
        goto source_ed89_m0x0;
source_ed89_m0x0: /* $80:ED89 [M=0 X=0] LDA   #$7F */
        IMM(0xed89, 3);
        set_a(s, 0x007fu);
        goto source_ed8c_m0x0;
source_ed8c_m0x0: /* $80:ED8C [M=0 X=0] STA   $12 */
        WRITE(0xed8c, 2, 0x12u, 2, false, A);
        goto source_ed8e_m0x0;
source_ed8e_m0x0: /* $80:ED8E [M=0 X=0] LDA   #$2000 */
        IMM(0xed8e, 3);
        set_a(s, 0x2000u);
        goto source_ed91_m0x0;
source_ed91_m0x0: /* $80:ED91 [M=0 X=0] STA   $10 */
        WRITE(0xed91, 2, 0x10u, 2, false, A);
        goto source_ed93_m0x0;
source_ed93_m0x0: /* $80:ED93 [M=0 X=0] JSL   $80C62B */
        SUSPEND(jsl_call(s, 0xed93));
        B.registers.stack_pointer -= 3u;
        goto source_c62b_m0x0;
source_ed97_m0x0: /* $80:ED97 [M=0 X=0] LDA   #$7F */
        IMM(0xed97, 3);
        set_a(s, 0x007fu);
        goto source_ed9a_m0x0;
source_ed9a_m0x0: /* $80:ED9A [M=0 X=0] STA   $0E */
        WRITE(0xed9a, 2, 0x0eu, 2, false, A);
        goto source_ed9c_m0x0;
source_ed9c_m0x0: /* $80:ED9C [M=0 X=0] LDA   #$2000 */
        IMM(0xed9c, 3);
        set_a(s, 0x2000u);
        goto source_ed9f_m0x0;
source_ed9f_m0x0: /* $80:ED9F [M=0 X=0] STA   $0C */
        WRITE(0xed9f, 2, 0x0cu, 2, false, A);
        goto source_eda1_m0x0;
source_eda1_m0x0: /* $80:EDA1 [M=0 X=0] LDA   $7E7E6A */
        READ(0xeda1, 4, 0x7e7e6au, 2, false);
        set_a(s, B.read_value);
        goto source_eda5_m0x0;
source_eda5_m0x0: /* $80:EDA5 [M=0 X=0] CLC */
        IMP(0xeda5);
        P = (uint8_t)(P & 0xfeu);
        goto source_eda6_m0x0;
source_eda6_m0x0: /* $80:EDA6 [M=0 X=0] ADC   #$0360 */
        IMM(0xeda6, 3);
        add_sub(s, 0x0360u, false);
        goto source_eda9_m0x0;
source_eda9_m0x0: /* $80:EDA9 [M=0 X=0] TAY */
        IMP(0xeda9);
        set_y(s, A);
        goto source_edaa_m0x0;
source_edaa_m0x0: /* $80:EDAA [M=0 X=0] JSL   $808BA1 */
        SUSPEND(jsl_call(s, 0xedaa));
        B.registers.stack_pointer -= 3u;
        goto source_8ba1_m0x0;
source_edae_m0x0: /* $80:EDAE [M=0 X=0] LDA   #$AF */
        IMM(0xedae, 3);
        set_a(s, 0x00afu);
        goto source_edb1_m0x0;
source_edb1_m0x0: /* $80:EDB1 [M=0 X=0] STA   $0E */
        WRITE(0xedb1, 2, 0x0eu, 2, false, A);
        goto source_edb3_m0x0;
source_edb3_m0x0: /* $80:EDB3 [M=0 X=0] LDA   #$97AA */
        IMM(0xedb3, 3);
        set_a(s, 0x97aau);
        goto source_edb6_m0x0;
source_edb6_m0x0: /* $80:EDB6 [M=0 X=0] STA   $0C */
        WRITE(0xedb6, 2, 0x0cu, 2, false, A);
        goto source_edb8_m0x0;
source_edb8_m0x0: /* $80:EDB8 [M=0 X=0] LDA   #$7F */
        IMM(0xedb8, 3);
        set_a(s, 0x007fu);
        goto source_edbb_m0x0;
source_edbb_m0x0: /* $80:EDBB [M=0 X=0] STA   $12 */
        WRITE(0xedbb, 2, 0x12u, 2, false, A);
        goto source_edbd_m0x0;
source_edbd_m0x0: /* $80:EDBD [M=0 X=0] LDA   #$2000 */
        IMM(0xedbd, 3);
        set_a(s, 0x2000u);
        goto source_edc0_m0x0;
source_edc0_m0x0: /* $80:EDC0 [M=0 X=0] STA   $10 */
        WRITE(0xedc0, 2, 0x10u, 2, false, A);
        goto source_edc2_m0x0;
source_edc2_m0x0: /* $80:EDC2 [M=0 X=0] JSL   $80C62B */
        SUSPEND(jsl_call(s, 0xedc2));
        B.registers.stack_pointer -= 3u;
        goto source_c62b_m0x0;
source_edc6_m0x0: /* $80:EDC6 [M=0 X=0] LDA   #$7F */
        IMM(0xedc6, 3);
        set_a(s, 0x007fu);
        goto source_edc9_m0x0;
source_edc9_m0x0: /* $80:EDC9 [M=0 X=0] STA   $0E */
        WRITE(0xedc9, 2, 0x0eu, 2, false, A);
        goto source_edcb_m0x0;
source_edcb_m0x0: /* $80:EDCB [M=0 X=0] LDA   #$2000 */
        IMM(0xedcb, 3);
        set_a(s, 0x2000u);
        goto source_edce_m0x0;
source_edce_m0x0: /* $80:EDCE [M=0 X=0] STA   $0C */
        WRITE(0xedce, 2, 0x0cu, 2, false, A);
        goto source_edd0_m0x0;
source_edd0_m0x0: /* $80:EDD0 [M=0 X=0] LDA   #$36 */
        IMM(0xedd0, 3);
        set_a(s, 0x0036u);
        goto source_edd3_m0x0;
source_edd3_m0x0: /* $80:EDD3 [M=0 X=0] STA   $166B */
        WRITE(0xedd3, 3, DB(0x166bu), 2, false, A);
        goto source_edd6_m0x0;
source_edd6_m0x0: /* $80:EDD6 [M=0 X=0] LDA   #$00 */
        IMM(0xedd6, 3);
        set_a(s, 0x0000u);
        goto source_edd9_m0x0;
source_edd9_m0x0: /* $80:EDD9 [M=0 X=0] JSL   $80EA4B */
        SUSPEND(jsl_call(s, 0xedd9));
        B.registers.stack_pointer -= 3u;
        goto source_ea4b_m0x0;
source_eddd_m0x0: /* $80:EDDD [M=0 X=0] LDA   #$7F */
        IMM(0xeddd, 3);
        set_a(s, 0x007fu);
        goto source_ede0_m0x0;
source_ede0_m0x0: /* $80:EDE0 [M=0 X=0] STA   $0E */
        WRITE(0xede0, 2, 0x0eu, 2, false, A);
        goto source_ede2_m0x0;
source_ede2_m0x0: /* $80:EDE2 [M=0 X=0] LDA   #$2000 */
        IMM(0xede2, 3);
        set_a(s, 0x2000u);
        goto source_ede5_m0x0;
source_ede5_m0x0: /* $80:EDE5 [M=0 X=0] STA   $0C */
        WRITE(0xede5, 2, 0x0cu, 2, false, A);
        goto source_ede7_m0x0;
source_ede7_m0x0: /* $80:EDE7 [M=0 X=0] LDX   #$00 */
        IMM(0xede7, 3);
        set_x(s, 0x0000u);
        goto source_edea_m0x0;
source_edea_m0x0: /* $80:EDEA [M=0 X=0] LDY   #$00 */
        IMM(0xedea, 3);
        set_y(s, 0x0000u);
        goto source_eded_m0x0;
source_eded_m0x0: /* $80:EDED [M=0 X=0] LDA   $7E7E6C */
        READ(0xeded, 4, 0x7e7e6cu, 2, false);
        set_a(s, B.read_value);
        goto source_edf1_m0x0;
source_edf1_m0x0: /* $80:EDF1 [M=0 X=0] STA   $05DB */
        WRITE(0xedf1, 3, DB(0x05dbu), 2, false, A);
        goto source_edf4_m0x0;
source_edf4_m0x0: /* $80:EDF4 [M=0 X=0] JSL   $808CD0 */
        SUSPEND(jsl_call(s, 0xedf4));
        B.registers.stack_pointer -= 3u;
        goto source_8cd0_m0x0;
source_edf8_m0x0: /* $80:EDF8 [M=0 X=0] RTL */
        PULL(0xedf8, 3, false);
        s->return_pc = ((uint32_t)L.read_bank << 16) | (uint16_t)(B.read_value + 1u);
        B.status = NBA_CODEC_WORK_DONE;
        return false;
source_edf9_m0x0: /* $80:EDF9 [M=0 X=0] LDA   #$00 */
        IMM(0xedf9, 3);
        set_a(s, 0x0000u);
        goto source_edfc_m0x0;
source_edfc_m0x0: /* $80:EDFC [M=0 X=0] JMP   $ECD2 */
        JUMP(0xedfc);
        goto source_ecd2_m0x0;
    default: return unsupported(s);
    }
}
