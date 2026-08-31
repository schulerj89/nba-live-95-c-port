"""NBPDRAW1: literal original head-order and jersey-direction table bytes."""
import hashlib,struct
ROM_SHA='2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
HEAD_SHA='c541203f3fedecf112bc992f79382f2a0c9b8e70baae0280e78cb2324fc32b97'
NUMBER_SHA='38a9c6ace59fd82da80e6d339813878bfb213aa6efa9242349bbdbb4443ca42e'
def sha(raw):return hashlib.sha256(raw).hexdigest()
def build(rom):
 if len(rom)!=1572864 or sha(rom)!=ROM_SHA:raise ValueError('canonical unheadered NBA Live95 ROM required')
 def table(pc,size):
  start=((pc>>16)&127)*32768+(pc&32767);return rom[start:start+size]
 head=table(0xacb6b3,2096);number=table(0x87a98e,16)
 if sha(head)!=HEAD_SHA or sha(number)!=NUMBER_SHA:raise ValueError('original draw table identity mismatch')
 return struct.pack('<8s6I',b'NBPDRAW1',1,2096,32,8,2128,2144)+head+number
