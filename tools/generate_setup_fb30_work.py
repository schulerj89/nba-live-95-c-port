"""Statically translate the bounded original FB30 source to C continuations.

This build-time source tool has no capture/trace input. The emitted C has no
opcode decoder or opcode dispatcher. Explicit native jump tables select only
compiled source labels. The generated program is reviewed/versioned source;
production never runs this script or executes ROM code.
"""
import argparse
from collections import deque
import hashlib
from pathlib import Path
import sys

ROM_SHA = '2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870'
DECODER_SHA = 'b1864664d3ac0abcd439055e88bf7220cf66be7a6ae9dfc5d59b3186f1469a46'
TABLES = {0xbfb1: range(1, 9), 0xbfe5: range(10), 0xc032: range(10),
          0xc080: range(10), 0xc0cf: range(10), 0xc11f: range(10),
          0xc170: range(10), 0xc1c2: range(10), 0xc215: range(10),
          0xc3ac: range(1, 9)}
BRANCH = {'BEQ': '(P & 2u) != 0', 'BNE': '(P & 2u) == 0',
          'BMI': '(P & 0x80u) != 0', 'BPL': '(P & 0x80u) == 0',
          'BCC': '(P & 1u) == 0', 'BCS': '(P & 1u) != 0', 'BRA': 'true'}


def in_scope(pc):
    return (0xbe6b <= pc <= 0xc5aa or 0xc62b <= pc <= 0xc682 or
            0x86da <= pc <= 0x86ec)


def label(state):
    return 'source_%04x_m%dx%d' % state


