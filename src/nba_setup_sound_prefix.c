#include "nba_setup_sound_prefix.h"

#include <string.h>

#define B s->bus
#define A B.registers.value
#define X B.registers.symbol
#define Y B.registers.stream_cursor
#define P B.registers.status
#define DB(address) (((uint32_t)B.registers.data_bank << 16) | (uint16_t)(address))

typedef enum { SOUND_INC, SOUND_DEC, SOUND_ASL, SOUND_LSR, SOUND_ROL } SoundChange;

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

static void cycle(NbaSetupSoundPrefix *s, NbaCodecBusKind kind, uint32_t address,
                  uint8_t value, int8_t read_shift, bool changed_value)
{
    NbaCodecPendingCycle *p = &B.pending[B.pending_count];
    ++B.pending_count;
    p->cycle.source_pc = B.pending[0].cycle.source_pc;
    p->cycle.address = address;
    p->cycle.value = value;
    p->cycle.kind = kind;
    p->cycle.master_clocks = kind == NBA_CODEC_IDLE ? 6u : bus_clocks(address);
    p->cycle.instruction_end = false;
    p->read_shift = read_shift;
    p->rmw_value = changed_value;
}

static void idle(NbaSetupSoundPrefix *s)
{
    cycle(s, NBA_CODEC_IDLE, 0, 0, -1, false);
}

static void instruction(NbaSetupSoundPrefix *s, uint16_t pc, uint8_t bytes)
{
    uint8_t i;
    B.pending_count = B.pending_index = 0;
    B.read_value = 0;
    s->read_bank = 0;
    B.pending[0].cycle.source_pc = 0x800000u | pc;
    for (i = 0; i < bytes; ++i)
        cycle(s, NBA_CODEC_READ, 0x800000u | (uint16_t)(pc + i), 0, -1, false);
}

static void implied(NbaSetupSoundPrefix *s, uint16_t pc, uint8_t bytes, uint8_t idles)
{
    instruction(s, pc, bytes);
    while (idles-- != 0) idle(s);
}


static bool index_idle(const NbaSetupSoundPrefix *s, uint16_t base, uint16_t index)
{
    return (P & 0x10u) == 0 || (base & 255u) + index > 255u;
}

static void memory(NbaSetupSoundPrefix *s, uint16_t pc, uint8_t bytes,
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


static uint16_t changed(uint16_t old, uint8_t width, SoundChange kind, uint8_t carry)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    switch (kind) {
    case SOUND_INC: return (uint16_t)((old + 1u) & mask);
    case SOUND_DEC: return (uint16_t)((old - 1u) & mask);
    case SOUND_ASL: return (uint16_t)((old << 1) & mask);
    case SOUND_LSR: return (uint16_t)((old & mask) >> 1);
    case SOUND_ROL: return (uint16_t)(((old << 1) | carry) & mask);
    }
    return 0;
}

static void nz(NbaSetupSoundPrefix *s, uint16_t value, uint8_t width)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    uint16_t sign = width == 1 ? 128u : 32768u;
    P = (uint8_t)((P & 0x7du) | ((value & mask) == 0 ? 2u : 0u) |
                  ((value & sign) != 0 ? 128u : 0u));
}

static void change_flags(NbaSetupSoundPrefix *s, uint16_t old, uint16_t value,
                         uint8_t width, SoundChange kind)
{
    if (kind == SOUND_ASL || kind == SOUND_ROL)
        P = (uint8_t)((P & 0xfeu) | ((old & (width == 1 ? 128u : 32768u)) != 0 ? 1u : 0u));
    else if (kind == SOUND_LSR) P = (uint8_t)((P & 0xfeu) | (old & 1u));
    nz(s, value, width);
}



static void rmw(NbaSetupSoundPrefix *s, uint16_t pc, uint8_t bytes, uint32_t address,
                uint8_t width, bool indexed, SoundChange kind)
{
    memory(s, pc, bytes, address, width, indexed, false, 0);
    idle(s);
    /* Native word RMW writes high byte first, then low byte. This order is
     * observable; ordinary 16-bit stores above write low byte first. */
    if (width == 2) cycle(s, NBA_CODEC_WRITE, (address + 1u) & 0xffffffu, 0, 8, true);
    cycle(s, NBA_CODEC_WRITE, address, 0, 0, true);
    s->rmw_width = width;
    s->change_kind = (uint8_t)kind;
    s->carry_in = P & 1u;
}

static void rmw_commit(NbaSetupSoundPrefix *s)
{
    uint16_t result = changed(B.read_value, s->rmw_width, (SoundChange)s->change_kind, s->carry_in);
    change_flags(s, B.read_value, result, s->rmw_width, (SoundChange)s->change_kind);
}

static void set_a(NbaSetupSoundPrefix *s, uint32_t value)
{
    uint8_t width = (P & 0x20u) != 0 ? 1u : 2u;
    A = width == 1 ? (uint16_t)((A & 0xff00u) | (value & 255u)) : (uint16_t)value;
    nz(s, A, width);
}

static void set_x(NbaSetupSoundPrefix *s, uint32_t value)
{
    uint8_t width = (P & 0x10u) != 0 ? 1u : 2u;
    X = (uint16_t)(width == 1 ? value & 255u : value);
    nz(s, X, width);
}

static void set_y(NbaSetupSoundPrefix *s, uint32_t value)
{
    uint8_t width = (P & 0x10u) != 0 ? 1u : 2u;
    Y = (uint16_t)(width == 1 ? value & 255u : value);
    nz(s, Y, width);
}

static void status_set(NbaSetupSoundPrefix *s, uint32_t value)
{
    P = (uint8_t)value;
    if ((P & 0x10u) != 0) { X &= 255u; Y &= 255u; }
}

