#include "nba_setup_codec_work.h"

#include <string.h>

/* The ROM program is translated below as source-specific C control flow.
 * resume is a C continuation label, never a 65816 PC or decoded opcode. Bus
 * recipe helpers describe only the statically selected source instructions.
 * The source PCs attest provenance and do not select runtime operations.
 * Canonical $80:BD1B..BE6A and $C62B..C682; see the work-proof document.
 */
static uint8_t bus_clocks(uint32_t address)
{
    uint8_t bank = (uint8_t)(address >> 16);
    uint16_t low = (uint16_t)address;
    if ((bank & 0x40u) != 0) return bank >= 0xc0u ? 6u : 8u;
    if (low >= 0x8000u) return bank >= 0x80u ? 6u : 8u;
    if (low < 0x2000u || (low >= 0x6000u)) return 8u;
    if (low >= 0x4000u && low < 0x4200u) return 12u;
    return 6u;
}

static void cycle(NbaSetupCodecWork *s, NbaCodecBusKind kind,
                  uint32_t address, uint8_t value, int8_t read_shift,
                  bool rmw_value)
{
    NbaCodecPendingCycle *p = &s->pending[s->pending_count++];
    p->cycle.source_pc = s->pending[0].cycle.source_pc;
    p->cycle.address = address;
    p->cycle.value = value;
    p->cycle.kind = kind;
    p->cycle.master_clocks = kind == NBA_CODEC_IDLE ? 6u : bus_clocks(address);
    p->cycle.instruction_end = false;
    p->read_shift = read_shift;
    p->rmw_value = rmw_value;
}

static void idle(NbaSetupCodecWork *s)
{
    cycle(s, NBA_CODEC_IDLE, 0, 0, -1, false);
}

static void instruction(NbaSetupCodecWork *s, uint16_t pc, uint8_t bytes)
{
    uint8_t i;
    s->pending_count = 0;
    s->pending_index = 0;
    s->read_value = 0;
    s->pending[0].cycle.source_pc = 0x800000u | pc;
    for (i = 0; i < bytes; ++i)
        cycle(s, NBA_CODEC_READ, 0x800000u | (uint16_t)(pc + i), 0, -1, false);
}

static void implied(NbaSetupCodecWork *s, uint16_t pc, uint8_t bytes,
                    uint8_t idles)
{
    instruction(s, pc, bytes);
    while (idles-- != 0) idle(s);
}

static void memory(NbaSetupCodecWork *s, uint16_t pc, uint8_t bytes,
                   uint32_t address, uint8_t width, bool indexed,
                   bool write, uint16_t value)
{
    uint8_t i;
    instruction(s, pc, bytes);
    /* X=0 throughout FB46: indexed absolute reads have the native idle even
     * without a page crossing. Replacing them with four-cycle byte loads
     * would erase real work. Writes always have the indexed idle. */
    if (indexed) idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, write ? NBA_CODEC_WRITE : NBA_CODEC_READ,
              (address + i) & 0xffffffu, (uint8_t)(value >> (i * 8u)),
              write ? -1 : (int8_t)(i * 8u), false);
}

static void rmw(NbaSetupCodecWork *s, uint16_t pc, uint8_t bytes,
                uint32_t address, bool indexed, int8_t delta)
{
    memory(s, pc, bytes, address, 1, indexed, false, 0);
    idle(s); /* native mode RMW uses an idle, not an emulation dummy write */
    cycle(s, NBA_CODEC_WRITE, address, 0, -1, true);
    s->rmw_delta = delta;
}

static void push(NbaSetupCodecWork *s, uint16_t pc, uint8_t bytes,
                 uint16_t value, uint8_t width)
{
    uint8_t i;
    instruction(s, pc, bytes);
    idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_WRITE, (uint16_t)(s->registers.stack_pointer - i),
              (uint8_t)(value >> ((width - i - 1u) * 8u)), -1, false);
}

static void pull(NbaSetupCodecWork *s, uint16_t pc, uint8_t width, bool rts)
{
    uint8_t i;
    instruction(s, pc, 1);
    idle(s);
    idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_READ, (uint16_t)(s->registers.stack_pointer + i + 1u),
              0, (int8_t)(i * 8u), false);
    if (rts) idle(s);
}

