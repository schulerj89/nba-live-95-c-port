#include "nba_spc.h"
#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------------
 * SPC700 CPU
 * ------------------------------------------------------------------------ */

#define F_N 0x80
#define F_V 0x40
#define F_P 0x20
#define F_B 0x10
#define F_H 0x08
#define F_I 0x04
#define F_Z 0x02
#define F_C 0x01

static void dsp_write(NbaSpc *s, uint8_t addr, uint8_t value);
unsigned nba_spc_dbg_timer_reads = 0, nba_spc_dbg_dsp_writes = 0, nba_spc_dbg_timer_ticks = 0;

static uint8_t spc_read(NbaSpc *s, uint16_t addr) {
    if (addr >= 0x00F0 && addr <= 0x00FF) {
        switch (addr) {
            case 0x00F2: return s->dsp_addr;
            case 0x00F3: return s->dsp[s->dsp_addr & 0x7F];
            case 0x00FD: case 0x00FE: case 0x00FF: {
                int t = addr - 0x00FD;
                nba_spc_dbg_timer_reads++;
                uint8_t v = s->timer_out[t] & 0x0F;
                s->timer_out[t] = 0;
                return v;
            }
            default: return s->ram[addr];
        }
    }
    return s->ram[addr];
}

static void spc_write(NbaSpc *s, uint16_t addr, uint8_t value) {
    if (addr >= 0x00F0 && addr <= 0x00FF) {
        switch (addr) {
            case 0x00F1:
                for (int t = 0; t < 3; ++t) {
                    bool en = (value >> t) & 1;
                    if (en && !s->timer_enabled[t]) {
                        s->timer_counter[t] = 0;
                        s->timer_out[t] = 0;
                    }
                    s->timer_enabled[t] = en;
                }
                s->ipl_enabled = (value & 0x80) != 0;
                return;
            case 0x00F2: s->dsp_addr = value; return;
            case 0x00F3: nba_spc_dbg_dsp_writes++; dsp_write(s, s->dsp_addr, value); return;
            /* $F4-$F7 are the CPU-facing output latches. What the SPC reads
             * back is whatever the 65816 last wrote, so writes here must not
             * change the read value. With no 65816 driving the ports they stay
             * at 0, which the driver treats as "no command pending". */
            case 0x00F4: case 0x00F5: case 0x00F6: case 0x00F7: return;
            case 0x00FA: s->timer_target[0] = value; return;
            case 0x00FB: s->timer_target[1] = value; return;
            case 0x00FC: s->timer_target[2] = value; return;
            default: s->ram[addr] = value; return;
        }
    }
    s->ram[addr] = value;
}

static uint8_t fetch(NbaSpc *s) { return spc_read(s, s->pc++); }

static uint16_t fetch16(NbaSpc *s) {
    uint16_t lo = fetch(s);
    return (uint16_t)(lo | ((uint16_t)fetch(s) << 8));
}

static uint16_t dpaddr(NbaSpc *s, uint8_t off) {
    return (uint16_t)(((s->psw & F_P) ? 0x0100 : 0x0000) + off);
}

static void push(NbaSpc *s, uint8_t v) { s->ram[0x0100 + s->sp--] = v; }
static uint8_t pop(NbaSpc *s) { return s->ram[0x0100 + ++s->sp]; }

static void set_nz(NbaSpc *s, uint8_t v) {
    s->psw = (uint8_t)((s->psw & ~(F_N | F_Z)) | (v & 0x80) | (v == 0 ? F_Z : 0));
}

static void set_nz16(NbaSpc *s, uint16_t v) {
    s->psw = (uint8_t)((s->psw & ~(F_N | F_Z)) | ((v >> 8) & 0x80) | (v == 0 ? F_Z : 0));
}

static uint8_t op_adc(NbaSpc *s, uint8_t a, uint8_t b) {
    int c = (s->psw & F_C) ? 1 : 0;
    int r = a + b + c;
    s->psw &= (uint8_t)~(F_N | F_V | F_H | F_Z | F_C);
    if (((a ^ r) & (b ^ r) & 0x80) != 0) s->psw |= F_V;
    if (((a & 0x0F) + (b & 0x0F) + c) > 0x0F) s->psw |= F_H;
    if (r > 0xFF) s->psw |= F_C;
    uint8_t v = (uint8_t)r;
    s->psw |= (uint8_t)((v & 0x80) | (v == 0 ? F_Z : 0));
    return v;
}

static uint8_t op_sbc(NbaSpc *s, uint8_t a, uint8_t b) {
    return op_adc(s, a, (uint8_t)~b);
}

static void op_cmp(NbaSpc *s, uint8_t a, uint8_t b) {
    int r = a - b;
    s->psw &= (uint8_t)~(F_N | F_Z | F_C);
    if (r >= 0) s->psw |= F_C;
    uint8_t v = (uint8_t)r;
    s->psw |= (uint8_t)((v & 0x80) | (v == 0 ? F_Z : 0));
}

static uint8_t op_asl(NbaSpc *s, uint8_t v) {
    s->psw = (uint8_t)((s->psw & ~F_C) | ((v & 0x80) ? F_C : 0));
    v = (uint8_t)(v << 1); set_nz(s, v); return v;
}
static uint8_t op_lsr(NbaSpc *s, uint8_t v) {
    s->psw = (uint8_t)((s->psw & ~F_C) | (v & 1));
    v = (uint8_t)(v >> 1); set_nz(s, v); return v;
}
static uint8_t op_rol(NbaSpc *s, uint8_t v) {
    int c = (s->psw & F_C) ? 1 : 0;
    s->psw = (uint8_t)((s->psw & ~F_C) | ((v & 0x80) ? F_C : 0));
    v = (uint8_t)((v << 1) | c); set_nz(s, v); return v;
}
static uint8_t op_ror(NbaSpc *s, uint8_t v) {
    int c = (s->psw & F_C) ? 0x80 : 0;
    s->psw = (uint8_t)((s->psw & ~F_C) | (v & 1));
    v = (uint8_t)((v >> 1) | c); set_nz(s, v); return v;
}
static uint8_t op_inc(NbaSpc *s, uint8_t v) { v++; set_nz(s, v); return v; }
static uint8_t op_dec(NbaSpc *s, uint8_t v) { v--; set_nz(s, v); return v; }

