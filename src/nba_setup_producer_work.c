#include "nba_setup_producer_work.h"

#include <string.h>

#define L s->local
#define B L.bus
#define A B.registers.value
#define X B.registers.symbol
#define Y B.registers.stream_cursor
#define P B.registers.status
#define DB(address) (((uint32_t)B.registers.data_bank << 16) | (uint16_t)(address))

typedef enum { PRODUCER_INC, PRODUCER_DEC, PRODUCER_ASL, PRODUCER_LSR, PRODUCER_ROL } ProducerChange;

static uint8_t bus_clocks(uint32_t address)
{
    uint8_t bank = (uint8_t)(address >> 16);
    uint16_t low = (uint16_t)address;
    if ((bank & 0x40u) != 0) return bank >= 0xc0u ? 6u : 8u;
    if (low >= 0x8000u) return bank >= 0x80u ? 6u : 8u;
    if (low < 0x2000u || low >= 0x6000u) return 8u;
    if (low >= 0x4000u && low < 0x4200u) return 12u;
    return 6u;
}

static void cycle(NbaSetupProducerWork *s, NbaCodecBusKind kind, uint32_t address,
                  uint8_t value, int8_t read_shift, bool changed_value)
{
    NbaCodecPendingCycle *p = &B.pending[B.pending_count];
    L.indirect_cycle[B.pending_count++] = 0;
    p->cycle.source_pc = B.pending[0].cycle.source_pc;
    p->cycle.address = address;
    p->cycle.value = value;
    p->cycle.kind = kind;
    p->cycle.master_clocks = kind == NBA_CODEC_IDLE ? 6u : bus_clocks(address);
    p->cycle.instruction_end = false;
    p->read_shift = read_shift;
    p->rmw_value = changed_value;
}

static void idle(NbaSetupProducerWork *s)
{
    cycle(s, NBA_CODEC_IDLE, 0, 0, -1, false);
}

static void instruction(NbaSetupProducerWork *s, uint16_t pc, uint8_t bytes)
{
    uint8_t i;
    B.pending_count = B.pending_index = 0;
    B.read_value = 0;
    L.pointer_read = 0;
    L.read_bank = 0;
    s->pointer_bank = 0;
    B.pending[0].cycle.source_pc = 0x800000u | pc;
    for (i = 0; i < bytes; ++i)
        cycle(s, NBA_CODEC_READ, 0x800000u | (uint16_t)(pc + i), 0, -1, false);
}

static void implied(NbaSetupProducerWork *s, uint16_t pc, uint8_t bytes, uint8_t idles)
{
    instruction(s, pc, bytes);
    while (idles-- != 0) idle(s);
}

static bool index_idle(const NbaSetupProducerWork *s, uint16_t base, uint16_t index)
{
    return (P & 0x10u) == 0 || (base & 255u) + index > 255u;
}

static void memory(NbaSetupProducerWork *s, uint16_t pc, uint8_t bytes,
                   uint32_t address, uint8_t width, bool indexed,
                   bool write, uint16_t value)
{
    uint8_t i;
    instruction(s, pc, bytes);
    if (indexed) idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, write ? NBA_CODEC_WRITE : NBA_CODEC_READ,
              (address + i) & 0xffffffu, (uint8_t)(value >> (i * 8u)),
              write ? -1 : (int8_t)(i * 8u), false);
}

static void indirect_read(NbaSetupProducerWork *s, uint16_t pc, uint8_t dp, uint8_t width)
{
    uint8_t i;
    instruction(s, pc, 2);
    cycle(s, NBA_CODEC_READ, dp, 0, 32, false);
    cycle(s, NBA_CODEC_READ, (uint16_t)(dp + 1u), 0, 40, false);
    for (i = 0; i < width; ++i) {
        cycle(s, NBA_CODEC_READ, i, 0, (int8_t)(8u * i), false);
        L.indirect_cycle[B.pending_count - 1u] = 1;
    }
}

static uint16_t changed(uint16_t old, uint8_t width, ProducerChange kind, uint8_t carry)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    switch (kind) {
    case PRODUCER_INC: return (uint16_t)((old + 1u) & mask);
    case PRODUCER_DEC: return (uint16_t)((old - 1u) & mask);
    case PRODUCER_ASL: return (uint16_t)((old << 1) & mask);
    case PRODUCER_LSR: return (uint16_t)((old & mask) >> 1);
    case PRODUCER_ROL: return (uint16_t)(((old << 1) | carry) & mask);
    }
    return 0;
}

static void nz(NbaSetupProducerWork *s, uint16_t value, uint8_t width)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    uint16_t sign = width == 1 ? 128u : 32768u;
    P = (uint8_t)((P & 0x7du) | ((value & mask) == 0 ? 2u : 0u) |
                  ((value & sign) != 0 ? 128u : 0u));
}

