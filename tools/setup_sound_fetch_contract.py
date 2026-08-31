"""Exact fetch bytes/slots for bounded sound source; not CPU execution."""
from verify_setup_scheduler import require

# Encoded instruction lengths for the opcodes in the two frozen sound slices.
# Immediate A/index widths come from the instruction-entry status, not a
# preceding disassembly's guessed width. JSL's bank fetch follows its stack
# bank write and idle cycle (zero-based bus slot5).
FIXED={0x08:1,0xe2:2,0xaf:4,0xf0:2,0x4c:3,0xd0:2,0x28:1,0x6b:1,
       0x22:4,0xa5:2,0xad:3,0x8d:3,0xce:3,0x20:3,0x80:2,0xc2:2,
       0x48:1,0xab:1,0x60:1,0x30:2,0x9b:1,0x84:2,0x8e:3,0xb9:3,
       0x90:2,0xbd:3,0x85:2,0xfe:3,0xa4:2,0xe8:1,0xc8:1,0xb0:2,
       0x8b:1,0x68:1,0x4b:1,0xc6:2,0xa6:2,0x86:2,0x9e:3,0xca:1,
       0x10:2,0x9c:3,0x1a:1,0x99:3}
A_IMMEDIATE={0xa9,0xc9};X_IMMEDIATE={0xa2,0xc0,0xa0}

def validate_65816_fetches(instructions,bus,rom):
    effects=[]
    for i,ins in enumerate(instructions):
        pc=ins['pc'];offset=pc&0x7fff
        require(pc>>16==0x80,'fetch source bank outside scope')
        op=rom[offset]
        if op in A_IMMEDIATE:length=2 if ins['ps']&0x20 else 3
        elif op in X_IMMEDIATE:length=2 if ins['ps']&0x10 else 3
        else:
            require(op in FIXED,'fetch opcode outside bounded sound contract')
            length=FIXED[op]
        positions=[0,1,2,5]if op==0x22 else list(range(length))
        start=ins['cycle']-1
        stop=instructions[i+1]['cycle']-1 if i+1<len(instructions)else len(bus)
        rows=bus[start:stop]
        require(len(rows)>positions[-1],'incomplete opcode/operand fetch sequence')
        fetches=dict(zip(positions,range(length)))
        for slot,row in enumerate(rows):
            if slot in fetches:
                byte=fetches[slot]
                require(row['access']==0 and row['address']==pc+byte and
                        row['value']==rom[offset+byte],
                        f'canonical opcode/operand fetch differs at {pc:06X}+{byte}')
            elif row['access']==2:
                require(row['address']==0 and row['value']==0,'noncanonical idle request')
            else:
                require(not(row['access']==0 and pc<=row['address']<pc+4),
                        'unexpected fetch inside source instruction window')
                effects.append(row)
    return effects
