"""Observation protocol only: no prediction of DMA, interrupt, or refresh time.

Validate native order before any composition/sorting can hide a malformed row.
Mesen reports an instruction's final data access at the following instruction's
entry clock, so association deliberately includes both interval endpoints.
"""
from bisect import bisect_right
from verify_setup_scheduler import require


def validate_chronology(rows, label, strict=False):
    require(bool(rows), label + ': empty observation')
    for index, (previous, current) in enumerate(zip(rows, rows[1:]), 1):
        for clock in ('cpu_cycles', 'master_clock'):
            delta = current[clock] - previous[clock]
            require(delta > 0 if strict else delta >= 0,
                    f'{label}: {clock} chronology differs at {index}')


def validate_native_scope(instructions, bus, start, end, label):
    """Check a full observed source scope, including observed NMI entry hooks.

    Caller owns schema validation and supplies original-order lists. A composed
    instruction list is permitted only after each original list was validated.
    Observational NMI entry->resume gaps remain gaps; this does not certify all
    native CPU reads or interrupt work. Positive stack checks belong to callers.
    """
    validate_chronology(instructions, label + ' instructions', strict=True)
    validate_chronology(bus, label + ' bus')
    require(all(instructions[0][key] == start[key] for key in
                ('pc', 'cpu_cycles', 'master_clock', 'a', 'x', 'y', 'ps', 'db', 'dp', 'sp')),
            label + ': first instruction differs from scope entry')
    for clock in ('cpu_cycles', 'master_clock'):
        require(start[clock] < end[clock], label + ': scope clocks reversed')
        require(instructions[-1][clock] < end[clock], label + ': instruction outside scope')
        require(start[clock] <= bus[0][clock] <= bus[-1][clock] <= end[clock],
                label + ': bus outside scope')
    entries = instructions + [end]
    masters = [r['master_clock'] for r in entries]
    for index, row in enumerate(bus):
        position = bisect_right(masters, row['master_clock']) - 1
        candidates = (position, position - 1)
        require(any(0 <= p < len(instructions) and row['pc'] == entries[p]['pc'] and
                    all(entries[p][clock] <= row[clock] <= entries[p + 1][clock]
                        for clock in ('cpu_cycles', 'master_clock')) for p in candidates),
                f'{label}: bus PC/clock has no source instruction association at {index}')