static void compare(NbaSetupSoundPrefix *s, uint16_t value, uint16_t operand, uint8_t width)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    value &= mask; operand &= mask;
    nz(s, (uint16_t)(value - operand), width);
    P = (uint8_t)((P & 0xfeu) | (value >= operand ? 1u : 0u));
}

static void push(NbaSetupSoundPrefix *s, uint16_t pc, uint8_t bytes, uint16_t value, uint8_t width)
{
    uint8_t i;
    instruction(s, pc, bytes);
    idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - i),
              (uint8_t)(value >> ((width - i - 1u) * 8u)), -1, false);
}

static void pull(NbaSetupSoundPrefix *s, uint16_t pc, uint8_t width, bool rts)
{
    uint8_t i;
    instruction(s, pc, 1);
    idle(s); idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_READ, (uint16_t)(B.registers.stack_pointer + i + 1u),
              0, (int8_t)(i * 8u), false);
    if (rts) idle(s);
}

static void jsl_call(NbaSetupSoundPrefix *s, uint16_t pc)
{
    instruction(s, pc, 3);
    cycle(s, NBA_CODEC_WRITE, B.registers.stack_pointer, 0x80, -1, false);
    idle(s);
    cycle(s, NBA_CODEC_READ, 0x800000u | (uint16_t)(pc + 3u), 0, -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - 1u), (uint8_t)((pc + 3u) >> 8), -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - 2u), (uint8_t)(pc + 3u), -1, false);
}




static bool boundary(NbaSetupSoundPrefix *s, uint32_t pc, NbaSoundPrefixStop stop)
{
    s->stop = stop; s->boundary_pc = pc;
    B.status = NBA_CODEC_WORK_UNSUPPORTED;
    return false;
}

static bool unsupported(NbaSetupSoundPrefix *s)
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
#define CHANGE(pc, bytes, address, width, indexed, kind) do { SUSPEND(rmw(s, pc, bytes, address, width, indexed, kind)); rmw_commit(s); } while (0)
#define PUSH(pc, bytes, value, width) do { SUSPEND(push(s, pc, bytes, value, width)); B.registers.stack_pointer -= (width); } while (0)
#define PULL(pc, width, is_rts) do { SUSPEND(pull(s, pc, width, is_rts)); B.registers.stack_pointer += (width); } while (0)

/* $A137 enters M=0/X=1, then SEP truncates both index registers. $A1B2
 * deliberately leaves DB loaded from live $5A (normally $82), so the pending
 * SPC read is on that bank's mirror. Preserve $A29F word-RMW high-first writes.
 * $AAE6 is an idle-port poll, not proof of acknowledgement of command $0B;
 * no SPC response or elapsed service time is implemented by this prefix. */
#include "nba_setup_sound_prefix_program.inc"

static bool prepare(NbaSetupSoundPrefix *s)
{
    if (!advance(s)) return false;
    if (B.instructions >= B.instruction_limit) {
        B.status = NBA_CODEC_WORK_LIMIT;
        B.pending_count = 0;
        return false;
    }
    ++B.instructions;
    B.pending[B.pending_count - 1u].cycle.instruction_end = true;
    return true;
}

bool nba_setup_sound_prefix_begin(NbaSetupSoundPrefix *s, const NbaCodecWorkEntry *entry,
                               uint64_t instruction_limit)
{
    if (s == NULL || entry == NULL || instruction_limit == 0 ||
        (entry->status & 0x38u) != 0x10u || entry->data_bank != 0x80u ||
        entry->symbol > 255u || entry->stream_cursor > 255u) return false;
    memset(s, 0, sizeof(*s));
    B.registers = *entry;
    B.instruction_limit = instruction_limit;
    B.status = NBA_CODEC_WORK_RUNNING;
    return prepare(s);
}

bool nba_setup_sound_prefix_peek(const NbaSetupSoundPrefix *s, NbaCodecBusCycle *out)
{
    const NbaCodecPendingCycle *p;
    if (s == NULL || out == NULL || B.status != NBA_CODEC_WORK_RUNNING) return false;
    if (B.pending_index >= B.pending_count) return false;
    p = &B.pending[B.pending_index];
    *out = p->cycle;
    if (p->rmw_value) out->value = (uint8_t)(changed(B.read_value, s->rmw_width, (SoundChange)s->change_kind, s->carry_in) >> p->read_shift);
    return true;
}

bool nba_setup_sound_prefix_accept(NbaSetupSoundPrefix *s, uint8_t value)
{
    const NbaCodecPendingCycle *p;
    if (s == NULL || B.status != NBA_CODEC_WORK_RUNNING) return false;
    if (B.pending_index >= B.pending_count || s->stop != NBA_SOUND_PREFIX_NONE) return false;

    p = &B.pending[B.pending_index++];
    if (p->cycle.kind == NBA_CODEC_READ) {
        if (p->read_shift == 16) s->read_bank = value;
        else if (p->read_shift >= 0) B.read_value |= (uint16_t)((uint16_t)value << p->read_shift);
    }
    if (B.pending_index == B.pending_count) (void)prepare(s);
    if (B.pending_index < B.pending_count && B.pending[B.pending_index].cycle.kind == NBA_CODEC_READ &&
        ((B.pending[B.pending_index].cycle.address >> 16) & 0x40u) == 0 &&
        (B.pending[B.pending_index].cycle.address & 65535u) == 0x2140u) {
        s->stop = NBA_SOUND_PREFIX_SPC_RESPONSE; s->boundary_pc = B.pending[B.pending_index].cycle.source_pc;
    }
    return true;
}
