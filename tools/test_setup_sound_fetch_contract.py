"""Fetch-order/width checks independent of captured native data effects."""
import copy,unittest
from setup_sound_fetch_contract import validate_65816_fetches
from setup_spc_fetch_contract import validate_spc_fetches
class FetchContracts(unittest.TestCase):
 def cpu(self,op,ps,count,slots=None):
  rom=bytearray(0x8000);rom[0x100:0x104]=bytes([op,0x12,0x34,0x56]);pc=0x808100
  ins=[dict(pc=pc,ps=ps,cycle=1)];slots=slots or list(range(count));rows=[dict(access=2,address=0,value=0)for _ in range(max(slots)+1)]
  for i,slot in enumerate(slots):rows[slot]=dict(access=0,address=pc+i,value=rom[0x100+i])
  return rom,ins,rows
 def test_accumulator_width(self):
  for ps,n in ((0x20,2),(0,3)):
   rom,ins,rows=self.cpu(0xa9,ps,n);self.assertEqual(validate_65816_fetches(ins,rows,rom),[])
   ins[0]['ps']^=0x20
   with self.assertRaises(ValueError):validate_65816_fetches(ins,rows,rom)
 def test_index_width(self):
  for ps,n in ((0x10,2),(0,3)):
   rom,ins,rows=self.cpu(0xa2,ps,n);self.assertEqual(validate_65816_fetches(ins,rows,rom),[])
   ins[0]['ps']^=0x10
   with self.assertRaises(ValueError):validate_65816_fetches(ins,rows,rom)
 def test_jsl_bank_slot(self):
  rom,ins,rows=self.cpu(0x22,0x30,4,[0,1,2,5]);self.assertEqual(validate_65816_fetches(ins,rows,rom),[])
  rows[4],rows[5]=rows[5],rows[4]
  with self.assertRaises(ValueError):validate_65816_fetches(ins,rows,rom)
 def test_fetch_byte_and_address(self):
  for field in ('value','address'):
   rom,ins,rows=self.cpu(0xad,0x30,3);rows[0][field]^=1
   with self.assertRaises(ValueError):validate_65816_fetches(ins,rows,rom)
 def test_idle_shape(self):
  for field in ('value','address'):
   rom,ins,rows=self.cpu(0x22,0x30,4,[0,1,2,5]);rows[3][field]=1
   with self.assertRaises(ValueError):validate_65816_fetches(ins,rows,rom)
 def spc(self):
  rom=bytearray(0x5000);pc=0x453;off=0x4687+pc-0x380;rom[off:off+3]=bytes([0x2e,0xf4,0xf4]);rows=[dict(kind='cycle',bus=4,address=0,value=0)for _ in range(5)]
  for slot,byte in ((0,0),(1,1),(4,2)):rows[slot]=dict(kind='cycle',bus=1,address=pc+byte,value=rom[off+byte])
  rows[2]=dict(kind='cycle',bus=2,address=0xf4,value=5)
  return rom,[dict(pc=pc,cycles=0)],rows
 def test_cbne_relative_fetch_slot(self):
  rom,ins,rows=self.spc();validate_spc_fetches(ins,rows,rom);rows[3],rows[4]=rows[4],rows[3]
  with self.assertRaises(ValueError):validate_spc_fetches(ins,rows,rom)
 def test_spc_fetch_and_idle_fields(self):
  for slot,field in ((0,'address'),(0,'value'),(3,'address'),(3,'value')):
   rom,ins,rows=self.spc();rows[slot][field]^=1
   with self.assertRaises(ValueError):validate_spc_fetches(ins,rows,rom)
if __name__=='__main__':unittest.main()
