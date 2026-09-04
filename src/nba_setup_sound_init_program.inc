/* Generated from canonical ROM only by tools/generate_setup_sound_init.py.
 * No capture, fitted cost, opcode dispatcher or runtime ROM execution.
 * Each label is static C source flow; SUSPEND labels are C continuations. */
static bool advance(NbaSetupSoundInit *s)
{
    switch (B.resume) {
    case 0: goto source_9b73_m0x0;
source_9b73_m0x0: /* $80:9B73 [M=0 X=0] PHP */
        PUSH(0x9b73, 1, P, 1);
        goto source_9b74_m0x0;
source_9b74_m0x0: /* $80:9B74 [M=0 X=0] SEP   #$20 */
        MODE(0x9b74);
        status_set(s, P | 0x20u);
        goto source_9b76_m1x0;
source_9b76_m1x0: /* $80:9B76 [M=1 X=0] PHA */
        PUSH(0x9b76, 1, A, 1);
        goto source_9b77_m1x0;
source_9b77_m1x0: /* $80:9B77 [M=1 X=0] PHB */
        PUSH(0x9b77, 1, B.registers.data_bank, 1);
        goto source_9b78_m1x0;
source_9b78_m1x0: /* $80:9B78 [M=1 X=0] PLA */
        PULL(0x9b78, 1, false);
        set_a(s, B.read_value);
        goto source_9b79_m1x0;
source_9b79_m1x0: /* $80:9B79 [M=1 X=0] CMP   #$7E */
        IMM(0x9b79, 2);
        compare(s, A, 0x007eu, 1);
        goto source_9b7b_m1x0;
source_9b7b_m1x0: /* $80:9B7B [M=1 X=0] BEQ   $9B81 */
        BRANCH(0x9b7b, (P & 2u) != 0);
        if (B.branch) { goto source_9b81_m1x0; }
        goto source_9b7d_m1x0;
source_9b7d_m1x0: /* $80:9B7D [M=1 X=0] CMP   #$7F */
        IMM(0x9b7d, 2);
        compare(s, A, 0x007fu, 1);
        goto source_9b7f_m1x0;
source_9b7f_m1x0: /* $80:9B7F [M=1 X=0] BNE   $9B83 */
        BRANCH(0x9b7f, (P & 2u) == 0);
        if (B.branch) { goto source_9b83_m1x0; }
        goto source_9b81_m1x0;
source_9b81_m1x0: /* $80:9B81 [M=1 X=0] PHK */
        PUSH(0x9b81, 1, 0x80, 1);
        goto source_9b82_m1x0;
source_9b82_m1x0: /* $80:9B82 [M=1 X=0] PLB */
        PULL(0x9b82, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_9b83_m1x0;
source_9b83_m1x0: /* $80:9B83 [M=1 X=0] PLA */
        PULL(0x9b83, 1, false);
        set_a(s, B.read_value);
        goto source_9b84_m1x0;
source_9b84_m1x0: /* $80:9B84 [M=1 X=0] PLP */
        PULL(0x9b84, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_9b85_m0x0;
source_9b85_m0x0: /* $80:9B85 [M=0 X=0] PHP */
        PUSH(0x9b85, 1, P, 1);
        goto source_9b86_m0x0;
source_9b86_m0x0: /* $80:9B86 [M=0 X=0] SEP   #$20 */
        MODE(0x9b86);
        status_set(s, P | 0x20u);
        goto source_9b88_m1x0;
source_9b88_m1x0: /* $80:9B88 [M=1 X=0] REP   #$10 */
        MODE(0x9b88);
        status_set(s, P & 0xefu);
        goto source_9b8a_m1x0;
source_9b8a_m1x0: /* $80:9B8A [M=1 X=0] DEC   $53 */
        CHANGE(0x9b8a, 2, 0x53u, 1, false, SOUND_DEC);
        goto source_9b8c_m1x0;
source_9b8c_m1x0: /* $80:9B8C [M=1 X=0] LDX   $33 */
        READ(0x9b8c, 2, 0x33u, 2, false);
        set_x(s, B.read_value);
        goto source_9b8e_m1x0;
source_9b8e_m1x0: /* $80:9B8E [M=1 X=0] STX   $5A */
        WRITE(0x9b8e, 2, 0x5au, 2, false, X);
        goto source_9b90_m1x0;
source_9b90_m1x0: /* $80:9B90 [M=1 X=0] LDX   #$01C3 */
        IMM(0x9b90, 3);
        set_x(s, 0x01c3u);
        goto source_9b93_m1x0;
source_9b93_m1x0: /* $80:9B93 [M=1 X=0] STZ   $062A,X */
        WRITE(0x9b93, 3, DB(0x062au + X), 1, true, 0);
        goto source_9b96_m1x0;
source_9b96_m1x0: /* $80:9B96 [M=1 X=0] DEX */
        IMP(0x9b96);
        set_x(s, X - 1u);
        goto source_9b97_m1x0;
source_9b97_m1x0: /* $80:9B97 [M=1 X=0] BPL   $9B93 */
        BRANCH(0x9b97, (P & 0x80u) == 0);
        if (B.branch) { goto source_9b93_m1x0; }
        goto source_9b99_m1x0;
source_9b99_m1x0: /* $80:9B99 [M=1 X=0] SEP   #$10 */
        MODE(0x9b99);
        status_set(s, P | 0x10u);
        goto source_9b9b_m1x1;
source_9b9b_m1x1: /* $80:9B9B [M=1 X=1] LDA   #$FF */
        IMM(0x9b9b, 2);
        set_a(s, 0x00ffu);
        goto source_9b9d_m1x1;
source_9b9d_m1x1: /* $80:9B9D [M=1 X=1] STA   $07CD */
        WRITE(0x9b9d, 3, DB(0x07cdu), 1, false, A);
        goto source_9ba0_m1x1;
source_9ba0_m1x1: /* $80:9BA0 [M=1 X=1] STA   $07CE */
        WRITE(0x9ba0, 3, DB(0x07ceu), 1, false, A);
        goto source_9ba3_m1x1;
source_9ba3_m1x1: /* $80:9BA3 [M=1 X=1] STA   $07CF */
        WRITE(0x9ba3, 3, DB(0x07cfu), 1, false, A);
        goto source_9ba6_m1x1;
source_9ba6_m1x1: /* $80:9BA6 [M=1 X=1] LDA   #$7F */
        IMM(0x9ba6, 2);
        set_a(s, 0x007fu);
        goto source_9ba8_m1x1;
source_9ba8_m1x1: /* $80:9BA8 [M=1 X=1] STA   $062F */
        WRITE(0x9ba8, 3, DB(0x062fu), 1, false, A);
        goto source_9bab_m1x1;
source_9bab_m1x1: /* $80:9BAB [M=1 X=1] STZ   $062E */
        WRITE(0x9bab, 3, DB(0x062eu), 1, false, 0);
        goto source_9bae_m1x1;
source_9bae_m1x1: /* $80:9BAE [M=1 X=1] INC   A */
        IMP(0x9bae);
        accumulator_change(s, SOUND_INC);
        goto source_9baf_m1x1;
source_9baf_m1x1: /* $80:9BAF [M=1 X=1] STA   $062D */
        WRITE(0x9baf, 3, DB(0x062du), 1, false, A);
        goto source_9bb2_m1x1;
source_9bb2_m1x1: /* $80:9BB2 [M=1 X=1] JSL   $809BD5 */
        SUSPEND(jsl_call(s, 0x9bb2));
        B.registers.stack_pointer -= 3u;
        goto source_9bd5_m1x1;
source_9bd5_m1x1: /* $80:9BD5 [M=1 X=1] PHP */
        PUSH(0x9bd5, 1, P, 1);
        goto source_9bd6_m1x1;
source_9bd6_m1x1: /* $80:9BD6 [M=1 X=1] PHB */
        PUSH(0x9bd6, 1, B.registers.data_bank, 1);
        goto source_9bd7_m1x1;
source_9bd7_m1x1: /* $80:9BD7 [M=1 X=1] PHK */
        PUSH(0x9bd7, 1, 0x80, 1);
        goto source_9bd8_m1x1;
source_9bd8_m1x1: /* $80:9BD8 [M=1 X=1] PLB */
        PULL(0x9bd8, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_9bd9_m1x1;
source_9bd9_m1x1: /* $80:9BD9 [M=1 X=1] PHP */
        PUSH(0x9bd9, 1, P, 1);
        goto source_9bda_m1x1;
source_9bda_m1x1: /* $80:9BDA [M=1 X=1] SEP   #$20 */
        MODE(0x9bda);
        status_set(s, P | 0x20u);
        goto source_9bdc_m1x1;
source_9bdc_m1x1: /* $80:9BDC [M=1 X=1] PHA */
        PUSH(0x9bdc, 1, A, 1);
        goto source_9bdd_m1x1;
source_9bdd_m1x1: /* $80:9BDD [M=1 X=1] PHB */
        PUSH(0x9bdd, 1, B.registers.data_bank, 1);
        goto source_9bde_m1x1;
source_9bde_m1x1: /* $80:9BDE [M=1 X=1] PLA */
        PULL(0x9bde, 1, false);
        set_a(s, B.read_value);
        goto source_9bdf_m1x1;
source_9bdf_m1x1: /* $80:9BDF [M=1 X=1] CMP   #$7E */
        IMM(0x9bdf, 2);
        compare(s, A, 0x007eu, 1);
        goto source_9be1_m1x1;
source_9be1_m1x1: /* $80:9BE1 [M=1 X=1] BEQ   $9BE7 */
        BRANCH(0x9be1, (P & 2u) != 0);
        if (B.branch) { goto source_9be7_m1x1; }
        goto source_9be3_m1x1;
source_9be3_m1x1: /* $80:9BE3 [M=1 X=1] CMP   #$7F */
        IMM(0x9be3, 2);
        compare(s, A, 0x007fu, 1);
        goto source_9be5_m1x1;
source_9be5_m1x1: /* $80:9BE5 [M=1 X=1] BNE   $9BE9 */
        BRANCH(0x9be5, (P & 2u) == 0);
        if (B.branch) { goto source_9be9_m1x1; }
        goto source_9be7_m1x1;
source_9be7_m1x1: /* $80:9BE7 [M=1 X=1] PHK */
        PUSH(0x9be7, 1, 0x80, 1);
        goto source_9be8_m1x1;
source_9be8_m1x1: /* $80:9BE8 [M=1 X=1] PLB */
        PULL(0x9be8, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_9be9_m1x1;
source_9be9_m1x1: /* $80:9BE9 [M=1 X=1] PLA */
        PULL(0x9be9, 1, false);
        set_a(s, B.read_value);
        goto source_9bea_m1x1;
source_9bea_m1x1: /* $80:9BEA [M=1 X=1] PLP */
        PULL(0x9bea, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_9beb_m1x1;
source_9beb_m1x1: /* $80:9BEB [M=1 X=1] SEP   #$30 */
        MODE(0x9beb);
        status_set(s, P | 0x30u);
        goto source_9bed_m1x1;
source_9bed_m1x1: /* $80:9BED [M=1 X=1] DEC   $53 */
        CHANGE(0x9bed, 2, 0x53u, 1, false, SOUND_DEC);
        goto source_9bef_m1x1;
source_9bef_m1x1: /* $80:9BEF [M=1 X=1] LDY   #$07 */
        IMM(0x9bef, 2);
        set_y(s, 0x0007u);
        goto source_9bf1_m1x1;
source_9bf1_m1x1: /* $80:9BF1 [M=1 X=1] JSR   $A4FC */
        PUSH(0x9bf1, 3, 0x9bf3, 2);
        goto source_a4fc_m1x1;
source_a4fc_m1x1: /* $80:A4FC [M=1 X=1] SEP   #$30 */
        MODE(0xa4fc);
        status_set(s, P | 0x30u);
        goto source_a4fe_m1x1;
source_a4fe_m1x1: /* $80:A4FE [M=1 X=1] LDA   #$00 */
        IMM(0xa4fe, 2);
        set_a(s, 0x0000u);
        goto source_a500_m1x1;
source_a500_m1x1: /* $80:A500 [M=1 X=1] STA   $073A,Y */
        WRITE(0xa500, 3, DB(0x073au + Y), 1, true, A);
        goto source_a503_m1x1;
source_a503_m1x1: /* $80:A503 [M=1 X=1] LDA   #$FF */
        IMM(0xa503, 2);
        set_a(s, 0x00ffu);
        goto source_a505_m1x1;
source_a505_m1x1: /* $80:A505 [M=1 X=1] STA   $0742,Y */
        WRITE(0xa505, 3, DB(0x0742u + Y), 1, true, A);
        goto source_a508_m1x1;
source_a508_m1x1: /* $80:A508 [M=1 X=1] JMP   $AACD */
        JUMP(0xa508);
        goto source_aacd_m1x1;
source_aacd_m1x1: /* $80:AACD [M=1 X=1] LDA   $2140 */
        READ(0xaacd, 3, DB(0x2140u), 1, false);
        return boundary(s, 0x80aacdu, NBA_SOUND_INIT_UNIMPLEMENTED);
    default: return unsupported(s);
    }
}
