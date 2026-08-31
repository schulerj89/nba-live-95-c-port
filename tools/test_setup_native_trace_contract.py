"""Native observation chronology, endpoint convention, and PC association tests."""
import copy
import unittest
from setup_native_trace_contract import validate_native_scope


class NativeTraceTests(unittest.TestCase):
    def setUp(self):
        regs = dict(a=0, x=0, y=0, ps=0, db=128, dp=0, sp=8182)
        self.instructions = [dict(regs, pc=0x808a0a, cpu_cycles=100, master_clock=1000),
                             dict(regs, pc=0x808a0d, cpu_cycles=104, master_clock=1024),
                             dict(regs, pc=0x808a0f, cpu_cycles=107, master_clock=1200)]
        self.start = self.instructions[0].copy()
        self.end = dict(regs, pc=0x808a11, cpu_cycles=110, master_clock=1218)
        self.bus = [dict(pc=0x808a0a, cpu_cycles=104, master_clock=1024),
                    dict(pc=0x808a0d, cpu_cycles=106, master_clock=1050),
                    dict(pc=0x808a0d, cpu_cycles=106, master_clock=1054),
                    dict(pc=0x808a0f, cpu_cycles=110, master_clock=1218)]

    def check(self):
        validate_native_scope(self.instructions, self.bus, self.start, self.end, 'test')

    def test_accepts_final_bus_at_next_entry_and_end(self):
        self.check()

    def test_accepts_same_clock_induced_bus_effect(self):
        self.bus.insert(1, self.bus[0].copy())
        self.check()

    def test_no_dma_duration_assumption(self):
        self.instructions[-1]['master_clock'] += 800
        self.end['master_clock'] += 800
        self.bus[-1]['master_clock'] += 800
        self.check()

    def test_rejects_each_clock_reversal(self):
        for kind in ('instructions', 'bus'):
            for clock in ('cpu_cycles', 'master_clock'):
                with self.subTest(kind=kind, clock=clock):
                    saved = copy.deepcopy(getattr(self, kind))
                    getattr(self, kind)[1][clock] = getattr(self, kind)[0][clock] - 1
                    with self.assertRaises(ValueError): self.check()
                    setattr(self, kind, saved)

    def test_rejects_equal_instruction_clock(self):
        for clock in ('cpu_cycles', 'master_clock'):
            saved = self.instructions[1][clock]
            self.instructions[1][clock] = self.instructions[0][clock]
            with self.assertRaises(ValueError): self.check()
            self.instructions[1][clock] = saved

    def test_rejects_pc_moved_to_different_instruction_interval(self):
        self.bus[1]['pc'] = self.instructions[0]['pc']
        with self.assertRaises(ValueError): self.check()

    def test_rejects_each_clock_outside_scope(self):
        for clock in ('cpu_cycles', 'master_clock'):
            saved = copy.deepcopy(self.bus)
            self.bus[0][clock] = self.start[clock] - 1
            with self.assertRaises(ValueError): self.check()
            self.bus = copy.deepcopy(saved)
            self.bus[-1][clock] = self.end[clock] + 1
            with self.assertRaises(ValueError): self.check()
            self.bus = saved

    def test_rejects_clock_pair_from_different_instructions(self):
        self.bus[1]['cpu_cycles'] = 107
        self.bus[2]['cpu_cycles'] = 107
        self.bus[1]['master_clock'] = 1201
        self.bus[2]['master_clock'] = 1205
        with self.assertRaises(ValueError): self.check()

    def test_rejects_entry_register_mismatch(self):
        self.start['a'] = 1
        with self.assertRaises(ValueError): self.check()

    def test_rejects_instruction_at_terminal_boundary(self):
        self.instructions[-1]['cpu_cycles'] = self.end['cpu_cycles']
        with self.assertRaises(ValueError): self.check()


if __name__ == '__main__': unittest.main(verbosity=2)
