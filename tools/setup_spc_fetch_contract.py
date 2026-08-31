"""Validate exposed resident fetch/idle requests against uploaded ROM bytes."""
LENGTH={0xc4:2,0xe4:2,0xd0:2,0x8f:3,0x3f:3,0xf0:2,0x00:1,0x2e:3,
        0x1c:1,0x5d:1,0x1f:3,0x9f:1,0x60:1,0x88:2,0xfd:1,0xe8:2,0xcb:2}
def validate_spc_fetches(instructions,rows,rom):
    cycles=[r for r in rows if r['kind']=='cycle']
    def check(ok,msg):
        if not ok:raise ValueError(msg)
    for i,ins in enumerate(instructions):
        pc=ins['pc'];check(0x380<=pc<0x870,'SPC fetch source outside upload')
        offset=0x4687+pc-0x380;op=rom[offset]
        check(op in LENGTH,'SPC fetch opcode outside bounded contract')
        length=LENGTH[op]
        # CBNE reads the DP operand's data, then idles, before fetching rel8.
        slots=[0,1,4]if op==0x2e else list(range(length))
        start=ins['cycles'];stop=instructions[i+1]['cycles']if i+1<len(instructions)else len(cycles)
        group=cycles[start:stop];check(len(group)>slots[-1],'SPC fetch sequence incomplete')
        fetch=dict(zip(slots,range(length)))
        for j,r in enumerate(group):
            if j in fetch:
                byte=fetch[j]
                check(r['bus']==1 and r['address']==pc+byte and r['value']==rom[offset+byte],f'SPC opcode/operand fetch differs at {pc:04X}+{byte}')
            else:
                check(r['bus']!=1,'SPC unexpected fetch slot')
                if r['bus']==4:check(r['address']==0 and r['value']==0,'SPC noncanonical idle request')