static void jsl_queue(NbaSetupCodecWork *s)
{
    instruction(s, 0xc62f, 3);
    cycle(s, NBA_CODEC_WRITE, s->registers.stack_pointer, 0x80, -1, false);
    idle(s);
    cycle(s, NBA_CODEC_READ, 0x80c632, 0, -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(s->registers.stack_pointer - 1u),
          0xc6, -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(s->registers.stack_pointer - 2u),
          0x32, -1, false);
}

/* One macro occurrence per physical line gives a stable, source-local C
 * suspension label. No C automatic variable survives an exposed bus event.
 * Actual hardware/memory values arrive through accept(), at the bus access.
 */
#define SUSPEND(recipe) do { recipe; s->resume = __LINE__; return true; case __LINE__:; } while (0)
#define IMP(pc) SUSPEND(implied(s, pc, 1, 1))
#define IMM(pc, bytes) SUSPEND(implied(s, pc, bytes, 0))
#define MODE(pc) SUSPEND(implied(s, pc, 2, 1))
#define JUMP(pc) SUSPEND(implied(s, pc, 3, 0))
#define BRANCH(pc, condition) do { s->branch = (condition); SUSPEND(implied(s, pc, 2, s->branch ? 1u : 0u)); } while (0)
#define READ(pc, bytes, address, width, indexed) SUSPEND(memory(s, pc, bytes, address, width, indexed, false, 0))
#define WRITE(pc, bytes, address, width, indexed, value) SUSPEND(memory(s, pc, bytes, address, width, indexed, true, value))
#define CHANGE(pc, bytes, address, indexed, delta) do { SUSPEND(rmw(s, pc, bytes, address, indexed, delta)); nz(s, (uint8_t)(s->read_value + (delta)), 1); } while (0)
#define PUSH(pc, bytes, value, width) do { SUSPEND(push(s, pc, bytes, value, width)); s->registers.stack_pointer -= (width); } while (0)
#define PULL(pc, width, is_rts) do { SUSPEND(pull(s, pc, width, is_rts)); s->registers.stack_pointer += (width); } while (0)
#define DB(address) (((uint32_t)s->registers.data_bank << 16) | (uint16_t)(address))
#define A s->registers.value
#define X s->registers.symbol
#define Y s->registers.stream_cursor
#define LOAD_A() do { A = s->read_value; nz(s, A, (s->registers.status & 0x20u) != 0 ? 1u : 2u); } while (0)
#define CALL_TREE(pc, kind) do { \
    if (s->return_depth >= 256u) { s->status = NBA_CODEC_WORK_LIMIT; return false; } \
    PUSH(pc, 3, (pc) + 2u, 2); \
    s->return_kind[s->return_depth] = (kind); \
    s->return_address[s->return_depth++] = (pc) + 3u; \
    goto recursive_tree; \
} while (0)

static void nz(NbaSetupCodecWork *s, uint16_t value, uint8_t width)
{
    uint16_t mask = width == 1 ? 0xffu : 0xffffu;
    uint16_t sign = width == 1 ? 0x80u : 0x8000u;
    s->registers.status = (uint8_t)((s->registers.status & ~0x82u) |
        ((value & mask) == 0 ? 2u : 0u) | ((value & sign) != 0 ? 0x80u : 0u));
}

static void compare(NbaSetupCodecWork *s, uint16_t value, uint16_t operand)
{
    nz(s, (uint16_t)(value - operand), 2);
    s->registers.status = (uint8_t)((s->registers.status & ~1u) | (value >= operand ? 1u : 0u));
}

