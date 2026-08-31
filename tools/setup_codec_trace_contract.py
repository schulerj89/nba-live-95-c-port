"""Strict bounded codec producer trace protocol; no observed-time fitting."""
from verify_setup_scheduler import require, exact_keys, integer


def validate_source_events(rows, report):
    instructions, writes = [], []
    previous_cycle = 0
    for row in rows:
        if row.get('kind') == 'instruction':
            schema = dict(cycle=2**63-1, master=2**63-1, pc=0xffffff,
                          a=65535, x=65535, y=65535, sp=65535, db=255, ps=255)
            instructions.append(row)
        else:
            require(row.get('kind') == 'write', 'unknown C source event')
            schema = dict(cycle=2**63-1, pc=0xffffff, address=0xffffff, value=255)
            writes.append(row)
        exact_keys(row, {'kind', *schema}, 'C source event')
        for key, limit in schema.items():
            integer(row[key], 0, limit, 'C source ' + key)
        require(previous_cycle < row['cycle'] <= report['cycles'],
                'mixed C source events are not strictly chronological')
        previous_cycle = row['cycle']
        require(row['pc'] >> 16 == 0x80, 'C source PC outside compiled bank')
        if row['kind'] == 'write':
            require(instructions and row['pc'] == instructions[-1]['pc'],
                    'C write detached from its current instruction')
    require(instructions and instructions[0]['cycle'] == 1 and instructions[0]['master'] == 0,
            'C source work must begin at cycle one/master zero')
    require(len(instructions) == report['instructions'], 'C instruction count differs')
    following = instructions[1:] + [dict(cycle=report['cycles'] + 1, master=report['master'])]
    for current, after in zip(instructions, following):
        cpu = after['cycle'] - current['cycle']
        master = after['master'] - current['master']
        # These two bounded codecs use only six/eight-clock bus accesses and
        # six-clock idles. Their longest recipe fits ten CPU cycles. This range
        # cannot admit a fabricated +/-40-clock refresh shift between events.
        require(1 <= cpu <= 10 and 6 * cpu <= master <= 8 * cpu and master % 2 == 0,
                'C instruction intrinsic duration outside bounded bus domain')
    return instructions, writes