static void change_flags(NbaSetupProducerWork *s, uint16_t old, uint16_t value,
                         uint8_t width, ProducerChange kind)
{
    if (kind == PRODUCER_ASL || kind == PRODUCER_ROL)
        P = (uint8_t)((P & 0xfeu) | ((old & (width == 1 ? 128u : 32768u)) != 0 ? 1u : 0u));
    else if (kind == PRODUCER_LSR) P = (uint8_t)((P & 0xfeu) | (old & 1u));
    nz(s, value, width);
}

static void rmw(NbaSetupProducerWork *s, uint16_t pc, uint8_t bytes, uint32_t address,
                uint8_t width, bool indexed, ProducerChange kind)
{
    memory(s, pc, bytes, address, width, indexed, false, 0);
    idle(s);
    /* Native word RMW writes high byte first, then low byte. This order is
     * observable; ordinary 16-bit stores above write low byte first. */
    if (width == 2) cycle(s, NBA_CODEC_WRITE, (address + 1u) & 0xffffffu, 0, 8, true);
    cycle(s, NBA_CODEC_WRITE, address, 0, 0, true);
    L.rmw_width = width;
    L.change_kind = (uint8_t)kind;
    L.carry_in = P & 1u;
}

static void rmw_commit(NbaSetupProducerWork *s)
{
    uint16_t result = changed(B.read_value, L.rmw_width, (ProducerChange)L.change_kind, L.carry_in);
    change_flags(s, B.read_value, result, L.rmw_width, (ProducerChange)L.change_kind);
}

static void set_a(NbaSetupProducerWork *s, uint32_t value)
{
    uint8_t width = (P & 0x20u) != 0 ? 1u : 2u;
    A = width == 1 ? (uint16_t)((A & 0xff00u) | (value & 255u)) : (uint16_t)value;
    nz(s, A, width);
}

static void set_x(NbaSetupProducerWork *s, uint32_t value)
{
    uint8_t width = (P & 0x10u) != 0 ? 1u : 2u;
    X = (uint16_t)(width == 1 ? value & 255u : value);
    nz(s, X, width);
}

static void set_y(NbaSetupProducerWork *s, uint32_t value)
{
    uint8_t width = (P & 0x10u) != 0 ? 1u : 2u;
    Y = (uint16_t)(width == 1 ? value & 255u : value);
    nz(s, Y, width);
}

static void status_set(NbaSetupProducerWork *s, uint32_t value)
{
    P = (uint8_t)value;
    if ((P & 0x10u) != 0) { X &= 255u; Y &= 255u; }
}

static void accumulator_change(NbaSetupProducerWork *s, ProducerChange kind)
{
    uint8_t width = (P & 0x20u) != 0 ? 1u : 2u;
    uint16_t old = A;
    uint16_t value = changed(old, width, kind, P & 1u);
    set_a(s, value);
    change_flags(s, old, value, width, kind);
}

static void compare(NbaSetupProducerWork *s, uint16_t value, uint16_t operand, uint8_t width)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    value &= mask; operand &= mask;
    nz(s, (uint16_t)(value - operand), width);
    P = (uint8_t)((P & 0xfeu) | (value >= operand ? 1u : 0u));
}

static void add_sub(NbaSetupProducerWork *s, uint16_t operand, bool subtract)
{
    uint16_t mask = (P & 0x20u) != 0 ? 255u : 65535u;
    uint16_t sign = (P & 0x20u) != 0 ? 128u : 32768u;
    uint16_t old = A & mask;
    uint32_t rhs = subtract ? (operand ^ mask) & mask : operand & mask;
    uint32_t result = old + rhs + (P & 1u);
    uint8_t overflow = ((~(old ^ rhs) & (old ^ result) & sign) != 0) ? 0x40u : 0u;
    set_a(s, (uint16_t)(result & mask));
    P = (uint8_t)((P & 0xbeu) | overflow | (result > mask ? 1u : 0u));
}

static void bit_test(NbaSetupProducerWork *s, uint16_t operand)
{
    uint16_t sign = (P & 0x20u) != 0 ? 128u : 32768u;
    uint16_t mask = (P & 0x20u) != 0 ? 255u : 65535u;
    P = (uint8_t)((P & 0x3du) | ((A & operand & mask) == 0 ? 2u : 0u) |
                 ((operand & sign) != 0 ? 0x80u : 0u) | ((operand & (sign >> 1)) != 0 ? 0x40u : 0u));
}

static void push(NbaSetupProducerWork *s, uint16_t pc, uint8_t bytes, uint16_t value, uint8_t width)
{
    uint8_t i;
    instruction(s, pc, bytes);
    idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - i),
              (uint8_t)(value >> ((width - i - 1u) * 8u)), -1, false);
}