static bool advance(NbaSetupCodecWork *s)
{
    switch (s->resume) {
    case 0:
        PUSH(0xc62b, 1, s->registers.status, 1);
        PUSH(0xc62c, 1, s->registers.data_bank, 1);
        MODE(0xc62d); s->registers.status &= 0xcfu;
        SUSPEND(jsl_queue(s));
        s->registers.stack_pointer -= 3u;
        PUSH(0x86da, 1, A, 2);
        READ(0x86db, 3, DB(0x0561), 2, false); LOAD_A();
        BRANCH(0x86de, (A & 0x8000u) != 0);
        if (s->branch) goto queue_immediate;
        READ(0x86e0, 2, 0x35, 2, false); LOAD_A();
        READ(0x86e2, 2, 0x37, 2, false); compare(s, A, s->read_value);
        BRANCH(0x86e4, A != s->read_value);
        if (s->branch) { s->status = NBA_CODEC_WORK_UNSUPPORTED; return false; }
        goto queue_done;
queue_immediate:
        READ(0x86e8, 2, 0x35, 2, false); LOAD_A();
        READ(0x86ea, 2, 0x37, 2, false); compare(s, A, s->read_value);
        BRANCH(0x86ec, A == s->read_value);
        if (!s->branch) { s->status = NBA_CODEC_WORK_UNSUPPORTED; return false; }
queue_done:
        PULL(0x86e6, 2, false); LOAD_A();
        PULL(0x86e7, 3, false);
        READ(0xc633, 2, 0x11, 2, false); LOAD_A();
        WRITE(0xc635, 3, DB(0x2182), 2, false, A);
        READ(0xc638, 2, 0x10, 2, false); LOAD_A();
        WRITE(0xc63a, 3, DB(0x2181), 2, false, A);
        READ(0xc63d, 2, 0x0d, 2, false); LOAD_A();
        PUSH(0xc63f, 1, A, 2);
        PULL(0xc640, 1, false); s->registers.data_bank = (uint8_t)s->read_value; nz(s, s->registers.data_bank, 1);
        PULL(0xc641, 1, false); s->registers.data_bank = (uint8_t)s->read_value; nz(s, s->registers.data_bank, 1);
        READ(0xc642, 2, 0x0c, 2, false); Y = s->read_value; nz(s, Y, 2);
        READ(0xc644, 3, DB(Y + 3u), 2, true); LOAD_A();
        SUSPEND(implied(s, 0xc647, 1, 2)); A = (uint16_t)((A << 8) | (A >> 8)); nz(s, A, 1);
        PUSH(0xc648, 1, A, 2);
        READ(0xc649, 3, DB(Y), 2, true); LOAD_A();
        IMP(0xc64c); ++Y; nz(s, Y, 2);
        IMP(0xc64d); ++Y; nz(s, Y, 2);
        IMM(0xc64e, 3); compare(s, A, 0xfb10);
        BRANCH(0xc651, A != 0xfb10u);
        if (!s->branch) { s->status = NBA_CODEC_WORK_UNSUPPORTED; return false; }
        IMM(0xc658, 3); compare(s, A, 0xfb46);
        BRANCH(0xc65b, A != 0xfb46u);
        if (s->branch) { s->status = NBA_CODEC_WORK_UNSUPPORTED; return false; }
        PUSH(0xc65d, 3, 0xc65f, 2);

        /* FB46 builds byte-type and left/right-symbol tables in mirrored
         * WRAM. Its hand-unrolled expansion below is deliberately retained:
         * flattening the tree changes both stack work and interrupt points. */
        IMP(0xbd1b); ++Y; nz(s, Y, 2);
        IMP(0xbd1c); ++Y; nz(s, Y, 2);
        IMM(0xbd1d, 3); A = 0; nz(s, A, 2);
        IMM(0xbd20, 3); X = 0x40; nz(s, X, 2);
clear_types:
        IMP(0xbd23); --X; nz(s, X, 2);
        IMP(0xbd24); --X; nz(s, X, 2);
        WRITE(0xbd25, 3, DB(0x100u + X), 2, true, A);
        WRITE(0xbd28, 3, DB(0x140u + X), 2, true, A);
        WRITE(0xbd2b, 3, DB(0x180u + X), 2, true, A);
        WRITE(0xbd2e, 3, DB(0x1c0u + X), 2, true, A);
        BRANCH(0xbd31, X != 0);
        if (s->branch) goto clear_types;
        MODE(0xbd33); s->registers.status |= 0x20u;
        IMP(0xbd35); ++Y; nz(s, Y, 2);
        READ(0xbd36, 3, DB(Y), 1, true); LOAD_A();
        IMP(0xbd39); X = A; nz(s, X, 2);
        CHANGE(0xbd3a, 3, DB(0x100u + X), true, 1);
        IMP(0xbd3d); ++Y; nz(s, Y, 2);
        READ(0xbd3e, 3, DB(Y), 1, true); LOAD_A();
        WRITE(0xbd41, 2, 0, 1, false, A);
        BRANCH(0xbd43, A == 0);
        if (s->branch) goto next_token;
dictionary:
        IMP(0xbd45); ++Y; nz(s, Y, 2);
        READ(0xbd46, 3, DB(Y), 1, true); LOAD_A();
        IMP(0xbd49); X = A; nz(s, X, 2);
        IMP(0xbd4a); ++Y; nz(s, Y, 2);
        READ(0xbd4b, 3, DB(Y), 1, true); LOAD_A();
        WRITE(0xbd4e, 3, DB(0x300u + X), 1, true, A);
        IMP(0xbd51); ++Y; nz(s, Y, 2);
        READ(0xbd52, 3, DB(Y), 1, true); LOAD_A();
        WRITE(0xbd55, 3, DB(0x400u + X), 1, true, A);
        CHANGE(0xbd58, 3, DB(0x100u + X), true, -1);
        CHANGE(0xbd5b, 2, 0, false, -1); s->operand = (uint8_t)(s->read_value - 1u);
        BRANCH(0xbd5d, s->operand != 0);
        if (s->branch) goto dictionary;
        BRANCH(0xbd5f, true);
        goto next_token;

finish_codec:
        PULL(0xbd81, 2, true);
        BRANCH(0xc660, true);
        MODE(0xc67d); s->registers.status &= 0xcfu;
        PULL(0xc67f, 2, false); X = s->read_value; nz(s, X, 2); s->output_size = X;
        PULL(0xc680, 1, false); s->registers.data_bank = (uint8_t)s->read_value; nz(s, s->registers.data_bank, 1);
        PULL(0xc681, 1, false); s->registers.status = (uint8_t)s->read_value;
        s->status = NBA_CODEC_WORK_DONE;
        return false; /* endpoint is C682 entry; final RTL is outside scope */

escape_dispatch:
        BRANCH(0xbd82, (A & 0x80u) != 0);
        if (s->branch) goto expand_root;
escape_payload:
        IMP(0xbd84); ++Y; nz(s, Y, 2);
        READ(0xbd85, 3, DB(Y), 1, true); LOAD_A();
        BRANCH(0xbd88, A == 0);
        if (s->branch) goto finish_codec;
        WRITE(0xbd8a, 3, DB(0x2180), 1, false, A);
        IMP(0xbd8d); ++Y; nz(s, Y, 2);
        READ(0xbd8e, 3, DB(Y), 1, true); LOAD_A();
        IMP(0xbd91); X = A; nz(s, X, 2);
        READ(0xbd92, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbd95, A != 0);
        if (s->branch) goto escape_dispatch;
literal:
        IMP(0xbd97); A = X; nz(s, A, 1);
        WRITE(0xbd98, 3, DB(0x2180), 1, false, A);
next_token:
        IMP(0xbd9b); ++Y; nz(s, Y, 2);
        READ(0xbd9c, 3, DB(Y), 1, true); LOAD_A();
        IMP(0xbd9f); X = A; nz(s, X, 2);
        READ(0xbda0, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbda3, A == 0);
        if (s->branch) goto literal;
        BRANCH(0xbda5, (A & 0x80u) == 0);
        if (s->branch) goto escape_payload;
expand_root:
        PUSH(0xbda7, 1, X, 2);
        READ(0xbda8, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbdab); X = A; nz(s, X, 2);
        READ(0xbdac, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbdaf, (A & 0x80u) != 0);
        if (s->branch) goto expand_left;
root_left_leaf:
        IMP(0xbdb1); A = X; nz(s, A, 1);
        WRITE(0xbdb2, 3, DB(0x2180), 1, false, A);
        PULL(0xbdb5, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbdb6, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbdb9); X = A; nz(s, X, 2);
        READ(0xbdba, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbdbd, A == 0);
        if (s->branch) goto literal;
        PUSH(0xbdbf, 1, X, 2);
        READ(0xbdc0, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbdc3); X = A; nz(s, X, 2);
        READ(0xbdc4, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbdc7, (A & 0x80u) != 0);
        if (s->branch) goto expand_right_left;
right_left_leaf:
        IMP(0xbdc9); A = X; nz(s, A, 1);
        WRITE(0xbdca, 3, DB(0x2180), 1, false, A);
        PULL(0xbdcd, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbdce, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbdd1); X = A; nz(s, X, 2);
        READ(0xbdd2, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbdd5, A == 0);
        if (s->branch) goto literal;
        PUSH(0xbdd7, 1, X, 2);
        READ(0xbdd8, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbddb); X = A; nz(s, X, 2);
        READ(0xbddc, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbddf, A == 0);
        if (s->branch) goto right_right_left_done;
        CALL_TREE(0xbde1, 1);
right_right_left_done:
        IMP(0xbde4); A = X; nz(s, A, 1);
        WRITE(0xbde5, 3, DB(0x2180), 1, false, A);
        PULL(0xbde8, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbde9, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbdec); X = A; nz(s, X, 2);
        READ(0xbded, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbdf0, A == 0);
        if (s->branch) goto literal;
        CALL_TREE(0xbdf2, 2);
right_right_done:
        BRANCH(0xbdf5, true);
        goto literal;

expand_right_left:
        PUSH(0xbd61, 1, X, 2);
        READ(0xbd62, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbd65); X = A; nz(s, X, 2);
        READ(0xbd66, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbd69, A == 0);
        if (s->branch) goto right_left_left_done;
        CALL_TREE(0xbd6b, 3);
right_left_left_done:
        IMP(0xbd6e); A = X; nz(s, A, 1);
        WRITE(0xbd6f, 3, DB(0x2180), 1, false, A);
        PULL(0xbd72, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbd73, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbd76); X = A; nz(s, X, 2);
        READ(0xbd77, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbd7a, A == 0);
        if (s->branch) goto right_left_leaf;
        CALL_TREE(0xbd7c, 4);
right_left_done:
        BRANCH(0xbd7f, true);
        goto right_left_leaf;

expand_left:
        PUSH(0xbdf7, 1, X, 2);
        READ(0xbdf8, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbdfb); X = A; nz(s, X, 2);
        READ(0xbdfc, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbdff, (A & 0x80u) != 0);
        if (s->branch) goto expand_left_left;
left_left_leaf:
        IMP(0xbe01); A = X; nz(s, A, 1);
        WRITE(0xbe02, 3, DB(0x2180), 1, false, A);
        PULL(0xbe05, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbe06, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbe09); X = A; nz(s, X, 2);
        READ(0xbe0a, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbe0d, A == 0);
        if (s->branch) goto root_left_leaf;
        PUSH(0xbe0f, 1, X, 2);
        READ(0xbe10, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbe13); X = A; nz(s, X, 2);
        READ(0xbe14, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbe17, A == 0);
        if (s->branch) goto left_right_left_done;
        CALL_TREE(0xbe19, 5);
left_right_left_done:
        IMP(0xbe1c); A = X; nz(s, A, 1);
        WRITE(0xbe1d, 3, DB(0x2180), 1, false, A);
        PULL(0xbe20, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbe21, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbe24); X = A; nz(s, X, 2);
        READ(0xbe25, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbe28, A == 0);
        if (s->branch) goto root_left_leaf;
        CALL_TREE(0xbe2a, 6);
left_right_done:
        BRANCH(0xbe2d, true);
        goto root_left_leaf;

expand_left_left:
        PUSH(0xbe2f, 1, X, 2);
        READ(0xbe30, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbe33); X = A; nz(s, X, 2);
        READ(0xbe34, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbe37, A == 0);
        if (s->branch) goto left_left_left_done;
        CALL_TREE(0xbe39, 7);
left_left_left_done:
        IMP(0xbe3c); A = X; nz(s, A, 1);
        WRITE(0xbe3d, 3, DB(0x2180), 1, false, A);
        PULL(0xbe40, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbe41, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbe44); X = A; nz(s, X, 2);
        READ(0xbe45, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbe48, A == 0);
        if (s->branch) goto left_left_leaf;
        CALL_TREE(0xbe4a, 8);
left_left_done:
        BRANCH(0xbe4d, true);
        goto left_left_leaf;

recursive_tree:
        PUSH(0xbe4f, 1, X, 2);
        READ(0xbe50, 3, DB(0x300u + X), 1, true); LOAD_A();
        IMP(0xbe53); X = A; nz(s, X, 2);
        READ(0xbe54, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbe57, A == 0);
        if (s->branch) goto recursive_left_done;
        CALL_TREE(0xbe59, 9);
recursive_left_done:
        IMP(0xbe5c); A = X; nz(s, A, 1);
        WRITE(0xbe5d, 3, DB(0x2180), 1, false, A);
        PULL(0xbe60, 2, false); X = s->read_value; nz(s, X, 2);
        READ(0xbe61, 3, DB(0x400u + X), 1, true); LOAD_A();
        IMP(0xbe64); X = A; nz(s, X, 2);
        READ(0xbe65, 3, DB(0x100u + X), 1, true); LOAD_A();
        BRANCH(0xbe68, A != 0);
        if (s->branch) goto recursive_tree;
        PULL(0xbe6a, 2, true);
        if (s->return_depth == 0) { s->status = NBA_CODEC_WORK_UNSUPPORTED; return false; }
        --s->return_depth;
        if ((uint16_t)(s->read_value + 1u) != s->return_address[s->return_depth]) {
            s->status = NBA_CODEC_WORK_UNSUPPORTED; return false;
        }
        switch (s->return_kind[s->return_depth]) {
        case 1: goto right_right_left_done;
        case 2: goto right_right_done;
        case 3: goto right_left_left_done;
        case 4: goto right_left_done;
        case 5: goto left_right_left_done;
        case 6: goto left_right_done;
        case 7: goto left_left_left_done;
        case 8: goto left_left_done;
        case 9: goto recursive_left_done;
        default: s->status = NBA_CODEC_WORK_UNSUPPORTED; return false;
        }
    default:
        s->status = NBA_CODEC_WORK_UNSUPPORTED;
        return false;
    }
}