/* cycle counts per opcode (Anomie's SPC700 timing table) */
static const uint8_t spc_cycles[256] = {
    2,8,4,5,3,4,3,6,2,6,5,4,5,4,6,8, 2,8,4,5,4,5,5,6,5,5,6,5,2,2,4,6,
    2,8,4,5,3,4,3,6,2,6,5,4,5,4,5,4, 2,8,4,5,4,5,5,6,5,5,6,5,2,2,3,8,
    2,8,4,5,3,4,3,6,2,6,4,4,5,4,6,6, 2,8,4,5,4,5,5,6,5,5,4,5,2,2,4,3,
    2,8,4,5,3,4,3,6,2,6,4,4,5,4,5,5, 2,8,4,5,4,5,5,6,5,5,5,5,2,2,3,6,
    2,8,4,5,3,4,3,6,2,6,5,4,5,2,4,5, 2,8,4,5,4,5,5,6,5,5,5,5,2,2,12,5,
    3,8,4,5,3,4,3,6,2,6,4,4,5,2,4,4, 2,8,4,5,4,5,5,6,5,5,5,5,2,2,3,4,
    3,8,4,5,4,5,4,7,2,5,6,4,5,2,4,9, 2,8,4,5,5,6,6,7,4,5,5,5,2,2,6,3,
    2,8,4,5,3,4,3,6,2,4,5,4,5,2,4,3, 2,8,4,5,4,5,5,6,3,4,5,4,2,2,4,3
};