static void pull(NbaSetupProducerWork *s, uint16_t pc, uint8_t width, bool rts)
{
    uint8_t i;
    instruction(s, pc, 1);
    idle(s); idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_READ, (uint16_t)(B.registers.stack_pointer + i + 1u),
              0, (int8_t)(i * 8u), false);
    if (rts) idle(s);
}

static void jsl_call(NbaSetupProducerWork *s, uint16_t pc)
{
    instruction(s, pc, 3);
    cycle(s, NBA_CODEC_WRITE, B.registers.stack_pointer, 0x80, -1, false);
    idle(s);
    cycle(s, NBA_CODEC_READ, 0x800000u | (uint16_t)(pc + 3u), 0, -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - 1u), (uint8_t)((pc + 3u) >> 8), -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - 2u), (uint8_t)(pc + 3u), -1, false);
}

static void long_indirect(NbaSetupProducerWork *s, uint16_t pc, uint8_t dp,
                          uint8_t width, bool write, uint16_t value)
{
    uint8_t i;
    instruction(s, pc, 2);
    cycle(s, NBA_CODEC_READ, dp, 0, 32, false);
    cycle(s, NBA_CODEC_READ, (uint16_t)(dp + 1u), 0, 40, false);
    cycle(s, NBA_CODEC_READ, (uint16_t)(dp + 2u), 0, 48, false);
    for (i = 0; i < width; ++i) {
        cycle(s, write ? NBA_CODEC_WRITE : NBA_CODEC_READ, (uint32_t)Y + i,
              (uint8_t)(value >> (8u * i)), write ? -1 : (int8_t)(8u * i), false);
        L.indirect_cycle[B.pending_count - 1u] = 2;
    }
}

static bool select_codec(NbaSetupProducerWork *s)
{
    NbaCodecWorkStatus left=s->child.fb30.bus.status,right=s->child.fb46.status;
    NbaCodecBusCycle a,b;
    if (left==NBA_CODEC_WORK_UNSUPPORTED && right!=NBA_CODEC_WORK_UNSUPPORTED) s->codec_kind=46;
    else if (right==NBA_CODEC_WORK_UNSUPPORTED || left!=NBA_CODEC_WORK_RUNNING) s->codec_kind=30;
    else if (right!=NBA_CODEC_WORK_RUNNING) s->codec_kind=46;
    else {
        /* Both frozen continuations execute the same source wrapper. Feed
         * each actual prefix bus result to both until the source signature
         * branch selects its supported body. Never fetch or charge twice. */
        if (!nba_setup_fb30_work_peek(&s->child.fb30,&a) || !nba_setup_codec_work_peek(&s->child.fb46,&b) ||
            a.kind!=b.kind || a.address!=b.address || a.value!=b.value || a.source_pc!=b.source_pc ||
            a.master_clocks!=b.master_clocks || a.instruction_end!=b.instruction_end) {
            B.status=NBA_CODEC_WORK_UNSUPPORTED; return false;
        }
    }
    return true;
}

static bool begin_codec(NbaSetupProducerWork *s)
{
    uint64_t remaining=B.instruction_limit-B.instructions;
    bool left=nba_setup_fb30_work_begin(&s->child.fb30,&B.registers,remaining);
    bool right=nba_setup_codec_work_begin(&s->child.fb46,&B.registers,remaining);
    if (!left || !right) { B.status=NBA_CODEC_WORK_LIMIT; return false; }
    s->codec_kind=0;
    s->child_active=true;
    return select_codec(s);
}

static bool unsupported(NbaSetupProducerWork *s)
{
    B.status = NBA_CODEC_WORK_UNSUPPORTED;
    return false;
}

#define SUSPEND(recipe) do { recipe; B.resume = __LINE__; return true; case __LINE__:; } while (0)
#define IMP(pc) SUSPEND(implied(s, pc, 1, 1))
#define IMM(pc, bytes) SUSPEND(implied(s, pc, bytes, 0))
#define MODE(pc) SUSPEND(implied(s, pc, 2, 1))
#define JUMP(pc) SUSPEND(implied(s, pc, 3, 0))
#define BRANCH(pc, condition) do { B.branch = (condition); SUSPEND(implied(s, pc, 2, B.branch ? 1u : 0u)); } while (0)
#define READ(pc, bytes, address, width, indexed) SUSPEND(memory(s, pc, bytes, address, width, indexed, false, 0))
#define WRITE(pc, bytes, address, width, indexed, value) SUSPEND(memory(s, pc, bytes, address, width, indexed, true, value))
#define INDIRECT(pc, dp, width) SUSPEND(indirect_read(s, pc, dp, width))
#define LONG_INDIRECT(pc, dp, width, write, value) SUSPEND(long_indirect(s, pc, dp, width, write, value))
#define CHANGE(pc, bytes, address, width, indexed, kind) do { SUSPEND(rmw(s, pc, bytes, address, width, indexed, kind)); rmw_commit(s); } while (0)
#define PUSH(pc, bytes, value, width) do { SUSPEND(push(s, pc, bytes, value, width)); B.registers.stack_pointer -= (width); } while (0)
#define PULL(pc, width, is_rts) do { SUSPEND(pull(s, pc, width, is_rts)); B.registers.stack_pointer += (width); } while (0)

