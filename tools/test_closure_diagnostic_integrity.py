"""Test the bounded change-attribution protocol, not gameplay or ROM parity."""
from pathlib import Path
import tempfile
import unittest

from audit_closure_diagnostics import compare_custom_session_buffers,equal_files


class ClosureProtocolTests(unittest.TestCase):
    def setUp(self):
        # Synthetic protocol packets; no C/game expectation is derived here.
        self.old=bytearray((b'\x10\x20\x01\x00\x30\x40')*6000)
        self.new=bytearray((b'\x10\x20\x02\x00\x30\x40')*6000)

    def test_exact_prescribed_word_change(self):
        compare_custom_session_buffers(self.old,self.new,6,2)

    def test_any_nonstyle_byte_change_fails_at_initial_middle_and_final_record(self):
        for frame in (0,3000,5999):
            for offset in (0,1,4,5):
                changed=self.new[:];changed[frame*6+offset]^=1
                with self.subTest(frame=frame,offset=offset),self.assertRaises(ValueError):
                    compare_custom_session_buffers(self.old,changed,6,2)

    def test_style_is_required_exact_transition_not_globally_ignored(self):
        for frame in (0,3000,5999):
            for value in (0,1,3,0x102):
                changed=self.new[:];changed[frame*6+2:frame*6+4]=value.to_bytes(2,'little')
                with self.subTest(frame=frame,value=value),self.assertRaises(ValueError):
                    compare_custom_session_buffers(self.old,changed,6,2)
        for value in (0,2,0x101):
            changed=self.old[:];changed[2:4]=value.to_bytes(2,'little')
            with self.subTest(old=value),self.assertRaises(ValueError):
                compare_custom_session_buffers(changed,self.new,6,2)

    def test_population_and_explicit_offset_are_checked(self):
        for old,new,width,offset in ((self.old[:-1],self.new,6,2),
                (self.old,self.new[:-6],6,2),(self.old,self.new,6,0),
                (self.old,self.new,True,2),(self.old,self.new,6,True),
                (self.old,self.new,6,-1),(self.old,self.new,6,5)):
            with self.subTest(width=width,offset=offset),self.assertRaises(ValueError):
                compare_custom_session_buffers(old,new,width,offset)

    def test_direct_file_comparison_checks_contents_and_lengths(self):
        with tempfile.TemporaryDirectory() as temp:
            a=Path(temp)/'a';b=Path(temp)/'b'
            a.write_bytes(self.old);b.write_bytes(self.old)
            self.assertTrue(equal_files(a,b))
            b.write_bytes(self.new);self.assertFalse(equal_files(a,b))
            b.write_bytes(self.old[:-1]);self.assertFalse(equal_files(a,b))


if __name__=='__main__':unittest.main()
