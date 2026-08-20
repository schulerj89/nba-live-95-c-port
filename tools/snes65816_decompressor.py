"""Minimal 65816 interpreter used for the NBA Live 95 decompressor."""

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