def translate(rom, decoder):
    def word(address): return int.from_bytes(rom[address & 0x7fff:(address & 0x7fff) + 2], 'little')
    jumps = {table: [word(table + 2 * index) for index in indexes] for table, indexes in TABLES.items()}
    jumps[0x0560] = [word(0xbf91 + 2 * index) for index in range(8, 16)]
    todo, decoded, edges, returns = deque([(0xc62b, 0, 0)]), {}, {}, set()
    while todo:
        state = todo.popleft()
        if state in decoded: continue
        pc, m, x = state
        if not in_scope(pc): continue
        ins = decoder.decode_insn(rom, pc & 0x7fff, pc, 0x80, m, x)
        ins.m_flag, ins.x_flag = m, x
        decoded[state] = ins
        post = (m, x)
        if ins.mnem in ('REP', 'SEP'):
            flag = int(ins.mnem == 'SEP')
            post = (flag if ins.operand & 32 else m, flag if ins.operand & 16 else x)
        next_state = (pc + ins.length, *post)
        successors = []
        if pc == 0xc682 or ins.mnem == 'RTS': pass
        elif ins.mnem == 'RTL':
            successors = [(0xc633, 0, 0)] if pc == 0x86e7 else []
        elif ins.mnem in ('JSR', 'JSL'):
            target = ins.operand & 65535
            if target in (0x86da, 0xbe6b, 0xc468, 0xc4ea, 0xc594):
                after = (0, 0) if target == 0x86da else (0, 1)
                successors = [(target, m, x), (pc + ins.length, *after)]
                if ins.mnem == 'JSR': returns.add((pc + ins.length, *after))
            # Other formats are explicit unsupported exits, not interpreted.
        elif ins.mnem in BRANCH:
            successors = [(ins.operand, *post)]
            if ins.mnem != 'BRA': successors.append(next_state)
            if pc == 0x86e4: successors = [next_state] # nonempty queue out of scope
        elif ins.mnem == 'JMP':
            if ins.mode == decoder.ABS: successors = [(ins.operand, *post)]
            else:
                if ins.operand not in jumps: raise ValueError(f'undeclared source jump table {pc:04x}')
                successors = [(target, *post) for target in jumps[ins.operand]]
        else: successors = [next_state]
        edges[state] = successors
        todo.extend(s for s in successors if in_scope(s[0]))

    def go(state): return 'goto ' + label(state) + ';' if state in decoded else 'return unsupported(s);'
    def address(ins):
        operand = ins.operand
        if ins.mode == decoder.DP: return f'0x{operand:02x}u', 'false'
        if ins.mode == decoder.ABS: return f'DB(0x{operand:04x}u)', 'false'
        if ins.mode == decoder.ABS_X: return f'DB(0x{operand:04x}u + X)', f'index_idle(s, 0x{operand:04x}u, X)'
        if ins.mode == decoder.ABS_Y: return f'DB(0x{operand:04x}u + Y)', f'index_idle(s, 0x{operand:04x}u, Y)'
        if ins.mode == decoder.LONG: return f'0x{operand:06x}u', 'false'
        if ins.mode == decoder.LONG_X: return f'(0x{operand:06x}u + X)', 'false'
        raise ValueError(f'unsupported memory addressing: {ins}')

    lines = ['/* Generated from canonical ROM only by tools/generate_setup_fb30_work.py.',
             ' * No capture, fitted cost, opcode dispatcher or runtime ROM execution.',
             ' * Each label is static C source flow; SUSPEND labels are C continuations. */',
             'static bool advance(NbaSetupFb30Work *s)', '{', '    switch (B.resume) {',
             '    case 0: goto source_c62b_m0x0;']
    for state, ins in sorted(decoded.items()):
        pc, m, x = state
        width, index_width = 1 if m else 2, 1 if x else 2
        nxt = (pc + ins.length, m, x)
        body = []
        if pc == 0xc682:
            body = ['B.output_size = X;', 'B.status = NBA_CODEC_WORK_DONE;', 'return false;']
        elif ins.mnem in ('LDA', 'LDX', 'LDY', 'ADC', 'SBC', 'CMP', 'CPX', 'BIT', 'ORA', 'AND'):
            load_width = index_width if ins.mnem in ('LDX', 'LDY', 'CPX') else width
            if ins.mode == decoder.IMM:
                body.append(f'IMM(0x{pc:04x}, {ins.length});')
                operand = f'0x{ins.operand:04x}u'
            elif ins.mode == decoder.DP_INDIR:
                body.append(f'INDIRECT(0x{pc:04x}, 0x{ins.operand:02x}, {load_width});')
                operand = 'B.read_value'
            else:
                addr, indexed = address(ins)
                body.append(f'READ(0x{pc:04x}, {ins.length}, {addr}, {load_width}, {indexed});')
                operand = 'B.read_value'
            if ins.mnem.startswith('LD'):
                body.append(f'set_{ins.mnem[-1].lower()}(s, {operand});')
            elif ins.mnem in ('ADC', 'SBC'):
                body.append(f'add_sub(s, {operand}, {str(ins.mnem == "SBC").lower()});')
            elif ins.mnem in ('CMP', 'CPX'):
                body.append(f'compare(s, {"X" if ins.mnem == "CPX" else "A"}, {operand}, {load_width});')
            elif ins.mnem == 'BIT': body.append(f'bit_test(s, {operand});')
            elif ins.mnem == 'ORA': body.append(f'set_a(s, A | {operand});')
            elif ins.mnem == 'AND': body.append(f'set_a(s, A & {operand});')
        elif ins.mnem in ('STA', 'STX', 'STY', 'STZ'):
            addr, indexed = address(ins)
            if ins.mode in (decoder.ABS_X, decoder.ABS_Y): indexed = 'true'
            w = index_width if ins.mnem in ('STX', 'STY') else width
            value = {'STA':'A','STX':'X','STY':'Y','STZ':'0'}[ins.mnem]
            body.append(f'WRITE(0x{pc:04x}, {ins.length}, {addr}, {w}, {indexed}, {value});')
        elif ins.mnem in ('ASL', 'LSR', 'ROL', 'INC', 'DEC'):
            if ins.mode in (decoder.ACC, decoder.IMP):
                body += [f'IMP(0x{pc:04x});', f'accumulator_change(s, FB30_{ins.mnem});']
            else:
                addr, indexed = address(ins)
                if ins.mode in (decoder.ABS_X, decoder.ABS_Y): indexed = 'true'
                body.append(f'CHANGE(0x{pc:04x}, {ins.length}, {addr}, {width}, {indexed}, FB30_{ins.mnem});')
        elif ins.mnem in ('INY','DEY','INX','DEX'):
            reg = ins.mnem[-1]
            body += [f'IMP(0x{pc:04x});', f'set_{reg.lower()}(s, {reg} {"+" if ins.mnem[0] == "I" else "-"} 1u);']
        elif ins.mnem in ('TAX','TAY','TXA','TYA','TXY','TYX'):
            body += [f'IMP(0x{pc:04x});', f'set_{ins.mnem[2].lower()}(s, {ins.mnem[1]});']
        elif ins.mnem in ('REP','SEP'):
            body += [f'MODE(0x{pc:04x});', f'status_set(s, P {"|" if ins.mnem == "SEP" else "&"} 0x{ins.operand if ins.mnem == "SEP" else 255 ^ ins.operand:02x}u);']
            nxt = edges[state][0]
        elif ins.mnem in ('CLC','SEC'):
            body += [f'IMP(0x{pc:04x});', f'P = (uint8_t)(P {"& 0xfeu" if ins.mnem == "CLC" else "| 1u"});']
        elif ins.mnem == 'XBA':
            body += [f'SUSPEND(implied(s, 0x{pc:04x}, 1, 2));', 'A = (uint16_t)((A << 8) | (A >> 8));', 'nz(s, A, 1);']
        elif ins.mnem in ('PHA','PHX','PHB','PHP'):
            value, w = {'PHA':('A',width),'PHX':('X',index_width),
                        'PHB':('B.registers.data_bank',1),'PHP':('P',1)}[ins.mnem]
            body.append(f'PUSH(0x{pc:04x}, 1, {value}, {w});')
        elif ins.mnem in ('PLA','PLX','PLB','PLP'):
            w = width if ins.mnem == 'PLA' else index_width if ins.mnem == 'PLX' else 1
            body.append(f'PULL(0x{pc:04x}, {w}, false);')
            if ins.mnem in ('PLA','PLX'): body.append(f'set_{ins.mnem[-1].lower()}(s, B.read_value);')
            elif ins.mnem == 'PLP': body.append('status_set(s, (uint8_t)B.read_value);')
            else: body += ['B.registers.data_bank = (uint8_t)B.read_value;', 'nz(s, B.registers.data_bank, 1);']
        elif ins.mnem in BRANCH:
            body.append(f'BRANCH(0x{pc:04x}, {BRANCH[ins.mnem]});')
            target = (ins.operand, m, x)
            if pc == 0x86e4: body.append('if (B.branch) return unsupported(s);')
            elif ins.mnem == 'BRA': body.append(go(target)); nxt = None
            else: body.append('if (B.branch) { ' + go(target) + ' }')
        elif ins.mnem == 'JMP':
            if ins.mode == decoder.ABS:
                body += [f'JUMP(0x{pc:04x});', go((ins.operand, m, x))]
            else:
                addr = f'(0x800000u | (uint16_t)(0x{ins.operand:04x}u + X))' if ins.mode == decoder.INDIR_X else f'0x{ins.operand:04x}u'
                body.append(f'READ(0x{pc:04x}, 3, {addr}, 2, {str(ins.mode == decoder.INDIR_X).lower()});')
                body.append('switch (B.read_value) {')
                body += [f'case 0x{target:04x}: ' + go((target,m,x)) for target in sorted(set(jumps[ins.operand]))]
                body += ['default: return unsupported(s);', '}']
            nxt = None
        elif ins.mnem in ('JSR','JSL'):
            if not edges[state]: body = ['return unsupported(s);']
            else:
                if ins.mnem == 'JSR': body.append(f'PUSH(0x{pc:04x}, 3, 0x{pc+2:04x}, 2);')
                else: body += ['SUSPEND(jsl_queue(s));', 'B.registers.stack_pointer -= 3u;']
                body.append(go(edges[state][0]))
            nxt = None
        elif ins.mnem == 'RTS':
            body += [f'PULL(0x{pc:04x}, 2, true);', 'switch ((uint16_t)(B.read_value + 1u)) {']
            body += [f'case 0x{target[0]:04x}: ' + go(target) for target in sorted(returns)]
            body += ['default: return unsupported(s);', '}']; nxt = None
        elif ins.mnem == 'RTL' and pc != 0x86e7:
            body = ['return unsupported(s);']; nxt = None
        elif ins.mnem == 'RTL':
            body += [f'PULL(0x{pc:04x}, 3, false);',
                     'if (B.read_value != 0xc632 || s->read_bank != 0x80) return unsupported(s);',
                     go((0xc633,0,0))]; nxt = None
        else: raise ValueError(f'unsupported source operation {ins}')
        if nxt is not None and pc != 0xc682: body.append(go(nxt))
        lines.append(label(state) + ': /* ' + str(ins).strip() + ' */')
        lines.extend('        ' + line for line in body)
    lines += ['    default: return unsupported(s);', '    }', '}', '']
    return '\n'.join(lines), decoded


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--decoder-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    if hashlib.sha256(rom).hexdigest() != ROM_SHA: raise ValueError('canonical original ROM required')
    decoder_path = args.decoder_root.resolve() / 'snes65816.py'
    if hashlib.sha256(decoder_path.read_bytes()).hexdigest() != DECODER_SHA:
        raise ValueError('audited static source decoder required')
    sys.path.insert(0, str(args.decoder_root.resolve()))
    import snes65816 as decoder
    result, decoded = translate(rom, decoder)
    if args.check:
        if args.output.read_text() != result: raise ValueError('generated source differs')
    else: args.output.write_text(result)
    print(f'PASS: {len(decoded)} static ROM source states; no capture input')


if __name__ == '__main__':
    main()