/* Execute one instruction, return cycles consumed. */
static int spc_step(NbaSpc *s) {
    uint8_t op = fetch(s);
    int cycles = spc_cycles[op];
    uint8_t t8; uint16_t t16, addr;

    switch (op) {
        /* ---- MOV A ---- */
        case 0xE8: s->a = fetch(s); set_nz(s, s->a); break;                        /* MOV A,#imm */
        case 0xE6: s->a = spc_read(s, (uint16_t)(s->x + ((s->psw & F_P) ? 0x100 : 0))); set_nz(s, s->a); break; /* MOV A,(X) */
        case 0xBF: s->a = spc_read(s, dpaddr(s, s->x)); s->x++; set_nz(s, s->a); break; /* MOV A,(X)+ */
        case 0xE4: s->a = spc_read(s, dpaddr(s, fetch(s))); set_nz(s, s->a); break; /* MOV A,dp */
        case 0xF4: s->a = spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x))); set_nz(s, s->a); break;
        case 0xE5: s->a = spc_read(s, fetch16(s)); set_nz(s, s->a); break;
        case 0xF5: s->a = spc_read(s, (uint16_t)(fetch16(s) + s->x)); set_nz(s, s->a); break;
        case 0xF6: s->a = spc_read(s, (uint16_t)(fetch16(s) + s->y)); set_nz(s, s->a); break;
        case 0xE7: t8 = (uint8_t)(fetch(s) + s->x); addr = dpaddr(s, t8);
                   t16 = (uint16_t)(spc_read(s, addr) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a = spc_read(s, t16); set_nz(s, s->a); break;
        case 0xF7: t8 = fetch(s); addr = dpaddr(s, t8);
                   t16 = (uint16_t)(spc_read(s, addr) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a = spc_read(s, (uint16_t)(t16 + s->y)); set_nz(s, s->a); break;

        case 0xCD: s->x = fetch(s); set_nz(s, s->x); break;
        case 0xF8: s->x = spc_read(s, dpaddr(s, fetch(s))); set_nz(s, s->x); break;
        case 0xF9: s->x = spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->y))); set_nz(s, s->x); break;
        case 0xE9: s->x = spc_read(s, fetch16(s)); set_nz(s, s->x); break;
        case 0x8D: s->y = fetch(s); set_nz(s, s->y); break;
        case 0xEB: s->y = spc_read(s, dpaddr(s, fetch(s))); set_nz(s, s->y); break;
        case 0xFB: s->y = spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x))); set_nz(s, s->y); break;
        case 0xEC: s->y = spc_read(s, fetch16(s)); set_nz(s, s->y); break;

        /* ---- MOV stores ---- */
        case 0xC6: spc_write(s, (uint16_t)(s->x + ((s->psw & F_P) ? 0x100 : 0)), s->a); break;
        case 0xAF: spc_write(s, dpaddr(s, s->x), s->a); s->x++; break;
        case 0xC4: spc_write(s, dpaddr(s, fetch(s)), s->a); break;
        case 0xD4: spc_write(s, dpaddr(s, (uint8_t)(fetch(s) + s->x)), s->a); break;
        case 0xC5: spc_write(s, fetch16(s), s->a); break;
        case 0xD5: spc_write(s, (uint16_t)(fetch16(s) + s->x), s->a); break;
        case 0xD6: spc_write(s, (uint16_t)(fetch16(s) + s->y), s->a); break;
        case 0xC7: t8 = (uint8_t)(fetch(s) + s->x);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   spc_write(s, t16, s->a); break;
        case 0xD7: t8 = fetch(s);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   spc_write(s, (uint16_t)(t16 + s->y), s->a); break;
        case 0xD8: spc_write(s, dpaddr(s, fetch(s)), s->x); break;
        case 0xD9: spc_write(s, dpaddr(s, (uint8_t)(fetch(s) + s->y)), s->x); break;
        case 0xC9: spc_write(s, fetch16(s), s->x); break;
        case 0xCB: spc_write(s, dpaddr(s, fetch(s)), s->y); break;
        case 0xDB: spc_write(s, dpaddr(s, (uint8_t)(fetch(s) + s->x)), s->y); break;
        case 0xCC: spc_write(s, fetch16(s), s->y); break;
        case 0x8F: { uint8_t v = fetch(s); spc_write(s, dpaddr(s, fetch(s)), v); } break; /* MOV dp,#imm */
        case 0xFA: { uint8_t v = spc_read(s, dpaddr(s, fetch(s))); spc_write(s, dpaddr(s, fetch(s)), v); } break;

        /* ---- register moves ---- */
        case 0x7D: s->a = s->x; set_nz(s, s->a); break;
        case 0xDD: s->a = s->y; set_nz(s, s->a); break;
        case 0x5D: s->x = s->a; set_nz(s, s->x); break;
        case 0xFD: s->y = s->a; set_nz(s, s->y); break;
        case 0x9D: s->x = s->sp; set_nz(s, s->x); break;
        case 0xBD: s->sp = s->x; break;

        /* ---- stack ---- */
        case 0x2D: push(s, s->a); break;
        case 0x4D: push(s, s->x); break;
        case 0x6D: push(s, s->y); break;
        case 0x0D: push(s, s->psw); break;
        case 0xAE: s->a = pop(s); break;
        case 0xCE: s->x = pop(s); break;
        case 0xEE: s->y = pop(s); break;
        case 0x8E: s->psw = pop(s); break;

        /* ---- ALU: A op operand ---- */
        case 0x88: s->a = op_adc(s, s->a, fetch(s)); break;
        case 0x86: s->a = op_adc(s, s->a, spc_read(s, dpaddr(s, s->x))); break;
        case 0x84: s->a = op_adc(s, s->a, spc_read(s, dpaddr(s, fetch(s)))); break;
        case 0x94: s->a = op_adc(s, s->a, spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x)))); break;
        case 0x85: s->a = op_adc(s, s->a, spc_read(s, fetch16(s))); break;
        case 0x95: s->a = op_adc(s, s->a, spc_read(s, (uint16_t)(fetch16(s) + s->x))); break;
        case 0x96: s->a = op_adc(s, s->a, spc_read(s, (uint16_t)(fetch16(s) + s->y))); break;
        case 0x87: t8 = (uint8_t)(fetch(s) + s->x);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a = op_adc(s, s->a, spc_read(s, t16)); break;
        case 0x97: t8 = fetch(s);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a = op_adc(s, s->a, spc_read(s, (uint16_t)(t16 + s->y))); break;
        case 0x99: { uint16_t d = dpaddr(s, s->x), src = dpaddr(s, s->y);
                     spc_write(s, d, op_adc(s, spc_read(s, d), spc_read(s, src))); } break;
        case 0x89: { uint8_t src = fetch(s), dst = fetch(s); uint16_t d = dpaddr(s, dst);
                     spc_write(s, d, op_adc(s, spc_read(s, d), spc_read(s, dpaddr(s, src)))); } break;
        case 0x98: { uint8_t imm = fetch(s); uint16_t d = dpaddr(s, fetch(s));
                     spc_write(s, d, op_adc(s, spc_read(s, d), imm)); } break;

        case 0xA8: s->a = op_sbc(s, s->a, fetch(s)); break;
        case 0xA6: s->a = op_sbc(s, s->a, spc_read(s, dpaddr(s, s->x))); break;
        case 0xA4: s->a = op_sbc(s, s->a, spc_read(s, dpaddr(s, fetch(s)))); break;
        case 0xB4: s->a = op_sbc(s, s->a, spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x)))); break;
        case 0xA5: s->a = op_sbc(s, s->a, spc_read(s, fetch16(s))); break;
        case 0xB5: s->a = op_sbc(s, s->a, spc_read(s, (uint16_t)(fetch16(s) + s->x))); break;
        case 0xB6: s->a = op_sbc(s, s->a, spc_read(s, (uint16_t)(fetch16(s) + s->y))); break;
        case 0xA7: t8 = (uint8_t)(fetch(s) + s->x);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a = op_sbc(s, s->a, spc_read(s, t16)); break;
        case 0xB7: t8 = fetch(s);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a = op_sbc(s, s->a, spc_read(s, (uint16_t)(t16 + s->y))); break;
        case 0xB9: { uint16_t d = dpaddr(s, s->x), src = dpaddr(s, s->y);
                     spc_write(s, d, op_sbc(s, spc_read(s, d), spc_read(s, src))); } break;
        case 0xA9: { uint8_t src = fetch(s), dst = fetch(s); uint16_t d = dpaddr(s, dst);
                     spc_write(s, d, op_sbc(s, spc_read(s, d), spc_read(s, dpaddr(s, src)))); } break;
        case 0xB8: { uint8_t imm = fetch(s); uint16_t d = dpaddr(s, fetch(s));
                     spc_write(s, d, op_sbc(s, spc_read(s, d), imm)); } break;

        case 0x68: op_cmp(s, s->a, fetch(s)); break;
        case 0x66: op_cmp(s, s->a, spc_read(s, dpaddr(s, s->x))); break;
        case 0x64: op_cmp(s, s->a, spc_read(s, dpaddr(s, fetch(s)))); break;
        case 0x74: op_cmp(s, s->a, spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x)))); break;
        case 0x65: op_cmp(s, s->a, spc_read(s, fetch16(s))); break;
        case 0x75: op_cmp(s, s->a, spc_read(s, (uint16_t)(fetch16(s) + s->x))); break;
        case 0x76: op_cmp(s, s->a, spc_read(s, (uint16_t)(fetch16(s) + s->y))); break;
        case 0x67: t8 = (uint8_t)(fetch(s) + s->x);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   op_cmp(s, s->a, spc_read(s, t16)); break;
        case 0x77: t8 = fetch(s);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   op_cmp(s, s->a, spc_read(s, (uint16_t)(t16 + s->y))); break;
        case 0x79: op_cmp(s, spc_read(s, dpaddr(s, s->x)), spc_read(s, dpaddr(s, s->y))); break;
        case 0x69: { uint8_t src = fetch(s), dst = fetch(s);
                     op_cmp(s, spc_read(s, dpaddr(s, dst)), spc_read(s, dpaddr(s, src))); } break;
        case 0x78: { uint8_t imm = fetch(s); op_cmp(s, spc_read(s, dpaddr(s, fetch(s))), imm); } break;
        case 0xC8: op_cmp(s, s->x, fetch(s)); break;
        case 0x3E: op_cmp(s, s->x, spc_read(s, dpaddr(s, fetch(s)))); break;
        case 0x1E: op_cmp(s, s->x, spc_read(s, fetch16(s))); break;
        case 0xAD: op_cmp(s, s->y, fetch(s)); break;
        case 0x7E: op_cmp(s, s->y, spc_read(s, dpaddr(s, fetch(s)))); break;
        case 0x5E: op_cmp(s, s->y, spc_read(s, fetch16(s))); break;

        case 0x08: s->a |= fetch(s); set_nz(s, s->a); break;
        case 0x06: s->a |= spc_read(s, dpaddr(s, s->x)); set_nz(s, s->a); break;
        case 0x04: s->a |= spc_read(s, dpaddr(s, fetch(s))); set_nz(s, s->a); break;
        case 0x14: s->a |= spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x))); set_nz(s, s->a); break;
        case 0x05: s->a |= spc_read(s, fetch16(s)); set_nz(s, s->a); break;
        case 0x15: s->a |= spc_read(s, (uint16_t)(fetch16(s) + s->x)); set_nz(s, s->a); break;
        case 0x16: s->a |= spc_read(s, (uint16_t)(fetch16(s) + s->y)); set_nz(s, s->a); break;
        case 0x07: t8 = (uint8_t)(fetch(s) + s->x);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a |= spc_read(s, t16); set_nz(s, s->a); break;
        case 0x17: t8 = fetch(s);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a |= spc_read(s, (uint16_t)(t16 + s->y)); set_nz(s, s->a); break;
        case 0x19: { uint16_t d = dpaddr(s, s->x); uint8_t v = (uint8_t)(spc_read(s, d) | spc_read(s, dpaddr(s, s->y)));
                     set_nz(s, v); spc_write(s, d, v); } break;
        case 0x09: { uint8_t src = fetch(s), dst = fetch(s); uint16_t d = dpaddr(s, dst);
                     uint8_t v = (uint8_t)(spc_read(s, d) | spc_read(s, dpaddr(s, src)));
                     set_nz(s, v); spc_write(s, d, v); } break;
        case 0x18: { uint8_t imm = fetch(s); uint16_t d = dpaddr(s, fetch(s));
                     uint8_t v = (uint8_t)(spc_read(s, d) | imm); set_nz(s, v); spc_write(s, d, v); } break;

        case 0x28: s->a &= fetch(s); set_nz(s, s->a); break;
        case 0x26: s->a &= spc_read(s, dpaddr(s, s->x)); set_nz(s, s->a); break;
        case 0x24: s->a &= spc_read(s, dpaddr(s, fetch(s))); set_nz(s, s->a); break;
        case 0x34: s->a &= spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x))); set_nz(s, s->a); break;
        case 0x25: s->a &= spc_read(s, fetch16(s)); set_nz(s, s->a); break;
        case 0x35: s->a &= spc_read(s, (uint16_t)(fetch16(s) + s->x)); set_nz(s, s->a); break;
        case 0x36: s->a &= spc_read(s, (uint16_t)(fetch16(s) + s->y)); set_nz(s, s->a); break;
        case 0x27: t8 = (uint8_t)(fetch(s) + s->x);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a &= spc_read(s, t16); set_nz(s, s->a); break;
        case 0x37: t8 = fetch(s);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a &= spc_read(s, (uint16_t)(t16 + s->y)); set_nz(s, s->a); break;
        case 0x39: { uint16_t d = dpaddr(s, s->x); uint8_t v = (uint8_t)(spc_read(s, d) & spc_read(s, dpaddr(s, s->y)));
                     set_nz(s, v); spc_write(s, d, v); } break;
        case 0x29: { uint8_t src = fetch(s), dst = fetch(s); uint16_t d = dpaddr(s, dst);
                     uint8_t v = (uint8_t)(spc_read(s, d) & spc_read(s, dpaddr(s, src)));
                     set_nz(s, v); spc_write(s, d, v); } break;
        case 0x38: { uint8_t imm = fetch(s); uint16_t d = dpaddr(s, fetch(s));
                     uint8_t v = (uint8_t)(spc_read(s, d) & imm); set_nz(s, v); spc_write(s, d, v); } break;

        case 0x48: s->a ^= fetch(s); set_nz(s, s->a); break;
        case 0x46: s->a ^= spc_read(s, dpaddr(s, s->x)); set_nz(s, s->a); break;
        case 0x44: s->a ^= spc_read(s, dpaddr(s, fetch(s))); set_nz(s, s->a); break;
        case 0x54: s->a ^= spc_read(s, dpaddr(s, (uint8_t)(fetch(s) + s->x))); set_nz(s, s->a); break;
        case 0x45: s->a ^= spc_read(s, fetch16(s)); set_nz(s, s->a); break;
        case 0x55: s->a ^= spc_read(s, (uint16_t)(fetch16(s) + s->x)); set_nz(s, s->a); break;
        case 0x56: s->a ^= spc_read(s, (uint16_t)(fetch16(s) + s->y)); set_nz(s, s->a); break;
        case 0x47: t8 = (uint8_t)(fetch(s) + s->x);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a ^= spc_read(s, t16); set_nz(s, s->a); break;
        case 0x57: t8 = fetch(s);
                   t16 = (uint16_t)(spc_read(s, dpaddr(s, t8)) | (spc_read(s, dpaddr(s, (uint8_t)(t8 + 1))) << 8));
                   s->a ^= spc_read(s, (uint16_t)(t16 + s->y)); set_nz(s, s->a); break;
        case 0x59: { uint16_t d = dpaddr(s, s->x); uint8_t v = (uint8_t)(spc_read(s, d) ^ spc_read(s, dpaddr(s, s->y)));
                     set_nz(s, v); spc_write(s, d, v); } break;
        case 0x49: { uint8_t src = fetch(s), dst = fetch(s); uint16_t d = dpaddr(s, dst);
                     uint8_t v = (uint8_t)(spc_read(s, d) ^ spc_read(s, dpaddr(s, src)));
                     set_nz(s, v); spc_write(s, d, v); } break;
        case 0x58: { uint8_t imm = fetch(s); uint16_t d = dpaddr(s, fetch(s));
                     uint8_t v = (uint8_t)(spc_read(s, d) ^ imm); set_nz(s, v); spc_write(s, d, v); } break;

        /* ---- inc/dec/shift ---- */
        case 0xBC: s->a = op_inc(s, s->a); break;
        case 0x3D: s->x = op_inc(s, s->x); break;
        case 0xFC: s->y = op_inc(s, s->y); break;
        case 0x9C: s->a = op_dec(s, s->a); break;
        case 0x1D: s->x = op_dec(s, s->x); break;
        case 0xDC: s->y = op_dec(s, s->y); break;
        case 0xAB: { uint16_t d = dpaddr(s, fetch(s)); spc_write(s, d, op_inc(s, spc_read(s, d))); } break;
        case 0xBB: { uint16_t d = dpaddr(s, (uint8_t)(fetch(s) + s->x)); spc_write(s, d, op_inc(s, spc_read(s, d))); } break;
        case 0xAC: { uint16_t d = fetch16(s); spc_write(s, d, op_inc(s, spc_read(s, d))); } break;
        case 0x8B: { uint16_t d = dpaddr(s, fetch(s)); spc_write(s, d, op_dec(s, spc_read(s, d))); } break;
        case 0x9B: { uint16_t d = dpaddr(s, (uint8_t)(fetch(s) + s->x)); spc_write(s, d, op_dec(s, spc_read(s, d))); } break;
        case 0x8C: { uint16_t d = fetch16(s); spc_write(s, d, op_dec(s, spc_read(s, d))); } break;
        case 0x1C: s->a = op_asl(s, s->a); break;
        case 0x0B: { uint16_t d = dpaddr(s, fetch(s)); spc_write(s, d, op_asl(s, spc_read(s, d))); } break;
        case 0x1B: { uint16_t d = dpaddr(s, (uint8_t)(fetch(s) + s->x)); spc_write(s, d, op_asl(s, spc_read(s, d))); } break;
        case 0x0C: { uint16_t d = fetch16(s); spc_write(s, d, op_asl(s, spc_read(s, d))); } break;
        case 0x5C: s->a = op_lsr(s, s->a); break;
        case 0x4B: { uint16_t d = dpaddr(s, fetch(s)); spc_write(s, d, op_lsr(s, spc_read(s, d))); } break;
        case 0x5B: { uint16_t d = dpaddr(s, (uint8_t)(fetch(s) + s->x)); spc_write(s, d, op_lsr(s, spc_read(s, d))); } break;
        case 0x4C: { uint16_t d = fetch16(s); spc_write(s, d, op_lsr(s, spc_read(s, d))); } break;
        case 0x3C: s->a = op_rol(s, s->a); break;
        case 0x2B: { uint16_t d = dpaddr(s, fetch(s)); spc_write(s, d, op_rol(s, spc_read(s, d))); } break;
        case 0x3B: { uint16_t d = dpaddr(s, (uint8_t)(fetch(s) + s->x)); spc_write(s, d, op_rol(s, spc_read(s, d))); } break;
        case 0x2C: { uint16_t d = fetch16(s); spc_write(s, d, op_rol(s, spc_read(s, d))); } break;
        case 0x7C: s->a = op_ror(s, s->a); break;
        case 0x6B: { uint16_t d = dpaddr(s, fetch(s)); spc_write(s, d, op_ror(s, spc_read(s, d))); } break;
        case 0x7B: { uint16_t d = dpaddr(s, (uint8_t)(fetch(s) + s->x)); spc_write(s, d, op_ror(s, spc_read(s, d))); } break;
        case 0x6C: { uint16_t d = fetch16(s); spc_write(s, d, op_ror(s, spc_read(s, d))); } break;
        case 0x9F: { uint8_t v = (uint8_t)((s->a >> 4) | (s->a << 4)); s->a = v; set_nz(s, s->a); } break;

        /* ---- 16-bit (YA / word) ---- */
        case 0xBA: { uint8_t d = fetch(s);
                     uint16_t v = (uint16_t)(spc_read(s, dpaddr(s, d)) |
                                  (spc_read(s, dpaddr(s, (uint8_t)(d + 1))) << 8));
                     s->a = (uint8_t)v; s->y = (uint8_t)(v >> 8); set_nz16(s, v); } break;
        case 0xDA: { uint8_t d = fetch(s); spc_write(s, dpaddr(s, d), s->a);
                     spc_write(s, dpaddr(s, (uint8_t)(d + 1)), s->y); } break;
        case 0x3A: { uint8_t d = fetch(s); uint16_t a1 = dpaddr(s, d), a2 = dpaddr(s, (uint8_t)(d + 1));
                     uint16_t v = (uint16_t)((spc_read(s, a1) | (spc_read(s, a2) << 8)) + 1);
                     spc_write(s, a1, (uint8_t)v); spc_write(s, a2, (uint8_t)(v >> 8)); set_nz16(s, v); } break;
        case 0x1A: { uint8_t d = fetch(s); uint16_t a1 = dpaddr(s, d), a2 = dpaddr(s, (uint8_t)(d + 1));
                     uint16_t v = (uint16_t)((spc_read(s, a1) | (spc_read(s, a2) << 8)) - 1);
                     spc_write(s, a1, (uint8_t)v); spc_write(s, a2, (uint8_t)(v >> 8)); set_nz16(s, v); } break;
        case 0x7A: { uint8_t d = fetch(s);
                     uint16_t w = (uint16_t)(spc_read(s, dpaddr(s, d)) |
                                  (spc_read(s, dpaddr(s, (uint8_t)(d + 1))) << 8));
                     uint16_t ya = (uint16_t)(s->a | (s->y << 8));
                     uint32_t r = (uint32_t)ya + w;
                     s->psw &= (uint8_t)~(F_N | F_V | F_H | F_Z | F_C);
                     if (((ya ^ r) & (w ^ r) & 0x8000) != 0) s->psw |= F_V;
                     if (((ya & 0x0FFF) + (w & 0x0FFF)) > 0x0FFF) s->psw |= F_H;
                     if (r > 0xFFFF) s->psw |= F_C;
                     s->a = (uint8_t)r; s->y = (uint8_t)(r >> 8);
                     set_nz16(s, (uint16_t)r); } break;
        case 0x9A: { uint8_t d = fetch(s);
                     uint16_t w = (uint16_t)(spc_read(s, dpaddr(s, d)) |
                                  (spc_read(s, dpaddr(s, (uint8_t)(d + 1))) << 8));
                     uint16_t ya = (uint16_t)(s->a | (s->y << 8));
                     int32_t r = (int32_t)ya - (int32_t)w;
                     s->psw &= (uint8_t)~(F_N | F_V | F_H | F_Z | F_C);
                     if (((ya ^ w) & (ya ^ (uint16_t)r) & 0x8000) != 0) s->psw |= F_V;
                     if (r >= 0) s->psw |= F_C;
                     s->a = (uint8_t)r; s->y = (uint8_t)(r >> 8);
                     set_nz16(s, (uint16_t)r); } break;
        case 0x5A: { uint8_t d = fetch(s);
                     uint16_t w = (uint16_t)(spc_read(s, dpaddr(s, d)) |
                                  (spc_read(s, dpaddr(s, (uint8_t)(d + 1))) << 8));
                     uint16_t ya = (uint16_t)(s->a | (s->y << 8));
                     int32_t r = (int32_t)ya - (int32_t)w;
                     s->psw &= (uint8_t)~(F_N | F_Z | F_C);
                     if (r >= 0) s->psw |= F_C;
                     set_nz16(s, (uint16_t)r); } break;
        case 0xCF: { uint16_t r = (uint16_t)(s->a * s->y); s->a = (uint8_t)r; s->y = (uint8_t)(r >> 8);
                     set_nz(s, s->y); } break;
        case 0x9E: { uint16_t ya = (uint16_t)(s->a | (s->y << 8));
                     if (s->x == 0) { s->a = 0xFF; s->y = 0xFF; }
                     else { s->a = (uint8_t)(ya / s->x); s->y = (uint8_t)(ya % s->x); }
                     set_nz(s, s->a); } break;

        /* ---- branches ---- */
        case 0x2F: { int8_t r = (int8_t)fetch(s); s->pc = (uint16_t)(s->pc + r); } break;
        case 0xF0: { int8_t r = (int8_t)fetch(s); if (s->psw & F_Z) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0xD0: { int8_t r = (int8_t)fetch(s); if (!(s->psw & F_Z)) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0xB0: { int8_t r = (int8_t)fetch(s); if (s->psw & F_C) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0x90: { int8_t r = (int8_t)fetch(s); if (!(s->psw & F_C)) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0x30: { int8_t r = (int8_t)fetch(s); if (s->psw & F_N) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0x10: { int8_t r = (int8_t)fetch(s); if (!(s->psw & F_N)) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0x70: { int8_t r = (int8_t)fetch(s); if (s->psw & F_V) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0x50: { int8_t r = (int8_t)fetch(s); if (!(s->psw & F_V)) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0xFE: { int8_t r = (int8_t)fetch(s); s->y--; if (s->y != 0) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0x6E: { uint16_t d = dpaddr(s, fetch(s)); int8_t r = (int8_t)fetch(s);
                     uint8_t v = (uint8_t)(spc_read(s, d) - 1); spc_write(s, d, v);
                     if (v != 0) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0xDE: { uint16_t d = dpaddr(s, (uint8_t)(fetch(s) + s->x)); int8_t r = (int8_t)fetch(s);
                     if (spc_read(s, d) != s->a) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;
        case 0x2E: { uint16_t d = dpaddr(s, fetch(s)); int8_t r = (int8_t)fetch(s);
                     if (spc_read(s, d) != s->a) { s->pc = (uint16_t)(s->pc + r); cycles += 2; } } break;

        /* ---- jumps / calls ---- */
        case 0x5F: s->pc = fetch16(s); break;
        case 0x1F: { uint16_t b = (uint16_t)(fetch16(s) + s->x);
                     s->pc = (uint16_t)(spc_read(s, b) | (spc_read(s, (uint16_t)(b + 1)) << 8)); } break;
        case 0x3F: { uint16_t d = fetch16(s); push(s, (uint8_t)(s->pc >> 8)); push(s, (uint8_t)s->pc); s->pc = d; } break;
        case 0x4F: { uint8_t d = fetch(s); push(s, (uint8_t)(s->pc >> 8)); push(s, (uint8_t)s->pc);
                     s->pc = (uint16_t)(0xFF00 | d); } break;
        case 0x6F: { uint8_t lo = pop(s); s->pc = (uint16_t)(lo | (pop(s) << 8)); } break;
        case 0x7F: { s->psw = pop(s); uint8_t lo = pop(s); s->pc = (uint16_t)(lo | (pop(s) << 8)); } break;
        case 0x0F: { push(s, (uint8_t)(s->pc >> 8)); push(s, (uint8_t)s->pc); push(s, s->psw);
                     s->psw |= F_B; s->psw &= (uint8_t)~F_I;
                     s->pc = (uint16_t)(spc_read(s, 0xFFDE) | (spc_read(s, 0xFFDF) << 8)); } break;

        /* ---- flags ---- */
        case 0x60: s->psw &= (uint8_t)~F_C; break;
        case 0x80: s->psw |= F_C; break;
        case 0xED: s->psw ^= F_C; break;
        case 0xE0: s->psw &= (uint8_t)~(F_V | F_H); break;
        case 0x20: s->psw &= (uint8_t)~F_P; break;
        case 0x40: s->psw |= F_P; break;
        case 0xA0: s->psw |= F_I; break;
        case 0xC0: s->psw &= (uint8_t)~F_I; break;
        case 0x00: break;                                  /* NOP  */
        case 0xEF: s->pc--; break;                          /* SLEEP */
        case 0xFF: s->pc--; break;                          /* STOP  */

        /* ---- bit ops on absolute.bit ---- */
        case 0x4A: case 0x6A: case 0x8A: case 0xAA: case 0xCA: case 0xEA:
        case 0x0A: case 0x2A: {
            uint16_t v = fetch16(s);
            uint16_t a = v & 0x1FFF;
            int bit = (v >> 13) & 7;
            uint8_t m = spc_read(s, a);
            int b = (m >> bit) & 1;
            switch (op) {
                case 0x4A: if (!b) s->psw &= (uint8_t)~F_C; break;                  /* AND1 */
                case 0x6A: if (b) s->psw &= (uint8_t)~F_C; break;                   /* AND1 /bit */
                case 0x8A: s->psw ^= (uint8_t)(b ? F_C : 0); break;                 /* EOR1 */
                case 0xAA: s->psw = (uint8_t)((s->psw & ~F_C) | b); break;          /* MOV1 C,bit */
                case 0xCA: m = (uint8_t)((m & ~(1 << bit)) | (((s->psw & F_C) ? 1 : 0) << bit));
                           spc_write(s, a, m); break;                                /* MOV1 bit,C */
                case 0xEA: spc_write(s, a, (uint8_t)(m ^ (1 << bit))); break;        /* NOT1 */
                case 0x0A: if (b) s->psw |= F_C; break;                              /* OR1 */
                case 0x2A: if (!b) s->psw |= F_C; break;                             /* OR1 /bit */
                default: break;
            }
        } break;

        default:
            /* SET1/CLR1 dp.bit (x2), BBS/BBC dp.bit,rel (x3), TCALL (x1) */
            if ((op & 0x0F) == 0x02) {
                uint16_t d = dpaddr(s, fetch(s));
                int bit = (op >> 5) & 7;
                uint8_t v = spc_read(s, d);
                if (op & 0x10) v = (uint8_t)(v & ~(1 << bit)); else v = (uint8_t)(v | (1 << bit));
                spc_write(s, d, v);
            } else if ((op & 0x0F) == 0x03) {
                uint16_t d = dpaddr(s, fetch(s));
                int8_t r = (int8_t)fetch(s);
                int bit = (op >> 5) & 7;
                int b = (spc_read(s, d) >> bit) & 1;
                int want = (op & 0x10) ? 0 : 1;
                if (b == want) { s->pc = (uint16_t)(s->pc + r); cycles += 2; }
            } else if ((op & 0x0F) == 0x01) {
                int n = (op >> 4) & 0x0F;
                uint16_t vec = (uint16_t)(0xFFDE - n * 2);
                push(s, (uint8_t)(s->pc >> 8)); push(s, (uint8_t)s->pc);
                s->pc = (uint16_t)(spc_read(s, vec) | (spc_read(s, (uint16_t)(vec + 1)) << 8));
            }
            break;
    }
    return cycles;
}

/* ------------------------------------------------------------------------
 * S-DSP
 * ------------------------------------------------------------------------ */

/* Envelope step periods in samples, indexed by the 5-bit rate field. */
static const int env_rate[32] = {
    0, 2048, 1536, 1280, 1024, 768, 640, 512, 384, 320, 256, 192, 160, 128,
    96, 80, 64, 48, 40, 32, 24, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2, 1
};

/* 4-point interpolation weights, generated as a gaussian-windowed sinc and
 * normalised to 2048 per position. This approximates the DSP's built-in
 * gaussian table closely enough to reproduce its characteristic filtering. */
static int16_t gauss_tab[256][4];
static bool gauss_ready = false;

static void gauss_init(void) {
    if (gauss_ready) return;
    for (int i = 0; i < 256; ++i) {
        double p = (double)i / 256.0;
        double w[4], sum = 0.0;
        for (int k = 0; k < 4; ++k) {
            double x = (double)(k - 1) - p;      /* taps at -1, 0, 1, 2 */
            double sinc = (fabs(x) < 1e-9) ? 1.0 : sin(3.14159265358979 * x) / (3.14159265358979 * x);
            double win = exp(-0.5 * (x / 1.7) * (x / 1.7));
            w[k] = sinc * win;
            sum += w[k];
        }
        for (int k = 0; k < 4; ++k) {
            gauss_tab[i][k] = (int16_t)((w[k] / sum) * 2048.0 + 0.5);
        }
    }
    gauss_ready = true;
}

static int clamp16(int v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return v;
}

static void dsp_write(NbaSpc *s, uint8_t addr, uint8_t value) {
    addr &= 0x7F;
    s->dsp[addr] = value;
    if (addr == NBA_DSP_KON) {
        for (int v = 0; v < 8; ++v) {
            if (value & (1 << v)) {
                s->voice[v].kon_delay_active = true;
                s->voice[v].kon_delay = 5;
            }
        }
    }
}

/* Decode one 9-byte BRR block into the voice ring (16 samples). */
static void brr_decode_block(NbaSpc *s, int idx) {
    NbaSpcVoice *v = &s->voice[idx];
    uint8_t header = s->ram[v->brr_addr];
    int shift = (header >> 4) & 0x0F;
    int filter = (header >> 2) & 0x03;
    int end = header & 0x01;
    int loop = (header >> 1) & 0x01;

    for (int i = 0; i < 16; ++i) {
        uint8_t byte = s->ram[(uint16_t)(v->brr_addr + 1 + i / 2)];
        int nib = (i & 1) ? (byte & 0x0F) : (byte >> 4);
        int sample = (nib < 8) ? nib : nib - 16;

        if (shift <= 12) sample = (sample << shift) >> 1;
        else sample = (sample < 0) ? -2048 : 0;

        switch (filter) {
            case 1:
                sample += v->last1 + ((-v->last1) >> 4);
                break;
            case 2:
                sample += (v->last1 << 1) + ((-(v->last1 * 3)) >> 5)
                          - v->last2 + (v->last2 >> 4);
                break;
            case 3:
                sample += (v->last1 << 1) + ((-(v->last1 * 13)) >> 6)
                          - v->last2 + ((v->last2 * 3) >> 4);
                break;
            default:
                break;
        }
        sample = clamp16(sample);
        v->last2 = v->last1;
        v->last1 = sample;

        v->ring[v->ring_write & 31] = (int16_t)sample;
        v->ring_write++;
    }

    if (end) {
        s->dsp[NBA_DSP_ENDX] |= (uint8_t)(1 << idx);
        if (loop) {
            uint16_t dir = (uint16_t)(s->dsp[NBA_DSP_DIR] << 8);
            uint8_t srcn = s->dsp[idx * 16 + NBA_DSP_SRCN];
            uint16_t entry = (uint16_t)(dir + srcn * 4);
            v->brr_addr = (uint16_t)(s->ram[entry + 2] | (s->ram[entry + 3] << 8));
        } else {
            v->reached_end = true;
        }
    } else {
        v->brr_addr = (uint16_t)(v->brr_addr + 9);
    }
}

/* Keep at least 8 decoded samples ahead of the read position. */
static void brr_fill(NbaSpc *s, int idx) {
    NbaSpcVoice *v = &s->voice[idx];
    int guard = 0;
    while (v->ring_write - v->ring_read < 8 && !v->reached_end && guard++ < 4) {
        brr_decode_block(s, idx);
    }
}

static void voice_keyon(NbaSpc *s, int idx) {
    NbaSpcVoice *v = &s->voice[idx];
    uint16_t dir = (uint16_t)(s->dsp[NBA_DSP_DIR] << 8);
    uint8_t srcn = s->dsp[idx * 16 + NBA_DSP_SRCN];
    uint16_t entry = (uint16_t)(dir + srcn * 4);

    v->brr_addr = (uint16_t)(s->ram[entry] | (s->ram[entry + 1] << 8));
    memset(v->ring, 0, sizeof(v->ring));
    v->ring_write = 0;
    v->ring_read = 0;
    v->last1 = 0;
    v->last2 = 0;
    v->reached_end = false;
    v->interp_pos = 0;
    v->env = 0;
    v->env_counter = 0;
    v->phase = NBA_ENV_ATTACK;
    v->active = true;
    s->dsp[NBA_DSP_ENDX] &= (uint8_t)~(1 << idx);

    brr_fill(s, idx);
}

static void env_run(NbaSpc *s, int idx) {
    NbaSpcVoice *v = &s->voice[idx];
    uint8_t adsr1 = s->dsp[idx * 16 + NBA_DSP_ADSR1];
    uint8_t adsr2 = s->dsp[idx * 16 + NBA_DSP_ADSR2];
    uint8_t gain = s->dsp[idx * 16 + NBA_DSP_GAIN];

    int rate = 0, step = 0;

    if (adsr1 & 0x80) {
        int ar = adsr1 & 0x0F;
        int dr = (adsr1 >> 4) & 0x07;
        int sr = adsr2 & 0x1F;
        int sl = (adsr2 >> 5) & 0x07;

        switch (v->phase) {
            case NBA_ENV_ATTACK:
                rate = env_rate[ar * 2 + 1];
                step = (ar == 15) ? 1024 : 32;
                break;
            case NBA_ENV_DECAY:
                rate = env_rate[dr * 2 + 16];
                step = -(((v->env - 1) >> 8) + 1);
                break;
            case NBA_ENV_SUSTAIN:
                rate = env_rate[sr];
                step = -(((v->env - 1) >> 8) + 1);
                break;
            case NBA_ENV_RELEASE:
            default:
                rate = 1;
                step = -8;
                break;
        }

        if (rate <= 0) return;
        if (++v->env_counter < rate) return;
        v->env_counter = 0;
        v->env += step;

        if (v->phase == NBA_ENV_ATTACK && v->env >= 0x7FF) {
            v->env = 0x7FF;
            v->phase = NBA_ENV_DECAY;
        } else if (v->phase == NBA_ENV_DECAY && v->env <= ((sl + 1) << 8)) {
            v->phase = NBA_ENV_SUSTAIN;
        }
    } else {
        if (!(gain & 0x80)) {
            v->env = (gain & 0x7F) << 4;   /* direct gain */
            return;
        }
        int mode = (gain >> 5) & 0x03;
        rate = env_rate[gain & 0x1F];
        if (rate <= 0) return;
        if (++v->env_counter < rate) return;
        v->env_counter = 0;
        switch (mode) {
            case 0: step = -32; break;
            case 1: step = -(((v->env - 1) >> 8) + 1); break;
            case 2: step = 32; break;
            default: step = (v->env < 0x600) ? 32 : 8; break;
        }
        v->env += step;
    }

    if (v->env > 0x7FF) v->env = 0x7FF;
    if (v->env < 0) {
        v->env = 0;
        if (v->phase == NBA_ENV_RELEASE) v->active = false;
    }
}

/* Produce one 32 kHz stereo sample. */
static void dsp_sample(NbaSpc *s, int16_t *left, int16_t *right) {
    uint8_t flg = s->dsp[NBA_DSP_FLG];
    uint8_t kof = s->dsp[NBA_DSP_KOF];
    int outl = 0, outr = 0;

    for (int i = 0; i < 8; ++i) {
        NbaSpcVoice *v = &s->voice[i];

        if (v->kon_delay_active) {
            if (--v->kon_delay <= 0) {
                v->kon_delay_active = false;
                voice_keyon(s, i);
            }
            v->out = 0;
            continue;
        }
        if (kof & (1 << i)) v->phase = NBA_ENV_RELEASE;
        if (!v->active) {
            v->out = 0;
            s->dsp[i * 16 + NBA_DSP_ENVX] = 0;
            continue;
        }

        env_run(s, i);
        brr_fill(s, i);

        if (v->reached_end && v->ring_write - v->ring_read < 4) {
            v->active = false;
            v->out = 0;
            continue;
        }

        int pitch = (int)(s->dsp[i * 16 + NBA_DSP_PITCH_L] |
                          ((s->dsp[i * 16 + NBA_DSP_PITCH_H] & 0x3F) << 8));

        if ((s->dsp[NBA_DSP_PMON] & (1 << i)) && i > 0) {
            int factor = (s->voice[i - 1].out >> 5) + 0x400;
            pitch = (pitch * factor) >> 10;
            if (pitch < 0) pitch = 0;
            if (pitch > 0x3FFF) pitch = 0x3FFF;
        }

        int frac = (v->interp_pos >> 4) & 0xFF;
        const int16_t *w = gauss_tab[frac];
        int acc = 0;
        for (int k = 0; k < 4; ++k) {
            acc += v->ring[(v->ring_read + k) & 31] * w[k];
        }
        int sample = clamp16(acc >> 11);

        if (s->dsp[NBA_DSP_NON] & (1 << i)) {
            s->noise_lfsr = ((s->noise_lfsr >> 1) |
                             (((s->noise_lfsr ^ (s->noise_lfsr >> 1)) & 1) << 14)) & 0x7FFF;
            sample = (int)(int16_t)(s->noise_lfsr << 1);
        }

        sample = (sample * v->env) >> 11;
        v->out = (int16_t)sample;

        s->dsp[i * 16 + NBA_DSP_ENVX] = (uint8_t)(v->env >> 4);
        s->dsp[i * 16 + NBA_DSP_OUTX] = (uint8_t)(sample >> 8);

        int vl = (int8_t)s->dsp[i * 16 + NBA_DSP_VOL_L];
        int vr = (int8_t)s->dsp[i * 16 + NBA_DSP_VOL_R];
        outl += (sample * vl) >> 7;
        outr += (sample * vr) >> 7;

        int pos = (int)v->interp_pos + pitch;
        v->ring_read += pos >> 12;
        v->interp_pos = (uint16_t)(pos & 0x0FFF);
    }

    if (flg & 0x40) { outl = 0; outr = 0; }   /* FLG bit 6 = mute all */

    outl = (outl * (int8_t)s->dsp[NBA_DSP_MVOL_L]) >> 7;
    outr = (outr * (int8_t)s->dsp[NBA_DSP_MVOL_R]) >> 7;

    *left = (int16_t)clamp16(outl);
    *right = (int16_t)clamp16(outr);
}

/* ------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------ */

bool nba_spc_load(NbaSpc *spc,
                  const void *ram, size_t ram_size,
                  const void *dsp, size_t dsp_size,
                  uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
                  uint8_t sp, uint8_t psw) {
    if (!spc || !ram || !dsp) return false;
    if (ram_size < NBA_SPC_RAM_SIZE || dsp_size < NBA_SPC_DSP_REGS) return false;

    memset(spc, 0, sizeof(*spc));
    memcpy(spc->ram, ram, NBA_SPC_RAM_SIZE);
    memcpy(spc->dsp, dsp, NBA_SPC_DSP_REGS);

    spc->pc = pc; spc->a = a; spc->x = x; spc->y = y; spc->sp = sp; spc->psw = psw;
    spc->noise_lfsr = 0x4000;
    spc->cycles_to_sample = NBA_SPC_CYCLES_PER_SAMPLE;

    /* No 65816 is driving the APU ports, so present them as idle. The
     * snapshot can otherwise land mid-handshake with the driver spinning at
     * $0443 (MOV A,$F4 / BNE) waiting for the CPU to clear a command. */
    spc->ram[0x00F4] = 0; spc->ram[0x00F5] = 0;
    spc->ram[0x00F6] = 0; spc->ram[0x00F7] = 0;

    spc->timer_target[0] = spc->ram[0x00FA];
    spc->timer_target[1] = spc->ram[0x00FB];
    spc->timer_target[2] = spc->ram[0x00FC];
    for (int t = 0; t < 3; ++t) spc->timer_enabled[t] = true;

    /* Resume the voices that the snapshot caught mid-note. */
    for (int i = 0; i < 8; ++i) {
        NbaSpcVoice *v = &spc->voice[i];
        uint8_t envx = spc->dsp[i * 16 + NBA_DSP_ENVX];
        if (envx > 0) {
            voice_keyon(spc, i);
            v->env = envx << 4;
            v->env_counter = 0;
            v->phase = NBA_ENV_SUSTAIN;
        }
    }

    gauss_init();
    spc->is_loaded = true;
    return true;
}

void nba_spc_render(NbaSpc *spc, int16_t *out, int frames) {
    if (!spc || !spc->is_loaded || !out) return;

    for (int f = 0; f < frames; ++f) {
        /* run the SPC700 for one sample's worth of cycles */
        int budget = NBA_SPC_CYCLES_PER_SAMPLE;
        while (budget > 0) {
            int c = spc_step(spc);
            budget -= c;

            /* timers: T0/T1 at 8 kHz (÷128), T2 at 64 kHz (÷16) */
            for (int t = 0; t < 3; ++t) {
                if (!spc->timer_enabled[t]) continue;
                spc->timer_div[t] += c;
                int period = (t == 2) ? 16 : 128;
                while (spc->timer_div[t] >= period) {
                    spc->timer_div[t] -= period;
                    spc->timer_counter[t]++;
                    int target = spc->timer_target[t] ? spc->timer_target[t] : 256;
                    if (spc->timer_counter[t] >= target) {
                        spc->timer_counter[t] = 0;
                        spc->timer_out[t] = (uint8_t)((spc->timer_out[t] + 1) & 0x0F);
                        nba_spc_dbg_timer_ticks++;
                    }
                }
            }
        }

        int16_t l, r;
        dsp_sample(spc, &l, &r);
        out[f * 2 + 0] = l;
        out[f * 2 + 1] = r;
    }
}
