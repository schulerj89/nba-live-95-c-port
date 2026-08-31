"""Attribute observed backdrop NMI work; does not predict production timing."""
import argparse
from collections import Counter
import json
from pathlib import Path

from verify_setup_scheduler import (check_previous, digest, pairs,
                                    read_capture, read_json, require)


def jsonlines(path, counter):
    rows = [json.loads(line, object_pairs_hook=pairs) for line in path.read_text().splitlines()]
    require([row[counter] for row in rows] == list(range(len(rows))), 'missing/reordered ' + str(path))
    require(all(a['master_clock'] <= b['master_clock'] for a, b in zip(rows, rows[1:])), 'clock order')
    return rows


def analyze(native, scheduler, previous, rom):
    rows = read_capture(native, rom)
    preserved = check_previous(rows, read_capture(scheduler, rom))
    manifest = read_json(native / 'manifest.json')
    instructions = jsonlines(native / 'interrupt_instructions.jsonl', 'instruction')
    boundaries = jsonlines(native / 'interrupt_boundaries.jsonl', 'event')
    bus = jsonlines(native / 'interrupt_bus.jsonl', 'event')
    require(len({(r['nmi'], r['master_clock'], r['pc']) for r in instructions}) == len(instructions),
            'duplicate instruction observation')
    require(manifest['interrupt_summary'] ==
            f'ok; scopes=4; nmi=46; instructions={len(instructions)}; bus={len(bus)}\n',
            'interrupt completion/count mismatch')
    native_rom = rom.read_bytes()
    require(native_rom[0x7FEA:0x7FEC] == bytes.fromhex('5681') and
            native_rom[0x156:0x15A] == bytes.fromhex('5c5a8180'), 'NMI vector ownership changed')
    require(native_rom[0x59B] == 0x40, 'RTI ownership changed')
    per_nmi = {}
    constructors = {}
    for row in boundaries:
        if row['tag'] in ('backdrop.entry', 'header.entry'):
            constructors.setdefault(row['scope'], {})[row['tag']] = row
        elif row['tag'] not in ('rules.constructor', 'main.constructor'):
            group = per_nmi.setdefault(row['nmi'], {})
            require(row['tag'] not in group, 'duplicate NMI stage boundary')
            group[row['tag']] = row
    required = {'nmi.entry', 'nmi.exit', 'epoch.after_guard', 'controller.before_call',
                'controller.after_call', 'controller.entry', 'controller.after_busy_loop',
                'callback.before_call', 'callback.after_call', 'audio.before_call',
                'audio.entry', 'audio.after_call'}
    require(set(per_nmi) == set(range(1, 47)) and all(set(v) == required for v in per_nmi.values()),
            '46 complete NMI stage observations required')
    ranges = [('entry_to_epoch', 'nmi.entry', 'epoch.after_guard'),
              ('controller', 'controller.before_call', 'controller.after_call'),
              ('controller_busy_prefix', 'controller.entry', 'controller.after_busy_loop'),
              ('callback', 'callback.before_call', 'callback.after_call'),
              ('audio', 'audio.before_call', 'audio.after_call'),
              ('total', 'nmi.entry', 'nmi.exit')]
    details = []
    for number, events in per_nmi.items():
        group = [r for r in instructions if r['nmi'] == number]
        require(group and group[0]['pc'] == 0x80815A and group[-1]['pc'] == 0x80859B,
                'instruction entry/exit incomplete')
        require(all(0x808000 <= r['pc'] <= 0x80FFFF for r in group), 'unexpected instruction bank')
        require(all(0 < b['cpu_cycles'] - a['cpu_cycles'] <= 12 for a, b in zip(group, group[1:])),
                'unexplained instruction-cycle gap; extend capture')
        detail = dict(nmi=number, scope=events['nmi.entry']['scope'],
                      epoch=events['nmi.entry']['epoch'], instructions=len(group))
        for name, start, end in ranges:
            detail[name] = {key: events[end][key] - events[start][key]
                            for key in ('cpu_cycles', 'master_clock')}
        detail['bus_reads'] = dict(Counter(f"{r['address']:06X}" for r in bus if r['nmi'] == number))
        detail['apu_busy_reads'] = sum(r['value'] != 0 for r in bus
                                      if r['nmi'] == number and (r['address'] & 65535) == 0x2140)
        details.append(detail)
    scopes = []
    for scope in range(1, 5):
        require(set(constructors[scope]) == {'backdrop.entry', 'header.entry'}, 'constructor boundaries incomplete')
        group = [r for r in details if r['scope'] == scope]
        require(len(group) == (11 if scope in (1, 3) else 12), 'constructor NMI population differs')
        totals = {name: {key: sum(r[name][key] for r in group)
                        for key in ('cpu_cycles', 'master_clock')} for name, _, _ in ranges}
        entry, header = constructors[scope]['backdrop.entry'], constructors[scope]['header.entry']
        # Logged boundaries omit native interrupt entry8, vector JML4,
        # RTI7 cycles. This19 is CPU instruction cycles, NOT master clocks.
        producer_cycles = header['cpu_cycles'] - entry['cpu_cycles'] - totals['total']['cpu_cycles'] - 19 * len(group)
        scopes.append(dict(scope=scope, nmi_count=len(group), stages=totals,
                           producer_cpu_cycles=producer_cycles))
    preservation = {}
    if previous:
        read_capture(previous, rom)
        for name in ('interrupt_instructions.jsonl', 'interrupt_boundaries.jsonl'):
            require((native / name).read_bytes() == (previous / name).read_bytes(), 'old instruction/boundary capture changed')
        old_bus = jsonlines(previous / 'interrupt_bus.jsonl', 'event')
        index = {(r['master_clock'], r['address']): r for r in bus}
        require(len(index) == len(bus), 'duplicate bus read identity')
        for old in old_bus:
            new = index.get((old['master_clock'], old['address']))
            require(new and all(new[k] == v for k, v in old.items() if k != 'event'), 'old bus observation changed')
        snapshots = list(previous.glob('interrupt_*.wram'))
        require(len(snapshots) == 92 and all((native / p.name).read_bytes() == p.read_bytes() for p in snapshots),
                'NMI WRAM snapshots changed')
        preservation = dict(instruction_and_boundary_files_exact=True,
                            bus_reads=len(old_bus), full_wram_snapshots=92)
    return dict(schema=1, scope='observed source-work attribution only; no portable work-model/parity claim',
                canonical_rom_sha256=digest(rom), native_manifest_sha256=digest(native / 'manifest.json'),
                scheduler_events_preserved=preserved, previous_preservation=preservation,
                instructions=len(instructions), bus_reads=len(bus),
                scopes=scopes, per_nmi=details)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', required=True, type=Path)
    parser.add_argument('--scheduler', required=True, type=Path)
    parser.add_argument('--previous-tail', type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    require(not args.report.exists(), 'report exists; preserve evidence')
    report = analyze(args.native, args.scheduler, args.previous_tail, args.rom)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS observed attribution:', report['instructions'], 'instruction observations;',
          report['bus_reads'], 'hardware reads;', report['scheduler_events_preserved'], 'unchanged scheduler events')
    for scope in report['scopes']:
        print('scope', scope['scope'], 'producer CPU cycles', scope['producer_cpu_cycles'],
              'controller/audio master clocks', scope['stages']['controller']['master_clock'],
              scope['stages']['audio']['master_clock'])


if __name__ == '__main__':
    main()
