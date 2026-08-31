#include "nba_bootstrap_fill.h"

#include <string.h>

#define B s->work
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

static void cycle(NbaBootstrapCpu *s, NbaCodecBusKind kind, uint32_t address,
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

static void idle(NbaBootstrapCpu *s)
{
    cycle(s, NBA_CODEC_IDLE, 0, 0, -1, false);
}

static void instruction(NbaBootstrapCpu *s, uint16_t pc, uint8_t bytes)
{
    uint8_t i;
    B.pending_count = B.pending_index = 0;
    B.read_value = 0;
    s->read_bank = 0; s->pointer=0; s->pointer_bank=0; memset(s->indirect,0,sizeof(s->indirect));
    B.pending[0].cycle.source_pc = ((uint32_t)s->program_bank << 16) | pc;
    for (i = 0; i < bytes; ++i)
        cycle(s, NBA_CODEC_READ, ((uint32_t)s->program_bank << 16) | (uint16_t)(pc + i), 0, -1, false);
}

static void implied(NbaBootstrapCpu *s, uint16_t pc, uint8_t bytes, uint8_t idles)
{
    instruction(s, pc, bytes);
    while (idles-- != 0) idle(s);
}


static bool index_idle(const NbaBootstrapCpu *s, uint16_t base, uint16_t index)
{
    return (P & 0x10u) == 0 || (base & 255u) + index > 255u;
}

static void memory(NbaBootstrapCpu *s, uint16_t pc, uint8_t bytes,
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

static void nz(NbaBootstrapCpu *s, uint16_t value, uint8_t width)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    uint16_t sign = width == 1 ? 128u : 32768u;
    P = (uint8_t)((P & 0x7du) | ((value & mask) == 0 ? 2u : 0u) |
                  ((value & sign) != 0 ? 128u : 0u));
}

static void change_flags(NbaBootstrapCpu *s, uint16_t old, uint16_t value,
                         uint8_t width, SoundChange kind)
{
    if (kind == SOUND_ASL || kind == SOUND_ROL)
        P = (uint8_t)((P & 0xfeu) | ((old & (width == 1 ? 128u : 32768u)) != 0 ? 1u : 0u));
    else if (kind == SOUND_LSR) P = (uint8_t)((P & 0xfeu) | (old & 1u));
    nz(s, value, width);
}



static void set_a(NbaBootstrapCpu *s, uint32_t value)
{
    uint8_t width = (P & 0x20u) != 0 ? 1u : 2u;
    A = width == 1 ? (uint16_t)((A & 0xff00u) | (value & 255u)) : (uint16_t)value;
    nz(s, A, width);
}

static void set_x(NbaBootstrapCpu *s, uint32_t value)
{
    uint8_t width = (P & 0x10u) != 0 ? 1u : 2u;
    X = (uint16_t)(width == 1 ? value & 255u : value);
    nz(s, X, width);
}

static void set_y(NbaBootstrapCpu *s, uint32_t value)
{
    uint8_t width = (P & 0x10u) != 0 ? 1u : 2u;
    Y = (uint16_t)(width == 1 ? value & 255u : value);
    nz(s, Y, width);
}

static void status_set(NbaBootstrapCpu *s, uint32_t value)
{
    P = (uint8_t)value;
    if ((P & 0x10u) != 0) { X &= 255u; Y &= 255u; }
}

static void accumulator_change(NbaBootstrapCpu *s, SoundChange kind)
{
    uint8_t width = (P & 0x20u) != 0 ? 1u : 2u;
    uint16_t old = A;
    uint16_t value = changed(old, width, kind, P & 1u);
    set_a(s, value);
    change_flags(s, old, value, width, kind);
}

static void compare(NbaBootstrapCpu *s, uint16_t value, uint16_t operand, uint8_t width)
{
    uint16_t mask = width == 1 ? 255u : 65535u;
    value &= mask; operand &= mask;
    nz(s, (uint16_t)(value - operand), width);
    P = (uint8_t)((P & 0xfeu) | (value >= operand ? 1u : 0u));
}

static void push(NbaBootstrapCpu *s, uint16_t pc, uint8_t bytes, uint16_t value, uint8_t width)
{
    uint8_t i;
    instruction(s, pc, bytes);
    idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - i),
              (uint8_t)(value >> ((width - i - 1u) * 8u)), -1, false);
}

static void pull(NbaBootstrapCpu *s, uint16_t pc, uint8_t width, bool rts)
{
    uint8_t i;
    instruction(s, pc, 1);
    idle(s); idle(s);
    for (i = 0; i < width; ++i)
        cycle(s, NBA_CODEC_READ, (uint16_t)(B.registers.stack_pointer + i + 1u),
              0, (int8_t)(i * 8u), false);
    if (rts) idle(s);
}

