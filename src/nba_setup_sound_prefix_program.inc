/* Generated from canonical ROM only by tools/generate_setup_sound_prefix_work.py.
 * No capture, fitted cost, opcode dispatcher or runtime ROM execution.
 * Each label is static C source flow; SUSPEND labels are C continuations. */
static bool advance(NbaSetupSoundPrefix *s)
{
    switch (B.resume) {
    case 0: goto source_a137_m0x1;
source_9fa1_m1x1: /* $80:9FA1 [M=1 X=1] PHP */
        PUSH(0x9fa1, 1, P, 1);
        goto source_9fa2_m1x1;
source_9fa2_m1x1: /* $80:9FA2 [M=1 X=1] SEP   #$30 */
        MODE(0x9fa2);
        status_set(s, P | 0x30u);
        goto source_9fa4_m1x1;
source_9fa4_m1x1: /* $80:9FA4 [M=1 X=1] LDA   $000053 */
        READ(0x9fa4, 4, 0x000053u, 1, false);
        set_a(s, B.read_value);
        goto source_9fa8_m1x1;
source_9fa8_m1x1: /* $80:9FA8 [M=1 X=1] BEQ   $9FAD */
        BRANCH(0x9fa8, (P & 2u) != 0);
        if (B.branch) { goto source_9fad_m1x1; }
        goto source_9faa_m1x1;
source_9faa_m1x1: /* $80:9FAA [M=1 X=1] JMP   $A0B2 */
        JUMP(0x9faa);
        goto source_a0b2_m1x1;
source_9fad_m1x1: /* $80:9FAD [M=1 X=1] LDA   $000637 */
        READ(0x9fad, 4, 0x000637u, 1, false);
        set_a(s, B.read_value);
        goto source_9fb1_m1x1;
source_9fb1_m1x1: /* $80:9FB1 [M=1 X=1] BNE   $9FB6 */
        BRANCH(0x9fb1, (P & 2u) == 0);
        if (B.branch) { return boundary(s, 0x809fb6u, NBA_SOUND_PREFIX_UNIMPLEMENTED); }
        goto source_9fb3_m1x1;
source_9fb3_m1x1: /* $80:9FB3 [M=1 X=1] JMP   $A0B2 */
        JUMP(0x9fb3);
        goto source_a0b2_m1x1;
source_a0b2_m1x1: /* $80:A0B2 [M=1 X=1] PLP */
        PULL(0xa0b2, 1, false);
        status_set(s, (uint8_t)B.read_value);
        goto source_a0b3_m1x1;
source_a0b3_m1x1: /* $80:A0B3 [M=1 X=1] RTL */
        PULL(0xa0b3, 3, false);
        if (s->read_bank != 0x80) return unsupported(s);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xa13d: goto source_a13d_m1x1;
        case 0xa164: goto source_a164_m1x1;
        case 0xa1b6: goto source_a1b6_m1x1;
        case 0xa2a7: return boundary(s, 0x80a2a7u, NBA_SOUND_PREFIX_UNIMPLEMENTED);
        default: return unsupported(s);
        }
source_a137_m0x1: /* $80:A137 [M=0 X=1] SEP   #$30 */
        MODE(0xa137);
        status_set(s, P | 0x30u);
        goto source_a139_m1x1;
source_a139_m1x1: /* $80:A139 [M=1 X=1] JSL   $809FA1 */
        SUSPEND(jsl_call(s, 0xa139));
        B.registers.stack_pointer -= 3u;
        goto source_9fa1_m1x1;
source_a13d_m1x1: /* $80:A13D [M=1 X=1] LDA   $53 */
        READ(0xa13d, 2, 0x53u, 1, false);
        set_a(s, B.read_value);
        goto source_a13f_m1x1;
source_a13f_m1x1: /* $80:A13F [M=1 X=1] BNE   $A1BE */
        BRANCH(0xa13f, (P & 2u) == 0);
        if (B.branch) { goto source_a1be_m1x1; }
        goto source_a141_m1x1;
source_a141_m1x1: /* $80:A141 [M=1 X=1] LDA   $062A */
        READ(0xa141, 3, DB(0x062au), 1, false);
        set_a(s, B.read_value);
        goto source_a144_m1x1;
source_a144_m1x1: /* $80:A144 [M=1 X=1] BEQ   $A154 */
        BRANCH(0xa144, (P & 2u) != 0);
        if (B.branch) { goto source_a154_m1x1; }
        return boundary(s, 0x80a146u, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a154_m1x1: /* $80:A154 [M=1 X=1] LDA   $062B */
        READ(0xa154, 3, DB(0x062bu), 1, false);
        set_a(s, B.read_value);
        goto source_a157_m1x1;
source_a157_m1x1: /* $80:A157 [M=1 X=1] BEQ   $A166 */
        BRANCH(0xa157, (P & 2u) != 0);
        if (B.branch) { goto source_a166_m1x1; }
        goto source_a159_m1x1;
source_a159_m1x1: /* $80:A159 [M=1 X=1] LDA   #$7F */
        IMM(0xa159, 2);
        set_a(s, 0x007fu);
        goto source_a15b_m1x1;
source_a15b_m1x1: /* $80:A15B [M=1 X=1] STA   $0634 */
        WRITE(0xa15b, 3, DB(0x0634u), 1, false, A);
        goto source_a15e_m1x1;
source_a15e_m1x1: /* $80:A15E [M=1 X=1] DEC   $062B */
        CHANGE(0xa15e, 3, DB(0x062bu), 1, false, SOUND_DEC);
        goto source_a161_m1x1;
source_a161_m1x1: /* $80:A161 [M=1 X=1] JSR   $A995 */
        PUSH(0xa161, 3, 0xa163, 2);
        return boundary(s, 0x80a995u, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a164_m1x1: /* $80:A164 [M=1 X=1] BRA   $A1AD */
        BRANCH(0xa164, true);
        goto source_a1ad_m1x1;
source_a166_m1x1: /* $80:A166 [M=1 X=1] REP   #$20 */
        MODE(0xa166);
        status_set(s, P & 0xdfu);
        goto source_a168_m0x1;
source_a168_m0x1: /* $80:A168 [M=0 X=1] LDA   $0635 */
        READ(0xa168, 3, DB(0x0635u), 2, false);
        set_a(s, B.read_value);
        goto source_a16b_m0x1;
source_a16b_m0x1: /* $80:A16B [M=0 X=1] BEQ   $A1AD */
        BRANCH(0xa16b, (P & 2u) != 0);
        if (B.branch) { goto source_a1ad_m0x1; }
        return boundary(s, 0x80a16du, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a1ad_m0x1: /* $80:A1AD [M=0 X=1] SEP   #$20 */
        MODE(0xa1ad);
        status_set(s, P | 0x20u);
        goto source_a1af_m1x1;
source_a1ad_m1x1: /* $80:A1AD [M=1 X=1] SEP   #$20 */
        MODE(0xa1ad);
        status_set(s, P | 0x20u);
        goto source_a1af_m1x1;
source_a1af_m1x1: /* $80:A1AF [M=1 X=1] LDA   $5A */
        READ(0xa1af, 2, 0x5au, 1, false);
        set_a(s, B.read_value);
        goto source_a1b1_m1x1;
source_a1b1_m1x1: /* $80:A1B1 [M=1 X=1] PHA */
        PUSH(0xa1b1, 1, A, 1);
        goto source_a1b2_m1x1;
source_a1b2_m1x1: /* $80:A1B2 [M=1 X=1] PLB */
        PULL(0xa1b2, 1, false);
        B.registers.data_bank = (uint8_t)B.read_value;
        nz(s, B.registers.data_bank, 1);
        goto source_a1b3_m1x1;
source_a1b3_m1x1: /* $80:A1B3 [M=1 X=1] JSR   $A1BF */
        PUSH(0xa1b3, 3, 0xa1b5, 2);
        goto source_a1bf_m1x1;
source_a1b6_m1x1: /* $80:A1B6 [M=1 X=1] LDA   $062C */
        READ(0xa1b6, 3, DB(0x062cu), 1, false);
        set_a(s, B.read_value);
        goto source_a1b9_m1x1;
source_a1b9_m1x1: /* $80:A1B9 [M=1 X=1] BEQ   $A1BE */
        BRANCH(0xa1b9, (P & 2u) != 0);
        if (B.branch) { goto source_a1be_m1x1; }
        goto source_a1bb_m1x1;
source_a1bb_m1x1: /* $80:A1BB [M=1 X=1] JMP   $A2CE */
        JUMP(0xa1bb);
        return boundary(s, 0x80a2ceu, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a1be_m1x1: /* $80:A1BE [M=1 X=1] RTS */
        return boundary(s, 0x80a1beu, NBA_SOUND_PREFIX_RETURN);
source_a1bf_m1x1: /* $80:A1BF [M=1 X=1] LDA   $07CD */
        READ(0xa1bf, 3, DB(0x07cdu), 1, false);
        set_a(s, B.read_value);
        goto source_a1c2_m1x1;
source_a1c2_m1x1: /* $80:A1C2 [M=1 X=1] BMI   $A1FA */
        BRANCH(0xa1c2, (P & 0x80u) != 0);
        if (B.branch) { goto source_a1fa_m1x1; }
        return boundary(s, 0x80a1c4u, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a1fa_m1x1: /* $80:A1FA [M=1 X=1] LDA   $07CE */
        READ(0xa1fa, 3, DB(0x07ceu), 1, false);
        set_a(s, B.read_value);
        goto source_a1fd_m1x1;
source_a1fd_m1x1: /* $80:A1FD [M=1 X=1] BMI   $A235 */
        BRANCH(0xa1fd, (P & 0x80u) != 0);
        if (B.branch) { goto source_a235_m1x1; }
        return boundary(s, 0x80a1ffu, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a235_m1x1: /* $80:A235 [M=1 X=1] LDA   $07CF */
        READ(0xa235, 3, DB(0x07cfu), 1, false);
        set_a(s, B.read_value);
        goto source_a238_m1x1;
source_a238_m1x1: /* $80:A238 [M=1 X=1] BMI   $A278 */
        BRANCH(0xa238, (P & 0x80u) != 0);
        if (B.branch) { goto source_a278_m1x1; }
        return boundary(s, 0x80a23au, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a278_m1x1: /* $80:A278 [M=1 X=1] LDX   #$00 */
        IMM(0xa278, 2);
        set_x(s, 0x0000u);
        goto source_a27a_m1x1;
source_a27a_m1x1: /* $80:A27A [M=1 X=1] TXY */
        IMP(0xa27a);
        set_y(s, X);
        goto source_a27b_m1x1;
source_a27b_m1x1: /* $80:A27B [M=1 X=1] STY   $55 */
        WRITE(0xa27b, 2, 0x55u, 1, false, Y);
        goto source_a27d_m1x1;
source_a27d_m1x1: /* $80:A27D [M=1 X=1] STX   $0648 */
        WRITE(0xa27d, 3, DB(0x0648u), 1, false, X);
        goto source_a280_m1x1;
source_a280_m1x1: /* $80:A280 [M=1 X=1] LDA   $073A,Y */
        READ(0xa280, 3, DB(0x073au + Y), 1, index_idle(s, 0x073au, Y));
        set_a(s, B.read_value);
        goto source_a283_m1x1;
source_a283_m1x1: /* $80:A283 [M=1 X=1] BNE   $A288 */
        BRANCH(0xa283, (P & 2u) == 0);
        if (B.branch) { goto source_a288_m1x1; }
        goto source_a285_m1x1;
source_a285_m1x1: /* $80:A285 [M=1 X=1] JMP   $A2BF */
        JUMP(0xa285);
        goto source_a2bf_m1x1;
source_a288_m1x1: /* $80:A288 [M=1 X=1] PHA */
        PUSH(0xa288, 1, A, 1);
        goto source_a289_m1x1;
source_a289_m1x1: /* $80:A289 [M=1 X=1] LDA   $0742,Y */
        READ(0xa289, 3, DB(0x0742u + Y), 1, index_idle(s, 0x0742u, Y));
        set_a(s, B.read_value);
        goto source_a28c_m1x1;
source_a28c_m1x1: /* $80:A28C [M=1 X=1] CMP   #$10 */
        IMM(0xa28c, 2);
        compare(s, A, 0x0010u, 1);
        goto source_a28e_m1x1;
source_a28e_m1x1: /* $80:A28E [M=1 X=1] BCC   $A293 */
        BRANCH(0xa28e, (P & 1u) == 0);
        if (B.branch) { goto source_a293_m1x1; }
        return boundary(s, 0x80a290u, NBA_SOUND_PREFIX_UNIMPLEMENTED);
source_a293_m1x1: /* $80:A293 [M=1 X=1] REP   #$20 */
        MODE(0xa293);
        status_set(s, P & 0xdfu);
        goto source_a295_m0x1;
source_a295_m0x1: /* $80:A295 [M=0 X=1] LDA   $076A,X */
        READ(0xa295, 3, DB(0x076au + X), 2, index_idle(s, 0x076au, X));
        set_a(s, B.read_value);
        goto source_a298_m0x1;
source_a298_m0x1: /* $80:A298 [M=0 X=1] STA   $58 */
        WRITE(0xa298, 2, 0x58u, 2, false, A);
        goto source_a29a_m0x1;
source_a29a_m0x1: /* $80:A29A [M=0 X=1] LDA   $078A,X */
        READ(0xa29a, 3, DB(0x078au + X), 2, index_idle(s, 0x078au, X));
        set_a(s, B.read_value);
        goto source_a29d_m0x1;
source_a29d_m0x1: /* $80:A29D [M=0 X=1] STA   $6C */
        WRITE(0xa29d, 2, 0x6cu, 2, false, A);
        goto source_a29f_m0x1;
source_a29f_m0x1: /* $80:A29F [M=0 X=1] INC   $07BA,X */
        CHANGE(0xa29f, 3, DB(0x07bau + X), 2, true, SOUND_INC);
        goto source_a2a2_m0x1;
source_a2a2_m0x1: /* $80:A2A2 [M=0 X=1] LDY   $55 */
        READ(0xa2a2, 2, 0x55u, 1, false);
        set_y(s, B.read_value);
        goto source_a2a4_m0x1;
source_a2a4_m0x1: /* $80:A2A4 [M=0 X=1] JSR   $AAE3 */
        PUSH(0xa2a4, 3, 0xa2a6, 2);
        goto source_aae3_m0x1;
source_a2bf_m1x1: /* $80:A2BF [M=1 X=1] SEP   #$20 */
        MODE(0xa2bf);
        status_set(s, P | 0x20u);
        goto source_a2c1_m1x1;
source_a2c1_m1x1: /* $80:A2C1 [M=1 X=1] INX */
        IMP(0xa2c1);
        set_x(s, X + 1u);
        goto source_a2c2_m1x1;
source_a2c2_m1x1: /* $80:A2C2 [M=1 X=1] INX */
        IMP(0xa2c2);
        set_x(s, X + 1u);
        goto source_a2c3_m1x1;
source_a2c3_m1x1: /* $80:A2C3 [M=1 X=1] LDY   $55 */
        READ(0xa2c3, 2, 0x55u, 1, false);
        set_y(s, B.read_value);
        goto source_a2c5_m1x1;
source_a2c5_m1x1: /* $80:A2C5 [M=1 X=1] INY */
        IMP(0xa2c5);
        set_y(s, Y + 1u);
        goto source_a2c6_m1x1;
source_a2c6_m1x1: /* $80:A2C6 [M=1 X=1] CPY   #$08 */
        IMM(0xa2c6, 2);
        compare(s, Y, 0x0008u, 1);
        goto source_a2c8_m1x1;
source_a2c8_m1x1: /* $80:A2C8 [M=1 X=1] BCS   $A2CD */
        BRANCH(0xa2c8, (P & 1u) != 0);
        if (B.branch) { goto source_a2cd_m1x1; }
        goto source_a2ca_m1x1;
source_a2ca_m1x1: /* $80:A2CA [M=1 X=1] JMP   $A27B */
        JUMP(0xa2ca);
        goto source_a27b_m1x1;
source_a2cd_m1x1: /* $80:A2CD [M=1 X=1] RTS */
        PULL(0xa2cd, 2, true);
        switch ((uint16_t)(B.read_value + 1u)) {
        case 0xa13d: goto source_a13d_m1x1;
        case 0xa164: goto source_a164_m1x1;
        case 0xa1b6: goto source_a1b6_m1x1;
        case 0xa2a7: return boundary(s, 0x80a2a7u, NBA_SOUND_PREFIX_UNIMPLEMENTED);
        default: return unsupported(s);
        }
source_aae3_m0x1: /* $80:AAE3 [M=0 X=1] PHP */
        PUSH(0xaae3, 1, P, 1);
        goto source_aae4_m0x1;
source_aae4_m0x1: /* $80:AAE4 [M=0 X=1] SEP   #$20 */
        MODE(0xaae4);
        status_set(s, P | 0x20u);
        goto source_aae6_m1x1;
source_aae6_m1x1: /* $80:AAE6 [M=1 X=1] LDA   $2140 */
        READ(0xaae6, 3, DB(0x2140u), 1, false);
        return boundary(s, 0x80aae6u, NBA_SOUND_PREFIX_UNIMPLEMENTED);
    default: return unsupported(s);
    }
}
