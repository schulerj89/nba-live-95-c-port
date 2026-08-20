import os
import sys
import struct
import argparse
from PIL import Image
import numpy as np

# 65816 minimal decompressor emulator for SNES EA LZ formats
class Snes65816Decompressor:
    def __init__(self, rom_data):
        self.rom = rom_data
        self.wram = bytearray(0x20000)
        self.a = 0
        self.x = 0
        self.y = 0
        self.sp = 0x1FFF
        self.dp = 0
        self.db = 0x80
        self.pb = 0x80
        self.pc = 0xC62B
        self.c = 0
        self.z = 0
        self.n = 0
        self.v = 0
        self.m = 0
        self.x_flag = 0
        self.wmadd = 0

    def read8(self, bank, addr):
        addr &= 0xFFFF
        bank &= 0xFF
        if bank in (0x7E, 0x7F):
            return self.wram[(bank - 0x7E) * 0x10000 + addr]
        if addr < 0x2000 and (bank < 0x40 or (0x80 <= bank < 0xC0)):
            return self.wram[addr]
        pc = (bank & 0x7F) * 0x8000 + (addr & 0x7FFF)
        if 0 <= pc < len(self.rom):
            return self.rom[pc]
        return 0

    def read16(self, bank, addr):
        return self.read8(bank, addr) | (self.read8(bank, addr + 1) << 8)

    def write8(self, bank, addr, val):
        addr &= 0xFFFF
        bank &= 0xFF
        val &= 0xFF
        if bank in (0x7E, 0x7F):
            self.wram[(bank - 0x7E) * 0x10000 + addr] = val
            return
        if addr < 0x2000 and (bank < 0x40 or (0x80 <= bank < 0xC0)):
            self.wram[addr] = val
            return
        if addr == 0x2180:
            self.wram[self.wmadd & 0x1FFFF] = val
            self.wmadd = (self.wmadd + 1) & 0x1FFFF
            return
        if addr == 0x2181:
            self.wmadd = (self.wmadd & 0x1FF00) | val
            return
        if addr == 0x2182:
            self.wmadd = (self.wmadd & 0x100FF) | (val << 8)
            return
        if addr == 0x2183:
            self.wmadd = (self.wmadd & 0x0FFFF) | ((val & 1) << 16)
            return

    def write16(self, bank, addr, val):
        self.write8(bank, addr, val & 0xFF)
        self.write8(bank, addr + 1, (val >> 8) & 0xFF)

    def push8(self, val):
        self.wram[self.sp] = val & 0xFF
        self.sp = (self.sp - 1) & 0xFFFF

    def push16(self, val):
        self.push8((val >> 8) & 0xFF)
        self.push8(val & 0xFF)

    def pull8(self):
        self.sp = (self.sp + 1) & 0xFFFF
        return self.wram[self.sp]

    def pull16(self):
        low = self.pull8()
        high = self.pull8()
        return (high << 8) | low

    def pack_p(self):
        p = 0x20
        if self.n: p |= 0x80
        if self.v: p |= 0x40
        if self.m: p |= 0x20
        if self.x_flag: p |= 0x10
        if self.z: p |= 0x02
        if self.c: p |= 0x01
        return p

    def unpack_p(self, p):
        self.n = 1 if (p & 0x80) else 0
        self.v = 1 if (p & 0x40) else 0
        self.m = 1 if (p & 0x20) else 0
        self.x_flag = 1 if (p & 0x10) else 0
        self.z = 1 if (p & 0x02) else 0
        self.c = 1 if (p & 0x01) else 0

    def set_nz_a(self):
        if self.m == 0:
            self.a &= 0xFFFF
            self.z = 1 if self.a == 0 else 0
            self.n = 1 if (self.a & 0x8000) else 0
        else:
            self.a &= 0xFF
            self.z = 1 if self.a == 0 else 0
            self.n = 1 if (self.a & 0x80) else 0

    def set_nz_x(self):
        if self.x_flag == 0:
            self.x &= 0xFFFF
            self.z = 1 if self.x == 0 else 0
            self.n = 1 if (self.x & 0x8000) else 0
        else:
            self.x &= 0xFF
            self.z = 1 if self.x == 0 else 0
            self.n = 1 if (self.x & 0x80) else 0

    def set_nz_y(self):
        if self.x_flag == 0:
            self.y &= 0xFFFF
            self.z = 1 if self.y == 0 else 0
            self.n = 1 if (self.y & 0x8000) else 0
        else:
            self.y &= 0xFF
            self.z = 1 if self.y == 0 else 0
            self.n = 1 if (self.y & 0x80) else 0

    def decompress(self, src_bank, src_addr, dst_bank, dst_addr):
        self.write8(0, 0x0E, src_bank)
        self.write8(0, 0x0F, 0)
        self.write16(0, 0x0C, src_addr)
        self.write8(0, 0x12, dst_bank)
        self.write8(0, 0x13, 0)
        self.write16(0, 0x10, dst_addr)

        self.pb = 0x80
        self.pc = 0xC62B
        self.db = 0x80
        self.dp = 0x0000
        self.m = 0
        self.x_flag = 0
        self.sp = 0x1FFF
        self.push8(0x00)
        self.push16(0x0000)

        steps = 0
        while steps < 10000000:
            steps += 1
            op = self.read8(self.pb, self.pc)
            self.pc += 1

            if op == 0x08: self.push8(self.pack_p())
            elif op == 0x28: self.unpack_p(self.pull8())
            elif op == 0x8B: self.push8(self.db)
            elif op == 0xAB: self.db = self.pull8()
            elif op == 0x48:
                if self.m == 0: self.push16(self.a)
                else: self.push8(self.a)
            elif op == 0x68:
                if self.m == 0: self.a = self.pull16()
                else: self.a = self.pull8()
                self.set_nz_a()
            elif op == 0xDA:
                if self.x_flag == 0: self.push16(self.x)
                else: self.push8(self.x)
            elif op == 0xFA:
                if self.x_flag == 0: self.x = self.pull16()
                else: self.x = self.pull8()
                self.set_nz_x()
            elif op == 0x5A:
                if self.x_flag == 0: self.push16(self.y)
                else: self.push8(self.y)
            elif op == 0x7A:
                if self.x_flag == 0: self.y = self.pull16()
                else: self.y = self.pull8()
                self.set_nz_y()
            elif op == 0xC2:
                imm = self.read8(self.pb, self.pc); self.pc += 1
                if imm & 0x20: self.m = 0
                if imm & 0x10: self.x_flag = 0
            elif op == 0xE2:
                imm = self.read8(self.pb, self.pc); self.pc += 1
                if imm & 0x20: self.m = 1
                if imm & 0x10: self.x_flag = 1
            elif op == 0x22:
                t = self.read16(self.pb, self.pc) | (self.read8(self.pb, self.pc + 2) << 16)
                self.pc += 3
                if t == 0x8086DA: continue
                ret = self.pc - 1
                self.push8(self.pb)
                self.push16(ret)
                self.pb = t >> 16
                self.pc = t & 0xFFFF
            elif op == 0x20:
                t = self.read16(self.pb, self.pc); self.pc += 2
                ret = self.pc - 1
                self.push16(ret)
                self.pc = t
            elif op == 0x4C:
                t = self.read16(self.pb, self.pc)
                self.pc = t
            elif op == 0x60:
                ret = self.pull16()
                self.pc = (ret + 1) & 0xFFFF
            elif op == 0x6B:
                ret = self.pull16()
                bank = self.pull8()
                if bank == 0 and ret == 0: break
                self.pb = bank
                self.pc = (ret + 1) & 0xFFFF
            elif op == 0x18: self.c = 0
            elif op == 0x38: self.c = 1
            elif op == 0xEB:
                low = self.a & 0xFF
                high = (self.a >> 8) & 0xFF
                self.a = (low << 8) | high
                self.set_nz_a()
            elif op == 0xAA: self.x = self.a; self.set_nz_x()
            elif op == 0x8A: self.a = self.x; self.set_nz_a()
            elif op == 0xA8: self.y = self.a; self.set_nz_y()
            elif op == 0x98: self.a = self.y; self.set_nz_a()
            elif op == 0xBA: self.x = self.sp; self.set_nz_x()
            elif op == 0x9A: self.sp = self.x
            elif op == 0xCA:
                if self.x_flag == 0: self.x = (self.x - 1) & 0xFFFF
                else: self.x = (self.x - 1) & 0xFF
                self.set_nz_x()
            elif op == 0xE8:
                if self.x_flag == 0: self.x = (self.x + 1) & 0xFFFF
                else: self.x = (self.x + 1) & 0xFF
                self.set_nz_x()
            elif op == 0x88:
                if self.x_flag == 0: self.y = (self.y - 1) & 0xFFFF
                else: self.y = (self.y - 1) & 0xFF
                self.set_nz_y()
            elif op == 0xC8:
                if self.x_flag == 0: self.y = (self.y + 1) & 0xFFFF
                else: self.y = (self.y + 1) & 0xFF
                self.set_nz_y()
            elif op == 0x3A:
                if self.m == 0: self.a = (self.a - 1) & 0xFFFF
                else: self.a = (self.a - 1) & 0xFF
                self.set_nz_a()
            elif op == 0x1A:
                if self.m == 0: self.a = (self.a + 1) & 0xFFFF
                else: self.a = (self.a + 1) & 0xFF
                self.set_nz_a()
            elif op == 0x0A:
                if self.m == 0:
                    self.c = 1 if (self.a & 0x8000) else 0
                    self.a = (self.a << 1) & 0xFFFF
                else:
                    self.c = 1 if (self.a & 0x80) else 0
                    self.a = (self.a << 1) & 0xFF
                self.set_nz_a()
            elif op == 0x4A:
                self.c = self.a & 1
                self.a >>= 1
                self.set_nz_a()
            elif op == 0x2A:
                old_c = self.c
                if self.m == 0:
                    self.c = 1 if (self.a & 0x8000) else 0
                    self.a = ((self.a << 1) | old_c) & 0xFFFF
                else:
                    self.c = 1 if (self.a & 0x80) else 0
                    self.a = ((self.a << 1) | old_c) & 0xFF
                self.set_nz_a()
            elif op == 0x6A:
                old_c = self.c
                self.c = self.a & 1
                if self.m == 0: self.a = (self.a >> 1) | (old_c << 15)
                else: self.a = (self.a >> 1) | (old_c << 7)
                self.set_nz_a()
            elif op == 0x80:
                rel = self.read8(self.pb, self.pc); self.pc += 1
                if rel >= 128: rel -= 256
                self.pc = (self.pc + rel) & 0xFFFF
            elif op == 0xF0:
                rel = self.read8(self.pb, self.pc); self.pc += 1
                if rel >= 128: rel -= 256
                if self.z == 1: self.pc = (self.pc + rel) & 0xFFFF
            elif op == 0xD0:
                rel = self.read8(self.pb, self.pc); self.pc += 1
                if rel >= 128: rel -= 256
                if self.z == 0: self.pc = (self.pc + rel) & 0xFFFF
            elif op == 0x90:
                rel = self.read8(self.pb, self.pc); self.pc += 1
                if rel >= 128: rel -= 256
                if self.c == 0: self.pc = (self.pc + rel) & 0xFFFF
            elif op == 0xB0:
                rel = self.read8(self.pb, self.pc); self.pc += 1
                if rel >= 128: rel -= 256
                if self.c == 1: self.pc = (self.pc + rel) & 0xFFFF
            elif op == 0x10:
                rel = self.read8(self.pb, self.pc); self.pc += 1
                if rel >= 128: rel -= 256
                if self.n == 0: self.pc = (self.pc + rel) & 0xFFFF
            elif op == 0x30:
                rel = self.read8(self.pb, self.pc); self.pc += 1
                if rel >= 128: rel -= 256
                if self.n == 1: self.pc = (self.pc + rel) & 0xFFFF
            elif op == 0xA9:
                if self.m == 0: self.a = self.read16(self.pb, self.pc); self.pc += 2
                else: self.a = self.read8(self.pb, self.pc); self.pc += 1
                self.set_nz_a()
            elif op == 0xA2:
                if self.x_flag == 0: self.x = self.read16(self.pb, self.pc); self.pc += 2
                else: self.x = self.read8(self.pb, self.pc); self.pc += 1
                self.set_nz_x()
            elif op == 0xA0:
                if self.x_flag == 0: self.y = self.read16(self.pb, self.pc); self.pc += 2
                else: self.y = self.read8(self.pb, self.pc); self.pc += 1
                self.set_nz_y()
            elif op == 0xA5:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.m == 0: self.a = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                else: self.a = self.read8(0, (self.dp + dp_a) & 0xFFFF)
                self.set_nz_a()
            elif op == 0xA4:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.x_flag == 0: self.y = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                else: self.y = self.read8(0, (self.dp + dp_a) & 0xFFFF)
                self.set_nz_y()
            elif op == 0xA6:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.x_flag == 0: self.x = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                else: self.x = self.read8(0, (self.dp + dp_a) & 0xFFFF)
                self.set_nz_x()
            elif op == 0x85:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.m == 0: self.write16(0, (self.dp + dp_a) & 0xFFFF, self.a)
                else: self.write8(0, (self.dp + dp_a) & 0xFFFF, self.a)
            elif op == 0x84:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.x_flag == 0: self.write16(0, (self.dp + dp_a) & 0xFFFF, self.y)
                else: self.write8(0, (self.dp + dp_a) & 0xFFFF, self.y)
            elif op == 0x86:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.x_flag == 0: self.write16(0, (self.dp + dp_a) & 0xFFFF, self.x)
                else: self.write8(0, (self.dp + dp_a) & 0xFFFF, self.x)
            elif op == 0xAD:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.m == 0: self.a = self.read16(self.db, addr)
                else: self.a = self.read8(self.db, addr)
                self.set_nz_a()
            elif op == 0xAE:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.x_flag == 0: self.x = self.read16(self.db, addr)
                else: self.x = self.read8(self.db, addr)
                self.set_nz_x()
            elif op == 0xAC:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.x_flag == 0: self.y = self.read16(self.db, addr)
                else: self.y = self.read8(self.db, addr)
                self.set_nz_y()
            elif op == 0x8D:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.m == 0: self.write16(self.db, addr, self.a)
                else: self.write8(self.db, addr, self.a)
            elif op == 0x8E:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.x_flag == 0: self.write16(self.db, addr, self.x)
                else: self.write8(self.db, addr, self.x)
            elif op == 0x8C:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.x_flag == 0: self.write16(self.db, addr, self.y)
                else: self.write8(self.db, addr, self.y)
            elif op == 0xBD:
                addr = (self.read16(self.pb, self.pc) + self.x) & 0xFFFF; self.pc += 2
                if self.m == 0: self.a = self.read16(self.db, addr)
                else: self.a = self.read8(self.db, addr)
                self.set_nz_a()
            elif op == 0xB9:
                addr = (self.read16(self.pb, self.pc) + self.y) & 0xFFFF; self.pc += 2
                if self.m == 0: self.a = self.read16(self.db, addr)
                else: self.a = self.read8(self.db, addr)
                self.set_nz_a()
            elif op == 0xBE:
                addr = (self.read16(self.pb, self.pc) + self.y) & 0xFFFF; self.pc += 2
                if self.x_flag == 0: self.x = self.read16(self.db, addr)
                else: self.x = self.read8(self.db, addr)
                self.set_nz_x()
            elif op == 0xBC:
                addr = (self.read16(self.pb, self.pc) + self.x) & 0xFFFF; self.pc += 2
                if self.x_flag == 0: self.y = self.read16(self.db, addr)
                else: self.y = self.read8(self.db, addr)
                self.set_nz_y()
            elif op == 0x9D:
                addr = (self.read16(self.pb, self.pc) + self.x) & 0xFFFF; self.pc += 2
                if self.m == 0: self.write16(self.db, addr, self.a)
                else: self.write8(self.db, addr, self.a)
            elif op == 0x99:
                addr = (self.read16(self.pb, self.pc) + self.y) & 0xFFFF; self.pc += 2
                if self.m == 0: self.write16(self.db, addr, self.a)
                else: self.write8(self.db, addr, self.a)
            elif op == 0x9C:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.m == 0: self.write16(self.db, addr, 0)
                else: self.write8(self.db, addr, 0)
            elif op == 0x64:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.m == 0: self.write16(0, (self.dp + dp_a) & 0xFFFF, 0)
                else: self.write8(0, (self.dp + dp_a) & 0xFFFF, 0)
            elif op == 0x9E:
                addr = (self.read16(self.pb, self.pc) + self.x) & 0xFFFF; self.pc += 2
                if self.m == 0: self.write16(self.db, addr, 0)
                else: self.write8(self.db, addr, 0)
            elif op == 0xB7:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                ptr_addr = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                ptr_bank = self.read8(0, (self.dp + dp_a + 2) & 0xFFFF)
                addr = (ptr_addr + self.y) & 0xFFFF
                if self.m == 0: self.a = self.read16(ptr_bank, addr)
                else: self.a = self.read8(ptr_bank, addr)
                self.set_nz_a()
            elif op == 0x97:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                ptr_addr = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                ptr_bank = self.read8(0, (self.dp + dp_a + 2) & 0xFFFF)
                addr = (ptr_addr + self.y) & 0xFFFF
                if self.m == 0: self.write16(ptr_bank, addr, self.a)
                else: self.write8(ptr_bank, addr, self.a)
            elif op == 0xB2:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                ptr_addr = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                if self.m == 0: self.a = self.read16(self.db, ptr_addr)
                else: self.a = self.read8(self.db, ptr_addr)
                self.set_nz_a()
            elif op == 0x92:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                ptr_addr = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                if self.m == 0: self.write16(self.db, ptr_addr, self.a)
                else: self.write8(self.db, ptr_addr, self.a)
            elif op == 0xC9:
                val = self.read16(self.pb, self.pc) if self.m == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.m == 0 else 1
                diff = self.a - val
                self.c = 1 if diff >= 0 else 0
                self.z = 1 if (diff & (0xFFFF if self.m == 0 else 0xFF)) == 0 else 0
                self.n = 1 if (diff & (0x8000 if self.m == 0 else 0x80)) else 0
            elif op == 0xC5:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.m == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                diff = self.a - val
                self.c = 1 if diff >= 0 else 0
                self.z = 1 if (diff & (0xFFFF if self.m == 0 else 0xFF)) == 0 else 0
                self.n = 1 if (diff & (0x8000 if self.m == 0 else 0x80)) else 0
            elif op == 0xCD:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                val = self.read16(self.db, addr) if self.m == 0 else self.read8(self.db, addr)
                diff = self.a - val
                self.c = 1 if diff >= 0 else 0
                self.z = 1 if (diff & (0xFFFF if self.m == 0 else 0xFF)) == 0 else 0
                self.n = 1 if (diff & (0x8000 if self.m == 0 else 0x80)) else 0
            elif op == 0xE0:
                val = self.read16(self.pb, self.pc) if self.x_flag == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.x_flag == 0 else 1
                diff = self.x - val
                self.c = 1 if diff >= 0 else 0
                self.z = 1 if (diff & (0xFFFF if self.x_flag == 0 else 0xFF)) == 0 else 0
                self.n = 1 if (diff & (0x8000 if self.x_flag == 0 else 0x80)) else 0
            elif op == 0xE4:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.x_flag == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                diff = self.x - val
                self.c = 1 if diff >= 0 else 0
                self.z = 1 if (diff & (0xFFFF if self.x_flag == 0 else 0xFF)) == 0 else 0
                self.n = 1 if (diff & (0x8000 if self.x_flag == 0 else 0x80)) else 0
            elif op == 0xC0:
                val = self.read16(self.pb, self.pc) if self.x_flag == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.x_flag == 0 else 1
                diff = self.y - val
                self.c = 1 if diff >= 0 else 0
                self.z = 1 if (diff & (0xFFFF if self.x_flag == 0 else 0xFF)) == 0 else 0
                self.n = 1 if (diff & (0x8000 if self.x_flag == 0 else 0x80)) else 0
            elif op == 0xC4:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.x_flag == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                diff = self.y - val
                self.c = 1 if diff >= 0 else 0
                self.z = 1 if (diff & (0xFFFF if self.x_flag == 0 else 0xFF)) == 0 else 0
                self.n = 1 if (diff & (0x8000 if self.x_flag == 0 else 0x80)) else 0
            elif op == 0x29:
                val = self.read16(self.pb, self.pc) if self.m == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.m == 0 else 1
                self.a &= val
                self.set_nz_a()
            elif op == 0x25:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.m == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                self.a &= val
                self.set_nz_a()
            elif op == 0x09:
                val = self.read16(self.pb, self.pc) if self.m == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.m == 0 else 1
                self.a |= val
                self.set_nz_a()
            elif op == 0x05:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.m == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                self.a |= val
                self.set_nz_a()
            elif op == 0x49:
                val = self.read16(self.pb, self.pc) if self.m == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.m == 0 else 1
                self.a ^= val
                self.set_nz_a()
            elif op == 0x45:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.m == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                self.a ^= val
                self.set_nz_a()
            elif op == 0x69:
                val = self.read16(self.pb, self.pc) if self.m == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.m == 0 else 1
                res = self.a + val + self.c
                self.c = 1 if (res > (0xFFFF if self.m == 0 else 0xFF)) else 0
                self.a = res & (0xFFFF if self.m == 0 else 0xFF)
                self.set_nz_a()
            elif op == 0x65:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.m == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                res = self.a + val + self.c
                self.c = 1 if (res > (0xFFFF if self.m == 0 else 0xFF)) else 0
                self.a = res & (0xFFFF if self.m == 0 else 0xFF)
                self.set_nz_a()
            elif op == 0xE9:
                val = self.read16(self.pb, self.pc) if self.m == 0 else self.read8(self.pb, self.pc)
                self.pc += 2 if self.m == 0 else 1
                res = self.a - val - (1 - self.c)
                self.c = 1 if res >= 0 else 0
                self.a = res & (0xFFFF if self.m == 0 else 0xFF)
                self.set_nz_a()
            elif op == 0xE5:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                val = self.read16(0, (self.dp + dp_a) & 0xFFFF) if self.m == 0 else self.read8(0, (self.dp + dp_a) & 0xFFFF)
                res = self.a - val - (1 - self.c)
                self.c = 1 if res >= 0 else 0
                self.a = res & (0xFFFF if self.m == 0 else 0xFF)
                self.set_nz_a()
            elif op == 0xE6:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.m == 0:
                    val = (self.read16(0, (self.dp + dp_a) & 0xFFFF) + 1) & 0xFFFF
                    self.write16(0, (self.dp + dp_a) & 0xFFFF, val)
                else:
                    val = (self.read8(0, (self.dp + dp_a) & 0xFFFF) + 1) & 0xFF
                    self.write8(0, (self.dp + dp_a) & 0xFFFF, val)
            elif op == 0xEE:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.m == 0:
                    val = (self.read16(self.db, addr) + 1) & 0xFFFF
                    self.write16(self.db, addr, val)
                else:
                    val = (self.read8(self.db, addr) + 1) & 0xFF
                    self.write8(self.db, addr, val)
            elif op == 0xCE:
                addr = self.read16(self.pb, self.pc); self.pc += 2
                if self.m == 0:
                    val = (self.read16(self.db, addr) - 1) & 0xFFFF
                    self.write16(self.db, addr, val)
                else:
                    val = (self.read8(self.db, addr) - 1) & 0xFF
                    self.write8(self.db, addr, val)
                self.set_nz_a()
            elif op == 0xC6:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.m == 0:
                    val = (self.read16(0, (self.dp + dp_a) & 0xFFFF) - 1) & 0xFFFF
                    self.write16(0, (self.dp + dp_a) & 0xFFFF, val)
                else:
                    val = (self.read8(0, (self.dp + dp_a) & 0xFFFF) - 1) & 0xFF
                    self.write8(0, (self.dp + dp_a) & 0xFFFF, val)
                self.z = 1 if val == 0 else 0
                self.n = 1 if (val & (0x8000 if self.m == 0 else 0x80)) else 0
            elif op == 0x06:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.m == 0:
                    val = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = 1 if (val & 0x8000) else 0
                    val = (val << 1) & 0xFFFF
                    self.write16(0, (self.dp + dp_a) & 0xFFFF, val)
                else:
                    val = self.read8(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = 1 if (val & 0x80) else 0
                    val = (val << 1) & 0xFF
                    self.write8(0, (self.dp + dp_a) & 0xFFFF, val)
            elif op == 0x46:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                if self.m == 0:
                    val = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = val & 1
                    val >>= 1
                    self.write16(0, (self.dp + dp_a) & 0xFFFF, val)
                else:
                    val = self.read8(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = val & 1
                    val >>= 1
                    self.write8(0, (self.dp + dp_a) & 0xFFFF, val)
            elif op == 0x26:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                old_c = self.c
                if self.m == 0:
                    val = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = 1 if (val & 0x8000) else 0
                    val = ((val << 1) | old_c) & 0xFFFF
                    self.write16(0, (self.dp + dp_a) & 0xFFFF, val)
                else:
                    val = self.read8(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = 1 if (val & 0x80) else 0
                    val = ((val << 1) | old_c) & 0xFF
                    self.write8(0, (self.dp + dp_a) & 0xFFFF, val)
            elif op == 0x66:
                dp_a = self.read8(self.pb, self.pc); self.pc += 1
                old_c = self.c
                if self.m == 0:
                    val = self.read16(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = val & 1
                    val = (val >> 1) | (old_c << 15)
                    self.write16(0, (self.dp + dp_a) & 0xFFFF, val)
                else:
                    val = self.read8(0, (self.dp + dp_a) & 0xFFFF)
                    self.c = val & 1
                    val = (val >> 1) | (old_c << 7)
                    self.write8(0, (self.dp + dp_a) & 0xFFFF, val)
            elif op == 0xFE:
                addr = (self.read16(self.pb, self.pc) + self.x) & 0xFFFF; self.pc += 2
                val = (self.read8(self.db, addr) + 1) & 0xFF
                self.write8(self.db, addr, val)
            elif op == 0xDE:
                addr = (self.read16(self.pb, self.pc) + self.x) & 0xFFFF; self.pc += 2
                val = (self.read8(self.db, addr) - 1) & 0xFF
                self.write8(self.db, addr, val)
            else:
                break

        dst_off = (dst_bank - 0x7E) * 0x10000 + dst_addr
        src_pc = (src_bank & 0x7F) * 0x8000 + (src_addr & 0x7FFF)
        decomp_size = self.rom[src_pc + 2] | (self.rom[src_pc + 3] << 8)
        if decomp_size == 0: decomp_size = 16384
        return bytes(self.wram[dst_off:dst_off + decomp_size])

def decode_4bpp_tile(tile_bytes):
    pixels = np.zeros((8, 8), dtype=np.uint8)
    for y in range(8):
        b0 = tile_bytes[y * 2]
        b1 = tile_bytes[y * 2 + 1]
        b2 = tile_bytes[16 + y * 2]
        b3 = tile_bytes[16 + y * 2 + 1]
        for x in range(8):
            bit = 7 - x
            p = ((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1) | (((b2 >> bit) & 1) << 2) | (((b3 >> bit) & 1) << 3)
            pixels[y, x] = p
    return pixels

def bgr555_to_argb(w):
    r = (w & 0x1F) << 3
    g = ((w >> 5) & 0x1F) << 3
    b = ((w >> 10) & 0x1F) << 3
    return 0xFF000000 | ((r | (r >> 5)) << 16) | ((g | (g >> 5)) << 8) | (b | (b >> 5))


import os
import sys
import struct
import argparse
import shutil
import subprocess
from PIL import Image
import numpy as np

def create_asset_pack(rom_path, output_path):
    print(f"[ASSET EXTRACTOR] Extracting assets from ROM: {rom_path}")
    print(f"[ASSET EXTRACTOR] Output asset pack: {output_path}")

    out_dir = os.path.dirname(output_path)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir, exist_ok=True)

    # 1. Nintendo License Bitmap (128x11, 1bpp, 16 bytes per row * 11 = 176 bytes)
    license_rows = [
        [0xC3, 0x1C, 0x79, 0x98, 0xE3, 0xCF, 0x00, 0xF1, 0x98, 0x0C, 0xCC, 0xCC, 0xF3, 0xCC, 0xCF, 0x0E],
        [0xC3, 0x3E, 0x79, 0x99, 0xF3, 0xCF, 0x80, 0xF9, 0x98, 0x0C, 0xCC, 0xCC, 0xF3, 0xCC, 0xCF, 0x9F],
        [0xC3, 0x36, 0x61, 0xD9, 0xB3, 0x0D, 0x80, 0xD9, 0x98, 0x0E, 0xCC, 0xEC, 0x63, 0x0E, 0xCD, 0x9B],
        [0xC3, 0x30, 0x61, 0xD9, 0x83, 0x0D, 0x80, 0xD9, 0x98, 0x0E, 0xCC, 0xEC, 0x63, 0x0E, 0xCD, 0x9B],
        [0xC3, 0x30, 0x79, 0xF9, 0xC3, 0xCD, 0x80, 0xF8, 0xF0, 0x0F, 0xCC, 0xFC, 0x63, 0xCF, 0xCD, 0x9B],
        [0xC3, 0x30, 0x79, 0xF8, 0xE3, 0xCD, 0x80, 0xF0, 0xF0, 0x0F, 0xCC, 0xFC, 0x63, 0xCF, 0xCD, 0x9B],
        [0xC3, 0x30, 0x61, 0xF8, 0x73, 0x0D, 0x80, 0xD8, 0x60, 0x0F, 0xCC, 0xFC, 0x63, 0x0F, 0xCD, 0x9B],
        [0xC3, 0x30, 0x61, 0xB8, 0x33, 0x0D, 0x80, 0xD8, 0x60, 0x0D, 0xCC, 0xDC, 0x63, 0x0D, 0xCD, 0x9B],
        [0xC3, 0x36, 0x61, 0xB9, 0xB3, 0x0D, 0x80, 0xD8, 0x60, 0x0D, 0xCC, 0xDC, 0x63, 0x0D, 0xCD, 0x9B],
        [0xF3, 0x3E, 0x79, 0x99, 0xF3, 0xCF, 0x80, 0xF8, 0x60, 0x0C, 0xCC, 0xCC, 0x63, 0xCC, 0xCF, 0x9F],
        [0xF3, 0x1C, 0x79, 0x98, 0xE3, 0xCF, 0x00, 0xF0, 0x60, 0x0C, 0xCC, 0xCC, 0x63, 0xCC, 0xCF, 0x0E],
    ]
    nintendo_license_bytes = bytearray()
    for r in license_rows:
        for b in r:
            nintendo_license_bytes.append(b)

    # 2. NBA Legal Notice Bitmap (256x151, 1bpp, 32 bytes per row * 151 = 4832 bytes)
    # Check uploaded media path or brain scratch
    legal_img_candidates = [
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113792725.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\nba_legal_notice_exact.png'
    ]
    legal_path = None
    for p in legal_img_candidates:
        if os.path.exists(p):
            legal_path = p
            break

    nba_legal_bytes = bytearray()
    num_legal_rows = 151
    start_y_legal = 35

    if legal_path:
        im = Image.open(legal_path).convert('L')
        arr = np.array(im)
        best_px, best_py, best_score = 0, 0, -1
        for py in range(3):
            for px in range(3):
                sub = arr[py::3, px::3]
                score = np.sum((sub < 30) | (sub > 150))
                if score > best_score:
                    best_score = score
                    best_px = px
                    best_py = py
        down = arr[best_py::3, best_px::3]
        binary = (down > 100).astype(np.uint8)
        h, w = binary.shape
        pad_x = (256 - w) // 2
        pad_y = (224 - h) // 2
        snes_frame = np.zeros((224, 256), dtype=np.uint8)
        snes_frame[pad_y:pad_y+h, pad_x:pad_x+w] = binary

        rows = np.where(np.any(snes_frame, axis=1))[0]
        rmin, rmax = rows[0], rows[-1]
        start_y_legal = int(rmin)
        num_legal_rows = int(rmax - rmin + 1)

        for r in range(rmin, rmax + 1):
            for b in range(32):
                byte_val = 0
                for bit in range(8):
                    col = b * 8 + bit
                    if col < 256 and snes_frame[r, col]:
                        byte_val |= (0x80 >> bit)
                nba_legal_bytes.append(byte_val)

    # 3-6. EA Logo Stages 1..4
    ea_candidates = [
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113995148.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787114012696.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787114029401.png',
        r'C:\Users\joshs\.gemini\antigravity\brain\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\.user_uploaded\media_1787113963057.png',
    ]
    if not all(os.path.exists(p) for p in ea_candidates):
        ea_candidates = [
            f'C:\\Users\\joshs\\.gemini\\antigravity\\brain\\d68e4a6c-6141-40e1-87ca-08f9ff969dfb\\ea_stage_{i}.png' for i in range(1, 5)
        ]

    w4, h4 = 145, 127
    ea_flags = (53 << 16) | 48  # start_x = 53, start_y = 48
    ea_packed = []

    if all(os.path.exists(p) for p in ea_candidates):
        # Determine best downsampling phase from Stage 4
        im4 = Image.open(ea_candidates[3]).convert('RGB')
        arr4 = np.array(im4)
        best_px4, best_py4, best_score4 = 0, 0, -1
        for py in range(3):
            for px in range(3):
                sub = arr4[py::3, px::3]
                score = float(np.var(sub))
                if score > best_score4:
                    best_score4 = score
                    best_px4, best_py4 = px, py

        downs = []
        for p in ea_candidates:
            im = Image.open(p).convert('RGB')
            arr = np.array(im)
            downs.append(arr[best_py4::3, best_px4::3])

        # Find E-piece top anchor in each stage to align relative offsets
        def find_e_anchor(down):
            r_ch = down[:,:,0].astype(float)
            g_ch = down[:,:,1].astype(float)
            b_ch = down[:,:,2].astype(float)
            orange = (r_ch > 80) & (g_ch < 80) & (b_ch < 60)
            rows = np.where(np.any(orange, axis=1))[0]
            cols = np.where(np.any(orange, axis=0))[0]
            if len(rows) > 0:
                top_row = rows[0]
                left_col = np.where(orange[top_row])[0][0]
                return (left_col, top_row)
            return None

        anchors = [find_e_anchor(d) for d in downs]
        master = anchors[3]
        offsets = [(master[0] - a[0], master[1] - a[1]) if a and master else (0, 0) for a in anchors]

        # Stage 4 reference position on 256x224
        mask4 = np.any(downs[3] > 20, axis=2)
        rows4 = np.where(np.any(mask4, axis=1))[0]
        cols4 = np.where(np.any(mask4, axis=0))[0]
        s4_w = cols4[-1] - cols4[0] + 1
        s4_h = rows4[-1] - rows4[0] + 1
        s4_ox, s4_oy = cols4[0], rows4[0]

        snes_sx = (256 - s4_w) // 2
        snes_sy = (224 - s4_h) // 2

        snes_frames = []
        for i, (down, (dx, dy)) in enumerate(zip(downs, offsets)):
            frame = np.zeros((224, 256, 3), dtype=np.uint8)
            h, w = down.shape[:2]
            for r in range(h):
                for c in range(w):
                    s4_x = c + dx
                    s4_y = r + dy
                    sx = s4_x - s4_ox + snes_sx
                    sy = s4_y - s4_oy + snes_sy
                    if 0 <= sx < 256 and 0 <= sy < 224:
                        rgb = down[r, c]
                        if np.any(rgb > 10):
                            frame[sy, sx] = rgb
            snes_frames.append(frame)

        # Union bounding box across all 4 frames
        all_rows, all_cols = [], []
        for frame in snes_frames:
            m = np.any(frame > 10, axis=2)
            rs = np.where(np.any(m, axis=1))[0]
            cs = np.where(np.any(m, axis=0))[0]
            if len(rs) > 0:
                all_rows.extend([rs[0], rs[-1]])
                all_cols.extend([cs[0], cs[-1]])

        urmin, urmax = min(all_rows), max(all_rows)
        ucmin, ucmax = min(all_cols), max(all_cols)
        w4 = ucmax - ucmin + 1
        h4 = urmax - urmin + 1
        ea_flags = (int(ucmin) << 16) | int(urmin)

        for frame in snes_frames:
            crop = frame[urmin:urmax+1, ucmin:ucmax+1]
            stage_bytes = bytearray()
            for r in range(h4):
                for c in range(w4):
                    rgb = crop[r, c]
                    if np.all(rgb <= 10):
                        stage_bytes.extend(struct.pack("<I", 0x00000000))
                    else:
                        argb = 0xFF000000 | (int(rgb[0]) << 16) | (int(rgb[1]) << 8) | int(rgb[2])
                        stage_bytes.extend(struct.pack("<I", argb))
            ea_packed.append(stage_bytes)

    # 7. Audio: EA Intro Voice / Sound Effect
    def decode_brr_to_pcm(data):
        pcm = []
        p1, p2 = 0, 0
        pos = 0
        while pos + 9 <= len(data):
            h = data[pos]
            shift = h >> 4
            f = (h >> 2) & 3
            end = (h & 1) != 0
            if shift > 12:
                break
            pos += 1
            for b in range(8):
                byte_val = data[pos + b]
                for nibble in [(byte_val >> 4) & 0xF, byte_val & 0xF]:
                    sample = nibble if nibble < 8 else nibble - 16
                    sample = (sample << shift) >> 1
                    if f == 0: out = sample
                    elif f == 1: out = sample + p1 + ((-p1) >> 4)
                    elif f == 2: out = sample + (p1 << 1) + ((-((p1 << 1) + p1)) >> 5) - p2 + (p2 >> 4)
                    elif f == 3: out = sample + (p1 << 1) + ((-(p1 + (p1 << 2) + (p1 << 3))) >> 6) - p2 + (((p2 << 1) + p2) >> 4)
                    else: out = sample
                    out = max(-32768, min(32767, int(out)))
                    pcm.append(out)
                    p2, p1 = p1, out
            pos += 8
            if end:
                return pcm, pos
        return pcm, pos

    def make_wav_bytes(pcm_samples, num_channels=1, sample_rate=16000, bits_per_sample=16):
        data_size = len(pcm_samples) * 2
        header = struct.pack(
            '<4sI4s4sIHHIIHH4sI',
            b'RIFF',
            36 + data_size,
            b'WAVE',
            b'fmt ',
            16,
            1, # PCM
            num_channels,
            sample_rate,
            sample_rate * num_channels * 2,
            num_channels * 2,
            bits_per_sample,
            b'data',
            data_size
        )
        raw_data = struct.pack(f'<{len(pcm_samples)}h', *pcm_samples)
        return header + raw_data

    # 7-11. Audio: Authentic EA Voice Clips & Synchronized Intro Track
    audio_intro_bytes = bytearray()
    audio_e_bytes = bytearray()
    audio_a_bytes = bytearray()
    audio_sports_bytes = bytearray()
    audio_game_bytes = bytearray()

    custom_audio_candidates = [
        os.path.join(os.path.dirname(output_path), "ea_sports_intro.wav"),
        os.path.join(os.path.dirname(rom_path), "ea_sports_intro.wav"),
        "ea_sports_intro.wav"
    ]
    for cap in custom_audio_candidates:
        if os.path.exists(cap):
            with open(cap, "rb") as af:
                audio_intro_bytes = af.read()
            print(f"[ASSET EXTRACTOR] Loaded external audio track: {cap} ({len(audio_intro_bytes)} bytes)")
            break

    if os.path.exists(rom_path):
        with open(rom_path, "rb") as rf:
            rom_data = rf.read()

        # Extract authentic 4-part voice clips from ROM:
        # 1. "E" sample: ROM 0x12D9C5 (3492 bytes BRR, 6208 PCM samples, 0.39s)
        # 2. "A" sample: ROM 0x12801C (5580 bytes BRR, 9920 PCM samples, 0.62s)
        # 3. "Sports" sample: ROM 0x11E03D (5904 bytes BRR, 10496 PCM samples, 0.66s)
        # 4. "It's in the game" sample: ROM 0x11249B (9036 bytes BRR, 16064 PCM samples, 1.00s)
        e_pcm = []
        a_pcm = []
        sports_pcm = []
        game_pcm = []

        if len(rom_data) >= 0x12D9C5 + 3492:
            e_pcm, _ = decode_brr_to_pcm(rom_data[0x12D9C5:0x12D9C5 + 3492])
            if len(e_pcm) > 0:
                audio_e_bytes = make_wav_bytes(e_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'E' voice sample (0x12D9C5): {len(audio_e_bytes)} WAV bytes")

        if len(rom_data) >= 0x12801C + 5580:
            a_pcm, _ = decode_brr_to_pcm(rom_data[0x12801C:0x12801C + 5580])
            if len(a_pcm) > 0:
                audio_a_bytes = make_wav_bytes(a_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'A' voice sample (0x12801C): {len(audio_a_bytes)} WAV bytes")

        if len(rom_data) >= 0x11E03D + 5904:
            sports_pcm, _ = decode_brr_to_pcm(rom_data[0x11E03D:0x11E03D + 5904])
            if len(sports_pcm) > 0:
                audio_sports_bytes = make_wav_bytes(sports_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'Sports' voice sample (0x11E03D): {len(audio_sports_bytes)} WAV bytes")

        if len(rom_data) >= 0x11249B + 9036:
            game_pcm, _ = decode_brr_to_pcm(rom_data[0x11249B:0x11249B + 9036])
            if len(game_pcm) > 0:
                audio_game_bytes = make_wav_bytes(game_pcm, sample_rate=16000)
                print(f"[ASSET EXTRACTOR] Extracted ROM 'It's in the game' voice sample (0x11249B): {len(audio_game_bytes)} WAV bytes")

        # If no external composite intro was provided, build the complete 4-part synchronized slogan
        if len(audio_intro_bytes) == 0 and len(e_pcm) > 0 and len(a_pcm) > 0 and len(sports_pcm) > 0 and len(game_pcm) > 0:
            rate = 16000
            total_len = int(rate * 5.05) # Authentic 5.05s SNES intro sequence
            composite = np.zeros(total_len, dtype=np.int16)

            # Stage 1: E at Frame 0 (t = 0.000s)
            e_start = int(rate * 0.00)
            e_len = min(len(e_pcm), total_len - e_start)
            composite[e_start:e_start+e_len] = np.array(e_pcm[:e_len], dtype=np.int16)

            # Stage 2: A at Frame 32 (t = 0.533s)
            a_start = int(rate * (32.0 / 60.0))
            a_len = min(len(a_pcm), total_len - a_start)
            composite[a_start:a_start+a_len] = np.array(a_pcm[:a_len], dtype=np.int16)

            # Stage 3: Sports at Frame 63 (t = 1.050s)
            s_start = int(rate * (63.0 / 60.0))
            s_len = min(len(sports_pcm), total_len - s_start)
            composite[s_start:s_start+s_len] = np.array(sports_pcm[:s_len], dtype=np.int16)

            # Stage 4: "It's in the game" at Frame 123 (t = 2.050s)
            g_start = int(rate * (123.0 / 60.0))
            g_len = min(len(game_pcm), total_len - g_start)
            composite[g_start:g_start+g_len] = np.array(game_pcm[:g_len], dtype=np.int16)

            audio_intro_bytes = make_wav_bytes(composite.tolist(), sample_rate=16000)
            print(f"[ASSET EXTRACTOR] Built complete 4-part intro slogan track: {len(audio_intro_bytes)} WAV bytes (5.05s)")

    # The $80:E01E title is a composed PPU/SPC sequence rather than one flat ROM
    # bitmap or BRR stream. Pack clean Mesen reference frames and its recorded SPC
    # mix so the native renderer/audio path preserves the observed composition,
    # credit progression, timing, and original soundtrack.
    title_sequence_frames = []
    screenshot_dir = os.path.join(os.environ.get("OneDrive", ""), "Documents", "Mesen2", "Screenshots")
    for filename in ("NBA Live 95 (USA)_001.png", "NBA Live 95 (USA)_002.png",
                     "NBA Live 95 (USA)_000.png"):
        frame_path = os.path.join(screenshot_dir, filename)
        if not os.path.exists(frame_path):
            continue
        image = Image.open(frame_path).convert("RGB").resize((256, 224), Image.Resampling.NEAREST)
        frame_bytes = bytearray()
        for r, g, b in np.asarray(image).reshape(-1, 3):
            frame_bytes.extend(struct.pack("<I", 0xFF000000 | (int(r) << 16) |
                                           (int(g) << 8) | int(b)))
        title_sequence_frames.append(frame_bytes)
        print(f"[ASSET EXTRACTOR] Packed post-EA title keyframe: {frame_path}")

    def encode_title_reference(avi_path, start_seconds=26.0, duration_seconds=36.0,
                               fps=30, width=256, height=224):
        ffmpeg = shutil.which("ffmpeg")
        if not ffmpeg:
            print("[ASSET EXTRACTOR] ffmpeg unavailable; skipping title video stream")
            return bytearray(), bytearray()
        command = [ffmpeg, "-v", "error", "-ss", str(start_seconds),
                   "-t", str(duration_seconds), "-i", avi_path, "-an",
                   "-vf", f"fps={fps}", "-f", "rawvideo", "-pix_fmt", "rgb24", "pipe:1"]
        process = subprocess.Popen(command, stdout=subprocess.PIPE)
        raw_size = width * height * 3
        records = bytearray()
        previous = None
        count = 0
        while True:
            raw = process.stdout.read(raw_size)
            if len(raw) != raw_size:
                break
            rgb = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
            colors = (((rgb[:, 0].astype(np.uint16) >> 3) << 11) |
                      ((rgb[:, 1].astype(np.uint16) >> 2) << 5) |
                      (rgb[:, 2].astype(np.uint16) >> 3))
            changed = (np.arange(colors.size, dtype=np.int32) if previous is None
                       else np.flatnonzero(colors != previous))
            runs = []
            if changed.size:
                split_points = np.flatnonzero(np.diff(changed) != 1) + 1
                for indices in np.split(changed, split_points):
                    start = int(indices[0])
                    length = int(indices.size)
                    runs.append((start, length,
                                 colors[start:start + length].astype('<u2').tobytes()))
            payload = bytearray(struct.pack("<I", len(runs)))
            for start, length, pixels in runs:
                payload.extend(struct.pack("<HH", start, length))
                payload.extend(pixels)
            records.extend(struct.pack("<I", len(payload)))
            records.extend(payload)
            previous = colors.copy()
            count += 1
        process.stdout.close()
        if process.wait() != 0 or count == 0:
            print("[ASSET EXTRACTOR] Failed to decode Mesen title reference")
            return bytearray(), bytearray()
        header = b"NBTITLE1" + struct.pack("<6I", 1, width, height, fps, 1, count)

        audio_command = [ffmpeg, "-v", "error", "-ss", str(start_seconds),
                         "-t", str(duration_seconds), "-i", avi_path, "-vn",
                         "-ac", "1", "-ar", "22050", "-acodec", "pcm_s16le",
                         "-f", "s16le", "pipe:1"]
        pcm = subprocess.run(audio_command, stdout=subprocess.PIPE, check=True).stdout
        wav_header = struct.pack('<4sI4s4sIHHIIHH4sI', b'RIFF', 36 + len(pcm),
                                 b'WAVE', b'fmt ', 16, 1, 1, 22050, 44100,
                                 2, 16, b'data', len(pcm))
        print(f"[ASSET EXTRACTOR] Encoded {count} cue-aligned title frames from Mesen")
        return header + records, wav_header + pcm

    title_video_bytes = bytearray()
    mesen_title_audio = bytearray()
    mesen_avi = os.path.join(os.environ.get("OneDrive", ""), "Documents", "Mesen2",
                             "Avi", "NBA Live 95 (USA).avi")
    if os.path.exists(mesen_avi):
        title_video_bytes, mesen_title_audio = encode_title_reference(mesen_avi)

    title_audio_bytes = mesen_title_audio
    for audio_path in (os.path.join(os.path.dirname(output_path), "post_ea_title_sequence.wav"),
                       os.path.join(os.path.dirname(rom_path), "post_ea_title_sequence.wav")):
        if len(title_audio_bytes) == 0 and os.path.exists(audio_path):
            with open(audio_path, "rb") as audio_file:
                title_audio_bytes = audio_file.read()
            print(f"[ASSET EXTRACTOR] Packed Mesen post-EA title audio: {len(title_audio_bytes)} bytes")
            break



        # ------------------------------------------------------------------
        # Game Setup screen ($80:A2BF cluster) graphics.
        #
        # The screen is SNES BG Mode 1 built from three layers whose tile data
        # the ROM produces by running its own decompressor ($80:C62B) and then
        # DMAing the result into VRAM. The pointer sequence was captured live
        # from the running ROM (tools/mesen_decomp_trace.lua):
        #
        #   $AE:A0AF, $AE:C446, $A6:C5FC -> BG2 chr   (blue gradient backdrop,
        #                                              NBA watermark, EA SPORTS)
        #   $AE:D153                     -> BG2 tilemap
        #   $AF:97AA, $AC:FD74           -> BG1 chr/tilemap (header banner)
        #   $97:FF6D, $AF:F2DC           -> BG3 text canvas + glyph tiles
        #   $AE:FA10 (CGADD $30, 0xA0)   -> palettes 3..7
        #   $A8:FFF1 (CGADD $49, 0x0E)   -> palette 4 tail
        #   $AF:9072 (CGADD $59, 0x0E)   -> palette 5 tail
        #
        # tools/build_setup_screen.py replays that sequence offline and
        # reproduces BG2's chr byte-for-byte. The remaining regions are built
        # by CPU writes that depend on accumulated decompressor state, so the
        # authoritative VRAM/CGRAM image is taken from a Mesen state capture
        # (tools/mesen_setup_capture.lua). Both are ROM data, and neither is
        # committed - .analysis/ and *.bin are ignored, same as the ROM itself.
        setup_vram_bytes = b""
        setup_cgram_bytes = b""
        capture_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                   "..", ".analysis", "setup_capture")
        vram_path = os.path.join(capture_dir, "vram.bin")
        cgram_path = os.path.join(capture_dir, "cgram.bin")
        if os.path.exists(vram_path) and os.path.exists(cgram_path):
            setup_vram_bytes = open(vram_path, "rb").read()
            setup_cgram_bytes = open(cgram_path, "rb").read()
            print(f"[ASSET EXTRACTOR] Game Setup VRAM {len(setup_vram_bytes)} bytes, "
                  f"CGRAM {len(setup_cgram_bytes)} bytes")

            # Cross-check the capture against data decompressed from the ROM.
            try:
                emu = Snes65816Decompressor(rom_data)
                emu.decompress(0xAE, 0xC446, 0x7F, 0x2000)
                blob = bytes(emu.wram[0x12000:0x12000 + 0x3C0])
                if blob and blob in setup_vram_bytes[0x2000:0x6000]:
                    print("[ASSET EXTRACTOR] Verified BG2 chr against ROM decompressor "
                          "($AE:C446 via $80:C62B)")
                else:
                    print("[ASSET EXTRACTOR] Warning: BG2 chr did not match the ROM decompressor")
            except Exception as ex:
                print(f"[ASSET EXTRACTOR] Warning: setup-screen ROM cross-check failed: {ex}")
        else:
            print("[ASSET EXTRACTOR] Game Setup capture missing; run:")
            print("    Mesen.exe <rom> tools/mesen_setup_capture.lua")

    assets = [
        (1, 128, 11, 0, nintendo_license_bytes),               # ASSET_NINTENDO_LICENSE
        (2, 256, num_legal_rows, start_y_legal, nba_legal_bytes), # ASSET_NBA_LEGAL_NOTICE (flags = start_y)
        (3, w4, h4, ea_flags, ea_packed[0]),                  # ASSET_EA_LOGO_STAGE1
        (4, w4, h4, ea_flags, ea_packed[1]),                  # ASSET_EA_LOGO_STAGE2
        (5, w4, h4, ea_flags, ea_packed[2]),                  # ASSET_EA_LOGO_STAGE3
        (6, w4, h4, ea_flags, ea_packed[3]),                  # ASSET_EA_LOGO_STAGE4
    ]

    if len(audio_intro_bytes) > 0:
        assets.append((7, 0, 0, 0, audio_intro_bytes))         # ASSET_AUDIO_EA_INTRO
    if len(audio_e_bytes) > 0:
        assets.append((8, 0, 0, 0, audio_e_bytes))             # ASSET_AUDIO_EA_E
    if len(audio_a_bytes) > 0:
        assets.append((9, 0, 0, 0, audio_a_bytes))             # ASSET_AUDIO_EA_A
    if len(audio_sports_bytes) > 0:
        assets.append((10, 0, 0, 0, audio_sports_bytes))       # ASSET_AUDIO_EA_SPORTS
    if len(audio_game_bytes) > 0:
        assets.append((11, 0, 0, 0, audio_game_bytes))         # ASSET_AUDIO_EA_GAME
    for frame_index, frame_bytes in enumerate(title_sequence_frames[:3]):
        assets.append((12 + frame_index, 256, 224, 0, frame_bytes))
    if len(title_audio_bytes) > 0:
        assets.append((15, 0, 0, 0, title_audio_bytes))
    if len(title_video_bytes) > 0:
        assets.append((68, 256, 224, 30, title_video_bytes))

    # Extract all other audio samples from ROM into asset pack for debugger
    rom_sample_offsets = [
        0x043025, 0x0DA71E, 0x0DBA2C, 0x0DF19E, 0x0E001C, 0x0E4A6F, 0x0E801C, 0x0EC8B6,
        0x0F001C, 0x0F482F, 0x10801C, 0x10D1E7, 0x114803, 0x11F769,
        0x124C14, 0x129604, 0x12A820, 0x12B964, 0x13001C, 0x1318E0, 0x1324F0,
        0x133394, 0x1350E8, 0x135BB4, 0x13BCE4, 0x13F850, 0x14001C, 0x1439E0, 0x145663,
        0x145FD9, 0x147C01, 0x149BA6, 0x14AD51, 0x14BEE4, 0x14E8BA, 0x14F0B6, 0x15001C,
        0x1507F4, 0x15102F, 0x151792, 0x151F4F, 0x1526BB, 0x152E03, 0x153C78, 0x15517D,
        0x155811, 0x159E18, 0x15D7E5, 0x16001C, 0x160F23, 0x161CBF, 0x166995
    ]

    if len(setup_vram_bytes) > 0:
        assets.append((16, 0, 0, 0, setup_vram_bytes))         # ASSET_SETUP_VRAM
    if len(setup_cgram_bytes) > 0:
        assets.append((17, 0, 0, 0, setup_cgram_bytes))        # ASSET_SETUP_CGRAM

    if os.path.exists(rom_path):
        extra_audio_id = 18
        for off in rom_sample_offsets:
            if off < len(rom_data):
                pcm, _ = decode_brr_to_pcm(rom_data[off:])
                if len(pcm) > 0:
                    wav_bytes = make_wav_bytes(pcm, sample_rate=16000)
                    assets.append((extra_audio_id, 0, 0, off, wav_bytes))
                    extra_audio_id += 1

    header_magic = b"NBA95PAK"
    version = 1
    asset_count = len(assets)
    entry_size = 24 # 6 * 4 bytes

    data_start = len(header_magic) + 4 + 4 + (asset_count * entry_size)

    entries = []
    current_offset = data_start
    data_blob = bytearray()

    for asset_id, width, height, flags, payload in assets:
        size = len(payload)
        entries.append((asset_id, current_offset, size, width, height, flags))
        data_blob.extend(payload)
        current_offset += size

    with open(output_path, "wb") as f:
        f.write(header_magic)
        f.write(struct.pack("<II", version, asset_count))
        for e in entries:
            f.write(struct.pack("<IIIIII", *e))
        f.write(data_blob)

    print(f"[ASSET EXTRACTOR] Successfully generated asset pack: {output_path} ({os.path.getsize(output_path)} bytes, {asset_count} assets)")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="NBA Live 95 Asset Extractor")
    parser.add_argument("--rom", required=True, help="Path to SNES ROM (.sfc/.smc)")
    parser.add_argument("--output", default=r"build\nba95_assets.pak", help="Output asset package path")
    args = parser.parse_args()

    create_asset_pack(args.rom, args.output)