static void jsl_call(NbaBootstrapCpu *s, uint16_t pc)
{
    instruction(s, pc, 3);
    cycle(s, NBA_CODEC_WRITE, B.registers.stack_pointer, 0x80, -1, false);
    idle(s);
    cycle(s, NBA_CODEC_READ, 0x800000u | (uint16_t)(pc + 3u), 0, -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - 1u), (uint8_t)((pc + 3u) >> 8), -1, false);
    cycle(s, NBA_CODEC_WRITE, (uint16_t)(B.registers.stack_pointer - 2u), (uint8_t)(pc + 3u), -1, false);
}




static void add(NbaBootstrapCpu *s,uint16_t value) {
    unsigned mask=(P&32)?255u:65535u,sign=(P&32)?128u:32768u;
    unsigned a=A&mask,b=value&mask,r=a+b+(P&1u);
    P=(uint8_t)((P&~0x41u)|(r>mask?1:0)|((~(a^b)&(a^r)&sign)?64:0));set_a(s,r);
}
static void long_read(NbaBootstrapCpu *s,uint16_t pc,uint8_t dp,uint8_t width) {
    unsigned i;instruction(s,pc,2);
    for(i=0;i<3;i++)cycle(s,NBA_CODEC_READ,(uint16_t)(dp+i),0,(int8_t)(32+8*i),false);
    for(i=0;i<width;i++){cycle(s,NBA_CODEC_READ,(uint16_t)(Y+i),0,(int8_t)(8*i),false);s->indirect[B.pending_count-1]=1;}
}
static bool boundary(NbaBootstrapCpu *s,uint32_t pc) {s->boundary_pc=pc;B.status=NBA_CODEC_WORK_UNSUPPORTED;return false;}
#define SUSPEND(recipe) do { recipe; B.resume = __LINE__; return true; case __LINE__:; } while (0)
#define IMP(pc) SUSPEND(implied(s,pc,1,1))
#define IMM(pc,n) SUSPEND(implied(s,pc,n,0))
#define MODE(pc) SUSPEND(implied(s,pc,2,1))
#define BRANCH(pc,test) do { B.branch=(test); SUSPEND(implied(s,pc,2,B.branch?1u:0u)); } while(0)
#define READ(pc,n,a,w,idx) SUSPEND(memory(s,pc,n,a,w,idx,false,0))
#define WRITE(pc,n,a,w,idx,v) SUSPEND(memory(s,pc,n,a,w,idx,true,v))
#define LONG_READ(pc,dp,w) SUSPEND(long_read(s,pc,dp,w))
#define PUSH(pc,n,v,w) do { SUSPEND(push(s,pc,n,v,w));B.registers.stack_pointer-=(w); } while(0)
#define PULL(pc,w,rts) do { SUSPEND(pull(s,pc,w,rts));B.registers.stack_pointer+=(w); } while(0)
#include "nba_bootstrap_fill_cpu_program.inc"
static bool prepare(NbaBootstrapCpu *s) {
    if(!advance(s))return false;
    B.pending[B.pending_count-1].cycle.instruction_end=true;return true;
}
bool nba_bootstrap_fill_cpu_power_on(NbaBootstrapCpu *s) {
    if(!s)return false;memset(s,0,sizeof(*s));
    /* Pinned SnesCpu::PowerOn, canonical reset vector 00800D. */
    B.registers.stack_pointer=0x1ff;B.registers.status=0x34;s->emulation=true;
    B.status=NBA_CODEC_WORK_RUNNING;return prepare(s);
}
bool nba_bootstrap_fill_cpu_peek(const NbaBootstrapCpu *s,NbaCodecBusCycle *out) {
    if(!s||!out||B.status!=NBA_CODEC_WORK_RUNNING||B.pending_index>=B.pending_count)return false;
    *out=B.pending[B.pending_index].cycle;
    if(s->indirect[B.pending_index])out->address=((((uint32_t)s->pointer_bank<<16)|s->pointer)+out->address)&0xffffffu;
    return true;
}
bool nba_bootstrap_fill_cpu_accept(NbaBootstrapCpu *s,uint8_t value) {
    const NbaCodecPendingCycle *p;
    if(!s||B.status!=NBA_CODEC_WORK_RUNNING||B.pending_index>=B.pending_count)return false;
    p=&B.pending[B.pending_index++];
    if(p->cycle.kind==NBA_CODEC_READ){
        if(p->read_shift==48)s->pointer_bank=value;
        else if(p->read_shift>=32)s->pointer|=(uint16_t)((uint16_t)value<<(p->read_shift-32));
        else if(p->read_shift==16)s->read_bank=value;
        else if(p->read_shift>=0)B.read_value|=(uint16_t)((uint16_t)value<<p->read_shift);
    }
    if(B.pending_index==B.pending_count){B.instructions++;(void)prepare(s);}return true;
}