static bool prepare(NbaSetupCodecWork *s)
{
    if (!advance(s)) return false;
    if (s->instructions >= s->instruction_limit) {
        s->status = NBA_CODEC_WORK_LIMIT;
        s->pending_count = 0;
        return false;
    }
    ++s->instructions;
    s->pending[s->pending_count - 1u].cycle.instruction_end = true;
    return true;
}

bool nba_setup_codec_work_begin(NbaSetupCodecWork *s,
                                const NbaCodecWorkEntry *entry,
                                uint64_t instruction_limit)
{
    if (s == NULL || entry == NULL || instruction_limit == 0 ||
        (entry->status & 0x38u) != 0 || (entry->data_bank & 0x40u) != 0)
        return false;
    memset(s, 0, sizeof(*s));
    s->registers = *entry;
    s->instruction_limit = instruction_limit;
    s->status = NBA_CODEC_WORK_RUNNING;
    return prepare(s);
}

bool nba_setup_codec_work_peek(const NbaSetupCodecWork *s,
                               NbaCodecBusCycle *out)
{
    const NbaCodecPendingCycle *p;
    if (s == NULL || out == NULL || s->status != NBA_CODEC_WORK_RUNNING ||
        s->pending_index >= s->pending_count) return false;
    p = &s->pending[s->pending_index];
    *out = p->cycle;
    if (p->rmw_value) out->value = (uint8_t)(s->read_value + s->rmw_delta);
    return true;
}

bool nba_setup_codec_work_accept(NbaSetupCodecWork *s, uint8_t value)
{
    const NbaCodecPendingCycle *p;
    if (s == NULL || s->status != NBA_CODEC_WORK_RUNNING ||
        s->pending_index >= s->pending_count) return false;
    p = &s->pending[s->pending_index++];
    if (p->cycle.kind == NBA_CODEC_READ && p->read_shift >= 0)
        s->read_value |= (uint16_t)((uint16_t)value << p->read_shift);
    if (s->pending_index == s->pending_count) (void)prepare(s);
    return true;
}