/* Preserve source-visible behavior: $8AD2 publishes separate fixed low/high
 * fill passes, $8D02..$8D0E carries the source shift/ADC flags into row setup,
 * and $ECC7/$EDF9 clamps selectors >=35 to zero. $EA4B transforms each map
 * word in place; neither its 416-word native loop nor the row DMAs collapse
 * into an empirical delay. The generated program retains each source step. */
#include "nba_setup_producer_program.inc"

static bool prepare(NbaSetupProducerWork *s)
{
    if (!advance(s)) return false;
    if (s->child_active) return true;
    if (B.instructions >= B.instruction_limit) {
        B.status = NBA_CODEC_WORK_LIMIT;
        B.pending_count = 0;
        return false;
    }
    ++B.instructions;
    B.pending[B.pending_count - 1u].cycle.instruction_end = true;
    return true;
}

bool nba_setup_producer_work_begin(NbaSetupProducerWork *s, const NbaCodecWorkEntry *entry,
                               uint64_t instruction_limit)
{
    if (s == NULL || entry == NULL || instruction_limit == 0 ||
        (entry->status & 0x38u) != 0 || (entry->data_bank & 0x40u) != 0) return false;
    memset(s, 0, sizeof(*s));
    B.registers = *entry;
    B.instruction_limit = instruction_limit;
    B.status = NBA_CODEC_WORK_RUNNING;
    return prepare(s);
}

bool nba_setup_producer_work_peek(const NbaSetupProducerWork *s, NbaCodecBusCycle *out)
{
    const NbaCodecPendingCycle *p;
    if (s == NULL || out == NULL || B.status != NBA_CODEC_WORK_RUNNING) return false;
    if (s->child_active) return s->codec_kind != 46 ?
        nba_setup_fb30_work_peek(&s->child.fb30, out) : nba_setup_codec_work_peek(&s->child.fb46, out);
    if (B.pending_index >= B.pending_count) return false;
    p = &B.pending[B.pending_index];
    *out = p->cycle;
    if (L.indirect_cycle[B.pending_index]) {
        out->address = L.indirect_cycle[B.pending_index] == 2 ?
            ((((uint32_t)s->pointer_bank << 16) | L.pointer_read) + out->address) & 0xffffffu :
            DB(L.pointer_read + out->address);
        out->master_clocks = bus_clocks(out->address);
    }
    if (p->rmw_value)
        out->value = (uint8_t)(changed(B.read_value, L.rmw_width, (ProducerChange)L.change_kind, L.carry_in) >> p->read_shift);
    return true;
}

bool nba_setup_producer_work_accept(NbaSetupProducerWork *s, uint8_t value)
{
    const NbaCodecPendingCycle *p;
    if (s == NULL || B.status != NBA_CODEC_WORK_RUNNING) return false;
    if (s->child_active) {
        NbaSetupCodecWork *child;
        bool accepted;
        if (s->codec_kind==0) {
            bool a=nba_setup_fb30_work_accept(&s->child.fb30,value);
            bool b=nba_setup_codec_work_accept(&s->child.fb46,value);
            accepted=a && b;
            if (!select_codec(s)) return false;
        } else accepted=s->codec_kind==30 ? nba_setup_fb30_work_accept(&s->child.fb30,value) :
                                           nba_setup_codec_work_accept(&s->child.fb46,value);
        if (!accepted) return false;
        child=s->codec_kind!=46 ? &s->child.fb30.bus : &s->child.fb46;
        if (child->status!=NBA_CODEC_WORK_RUNNING) {
            B.instructions+=child->instructions;
            B.registers=child->registers;
            s->child_active=false;
            if (child->status==NBA_CODEC_WORK_DONE) (void)prepare(s);
            else B.status=child->status;
        }
        return true;
    }
    if (B.pending_index >= B.pending_count) return false;

    p = &B.pending[B.pending_index++];
    if (p->cycle.kind == NBA_CODEC_READ) {
        if (p->read_shift == 48) s->pointer_bank = value;
        else if (p->read_shift >= 32) L.pointer_read |= (uint16_t)((uint16_t)value << (p->read_shift - 32));
        else if (p->read_shift == 16) L.read_bank = value;
        else if (p->read_shift >= 0) B.read_value |= (uint16_t)((uint16_t)value << p->read_shift);
    }
    if (B.pending_index == B.pending_count) (void)prepare(s);
    return true;
}
