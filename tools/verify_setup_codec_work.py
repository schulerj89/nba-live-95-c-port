"""Separate source-work and native validation for the bounded FB46 continuation.

Native entry registers are diagnostic-only leaf inputs. No native clock or
snapshot is input to the work producer. This is not an end-to-end phase gate.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess

from verify_setup_scheduler import (CAPTURE_KIND, CAPTURE_SCHEDULE, ROM_SHA, MESEN_SHA,
    require, digest, pairs, read_json, exact_keys, integer, sha256, path_value,
    typed_equal, expected_settings, validate_rows, read_capture, check_previous)

BUILD_SOURCES = {'include/nba_setup_codec_work.h', 'src/nba_setup_codec_work.c',
                 'tools/setup_codec_work_probe.c', 'tools/build_setup_codec_work_probe.ps1'}
RESOURCES = {
    0xAEC446: (960, 34826, 218142),
    0xAED153: (2054, 94738, 602298),
    0xA6C5FC: (6336, 246864, 1556550),
    0xAF97AA: (838, 32091, 202290),
}

def validate_capture_manifest(manifest, directory, rom):
    # Known legacy captures omit four before-wait dumps. Their core identities
    # are still mandatory. The complete on-disk file inventory must be attested
    # as well, preventing an omitted declaration from silently avoiding hashing.
    tail = True
    fields = {'schema', 'kind', 'state_injection', 'rom_patch', 'accepted', 'sources',
              'arguments', 'isolation', 'schedule', 'exit_code', 'artifacts'}
    exact_keys(manifest, fields | ({'codec_summary'} if tail else set()), 'capture manifest')
    integer(manifest['schema'], 1, 1, 'capture schema')
    integer(manifest['exit_code'], 0, 0, 'capture exit_code')
    require(manifest['accepted'] is True and manifest['state_injection'] is False and
            manifest['rom_patch'] is False, 'successful natural capture required')
    require(manifest['kind'] == CAPTURE_KIND and manifest['schedule'] == CAPTURE_SCHEDULE,
            'capture kind/input schedule differs')
    source_paths = {'rom': Path(rom).resolve(), 'mesen': directory / 'portable-mesen/Mesen.exe',
                    'script': directory / 'capture.lua', 'runner': directory / 'capture_runner.py',
                    'settings': directory / 'initial-settings.json'}
    if tail:
        source_paths['base_script'] = directory / 'scheduler_base.lua'
        require(type(manifest['codec_summary']) is str and
                re.fullmatch(r'ok; scopes=4; calls=20; instructions=[1-9][0-9]*; writes=[1-9][0-9]*\n',
                             manifest['codec_summary']) is not None, 'invalid codec summary')
    exact_keys(manifest['sources'], source_paths, 'capture sources')
    for name, expected in source_paths.items():
        entry = manifest['sources'][name]
        exact_keys(entry, {'path', 'sha256'}, 'capture source ' + name)
        sha256(entry['sha256'], 'capture source ' + name)
        require(path_value(entry['path'], name) == expected.resolve(), 'capture source path differs: ' + name)
    require(manifest['sources']['rom']['sha256'] == ROM_SHA and
            manifest['sources']['mesen']['sha256'] == MESEN_SHA, 'canonical ROM/Mesen identity mismatch')
    arguments = manifest['arguments']
    require(type(arguments) is list and len(arguments) == 5 and all(type(a) is str for a in arguments),
            'invalid capture arguments')
    require(arguments[1:3] == ['--testrunner', '--timeout=300'] and
            all(path_value(arguments[index], 'capture argument') == source_paths[name].resolve()
                for index, name in ((0, 'mesen'), (3, 'rom'), (4, 'script'))), 'capture arguments differ')
    core = {'capture.lua', 'capture_runner.py', 'initial-settings.json', 'mesen.log',
            'observed_environment.txt', 'scheduler.jsonl', 'state_fields.txt', 'capture_complete.txt'}
    core |= {f'header_{n:02d}_{stage}.wram' for n in range(1, 5) for stage in ('entry', 'after_wait')}
    before = {f'header_{n:02d}_before_wait.wram' for n in range(1, 5)}
    inventory = {p.name for p in directory.iterdir() if p.is_file() and p.name != 'manifest.json'}
    core |= before
    if tail:
        core |= {'scheduler_base.lua', 'codec_complete.txt', 'codec_instructions.jsonl',
                 'codec_boundaries.jsonl', 'codec_writes.jsonl'}
        core |= {f'codec_{n:02d}_{stage}.wram' for n in range(1, 21) for stage in ('entry', 'exit')}
    exact_keys(manifest['artifacts'], core, 'capture artifacts')
    require(inventory == core, 'capture artifact inventory differs from required declarations')
    for name, entry in manifest['artifacts'].items():
        exact_keys(entry, {'bytes', 'sha256'}, 'capture artifact ' + name)
        integer(entry['bytes'], 0 if name == 'mesen.log' else 1, 2**63 - 1, 'artifact bytes ' + name)
        if name.endswith('.wram'):
            integer(entry['bytes'], 0x20000, 0x20000, 'WRAM snapshot bytes')
        sha256(entry['sha256'], 'capture artifact ' + name)
    isolation = manifest['isolation']
    exact_keys(isolation, {'home', 'save_folder', 'initial_saves', 'settings', 'observed', 'post_settings_sha256'},
               'capture isolation')
    require(path_value(isolation['home'], 'home') == directory / 'portable-mesen' and
            path_value(isolation['save_folder'], 'save folder') == directory / 'isolated-saves', 'private home/save path differs')
    require(type(isolation['initial_saves']) is list and isolation['initial_saves'] == [], 'fresh empty saves required')
    typed_equal(isolation['settings'], expected_settings(directory), 'declared settings')
    exact_keys(isolation['observed'], {'output', 'home'}, 'observed environment')
    require(path_value(isolation['observed']['output'], 'observed output') == directory and
            path_value(isolation['observed']['home'], 'observed home').is_relative_to(directory / 'portable-mesen'),
            'declared observed environment differs')
    sha256(isolation['post_settings_sha256'], 'persisted settings')


def read_native(directory, rom):
    directory = Path(directory).resolve()
    manifest = read_json(directory / 'manifest.json')
    validate_capture_manifest(manifest, directory, rom)
    for entry in manifest['sources'].values():
        require(digest(entry['path']) == entry['sha256'], 'source changed: ' + entry['path'])
    for name, entry in manifest['artifacts'].items():
        require(Path(name).name == name, 'invalid artifact path')
        path = directory / name
        require(path.stat().st_size == entry['bytes'] and digest(path) == entry['sha256'],
                'capture artifact changed: ' + name)
    observed = pairs(line.split('=', 1) for line in
                     (directory / 'observed_environment.txt').read_text().splitlines())
    typed_equal(observed, manifest['isolation']['observed'], 'actual observed environment')
    settings = expected_settings(directory)
    typed_equal(read_json(directory / 'initial-settings.json'), settings, 'actual initial settings')
    post = directory / 'portable-mesen/settings.json'
    require(digest(post) == manifest['isolation']['post_settings_sha256'], 'persisted settings identity differs')
    typed_equal(read_json(post), settings, 'actual persisted settings', subset=True)
    require((directory / 'capture_complete.txt').read_text() ==
            'ok; headers=4; normal controller-only Rules repeat journey\n', 'missing sentinel')
    rows = [json.loads(line, object_pairs_hook=pairs) for line in
            (directory / 'scheduler.jsonl').read_text().splitlines()]
    validate_rows(rows)
    return rows


def check_build(exe):
    manifest = read_json(exe.parent / 'build-manifest.json')
    exact_keys(manifest, {'schema', 'compiler_exit', 'sources', 'executable'}, 'build manifest')
    integer(manifest['schema'], 1, 1, 'build schema')
    integer(manifest['compiler_exit'], 0, 0, 'compiler_exit')
    exact_keys(manifest['sources'], BUILD_SOURCES, 'build sources')
    for entry in [manifest['executable'], *manifest['sources'].values()]:
        exact_keys(entry, {'path', 'sha256'}, 'build identity')
        path_value(entry['path'], 'build identity')
        sha256(entry['sha256'], 'build identity')
    source_root = Path(manifest['sources']['src/nba_setup_codec_work.c']['path']).resolve().parents[1]
    require(all(Path(entry['path']).resolve() == (source_root / name).resolve()
                for name, entry in manifest['sources'].items()), 'build source paths differ')
    require(Path(manifest['executable']['path']).resolve() == exe.resolve() and
            digest(exe) == manifest['executable']['sha256'], 'executable changed')
    for entry in manifest['sources'].values():
        require(digest(entry['path']) == entry['sha256'], 'source changed since probe build')
    return manifest


def json_lines(path):
    return [json.loads(line, object_pairs_hook=pairs) for line in path.read_text().splitlines()]


def integer_rows(rows, schema, ordinal):
    for index, row in enumerate(rows):
        exact_keys(row, schema, 'trace row')
        for key, limit in schema.items():
            if key == 'tag':
                require(row[key] in {'codec.entry', 'codec.exit'}, 'unknown codec boundary')
            else:
                integer(row[key], 0, limit, key)
        if ordinal:
            require(row[ordinal] == index, 'trace ordinal discontinuity')


def native_records(directory):
    common = dict(pc=0xffffff, call=20, cpu_cycles=2**63-1, master_clock=2**63-1,
                  a=65535, x=65535, y=65535, ps=255, db=255, sp=65535)
    instructions = json_lines(directory / 'codec_instructions.jsonl')
    integer_rows(instructions, dict(common, instruction=2**31-1), 'instruction')
    boundaries = json_lines(directory / 'codec_boundaries.jsonl')
    integer_rows(boundaries, dict(common, scope=4, event=39, dp=65535, source=0xffffff,
        destination=0xffffff, mode=65535, head=511, tail=511, ppu_frame=2**31-1,
        scanline=261, hclock=1363, tag=None), 'event')
    require(len(boundaries) == 40, 'twenty codec entry/exit pairs required')
    for index, row in enumerate(boundaries):
        require(row['call'] == index // 2 + 1 and row['scope'] == index // 10 + 1,
                'unexpected codec call/scope sequence')
        require(row['tag'] == ('codec.entry' if index % 2 == 0 else 'codec.exit') and
                row['pc'] == (0x80c62b if index % 2 == 0 else 0x80c682), 'boundary PC/tag mismatch')
        require(row['dp'] == 0 and row['ps'] & 0x38 == 0 and row['head'] == row['tail'],
                'bounded native call preconditions differ')
        if index % 2 == 0:
            source = [0xAEA0AF, *RESOURCES][(index // 2) % 5]
            require(row['source'] == source and row['destination'] == 0x7f2000,
                    'canonical backdrop operands differ')
    writes = json_lines(directory / 'codec_writes.jsonl')
    integer_rows(writes, dict(call=20, event=2**31-1, pc=0xffffff, address=0xffffff,
        value=255, cpu_cycles=2**63-1, master_clock=2**63-1), 'event')
    expected = f'ok; scopes=4; calls=20; instructions={len(instructions)}; writes={len(writes)}\n'
    require((directory / 'codec_complete.txt').read_text() == expected and
            read_json(directory / 'manifest.json')['codec_summary'] == expected,
            'codec completion counts differ')
    require({x['call'] for x in instructions} == {1, 2, 3, 4, 5} and
            {x['call'] for x in writes} == {1, 2, 3, 4, 5}, 'first-call observation scope differs')
    return boundaries, instructions, writes


def separate_native_effects(call, instructions, writes):
    rows = [r for r in instructions if r['call'] == call]
    observed = [r for r in writes if r['call'] == call]
    # The broad exec callback is registered before the NMI guard callback.
    # It therefore observes the first $815A instruction, but no handler body.
    # CPU hardware entry also emits four stack writes before that hook. Validate
    # all four against native state and the next producer PC; never blind-skip.
    excluded = set()
    nmis = [r for r in rows if r['pc'] == 0x80815a]
    for nmi in nmis:
        position = rows.index(nmi)
        require(position > 0 and position + 1 < len(rows), 'incomplete NMI bracket')
        resume = rows[position + 1]
        expected_values = [0x80, (resume['pc'] >> 8) & 255, resume['pc'] & 255, resume['ps']]
        for k, value in enumerate(expected_values):
            matches = [r for r in observed if r['cpu_cycles'] == nmi['cpu_cycles'] - 9 + k]
            require(len(matches) == 1, 'missing/duplicate NMI hardware stack write')
            row = matches[0]
            require(row['address'] == nmi['sp'] + 4 - k and row['value'] == value and
                    row['pc'] == rows[position - 1]['pc'], 'NMI stack entry differs from source protocol')
            excluded.add(row['event'])
    # Mesen observes the WRAM effect induced by $2180 as a second callback.
    # This is an effect of the same CPU bus write, not another CPU bus cycle.
    ram = [r for r in observed if r['address'] >> 16 in (0x7e, 0x7f)]
    cpu = [r for r in observed if r['event'] not in excluded and r['address'] >> 16 not in (0x7e, 0x7f)]
    ports = [r for r in cpu if r['address'] & 0xffff == 0x2180]
    require(len(ram) == len(ports), 'WRAM port effect cardinality differs')
    for index, (write, effect) in enumerate(zip(ports, ram)):
        require(effect['address'] == 0x7f2000 + index and
                all(effect[k] == write[k] for k in ('pc', 'value', 'cpu_cycles', 'master_clock')) and
                effect['event'] == write['event'] + 1, 'WRAM port write effect/order differs')
    return [r for r in rows if r['pc'] != 0x80815a], cpu, len(nmis)


def run_probe(exe, rom, source, output, entry=None):
    prefix = output / (f'{source:06X}' + ('-native-entry' if entry else '-default'))
    args = [str(exe), str(rom), f'{source:06X}', str(prefix.with_suffix('.wram'))]
    if entry:
        args += [str(prefix.with_suffix('.jsonl')),
                 ','.join(str(entry[k]) for k in ('a', 'x', 'y', 'sp', 'db', 'ps', 'head'))]
    result = subprocess.run(args, text=True, capture_output=True)
    require(result.returncode == 0 and result.stderr == '', 'fresh codec probe failed')
    prefix.with_suffix('.json').write_text(result.stdout)
    report = json.loads(result.stdout, object_pairs_hook=pairs)
    exact_keys(report, {'schema', 'status', 'bus_valid', 'source', 'cycles', 'master', 'slow',
        'instructions', 'output_bytes', 'declared_bytes', 'cursor', 'sp', 'wmadd', 'counts'}, 'probe report')
    for key in set(report) - {'bus_valid', 'counts'}:
        integer(report[key], 0, 2**63-1, 'probe ' + key)
    require(report['schema'] == 1 and report['status'] == 1 and report['bus_valid'] is True and
            report['source'] == source, 'probe not complete')
    require(type(report['counts']) is dict and report['counts'], 'source block counts missing')
    for pc, count in report['counts'].items():
        require(re.fullmatch(r'80[0-9A-F]{4}', pc) is not None, 'invalid source block PC')
        integer(count, 1, 2**31-1, 'source count')
    require(sum(report['counts'].values()) == report['instructions'], 'source counts do not sum')
    size, cycles, master = RESOURCES[source]
    require((report['output_bytes'], report['declared_bytes'], report['cycles'], report['master']) ==
            (size, size, cycles, master), 'independent source checksum differs')
    require(master == cycles * 6 + report['slow'] * 2, 'intrinsic bus accounting differs')
    blob = prefix.with_suffix('.wram').read_bytes()
    require(len(blob) == 0x20000 and report['wmadd'] == 0x12000 + size and
            report['sp'] == (entry['sp'] if entry else 0x1fef), 'call output/stack contract differs')
    return report, blob[0x12000:0x12000 + size], prefix


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--previous-native', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    native, rom, exe, output = (p.resolve() for p in (args.native, args.rom, args.exe, args.output))
    require(digest(rom) == ROM_SHA, 'canonical ROM required')
    build = check_build(exe)
    rows = read_native(native, rom)
    previous = read_capture(args.previous_native, rom)
    invariance = check_previous(rows, previous)
    boundaries, instructions, writes = native_records(native)
    output.mkdir(parents=True, exist_ok=False)
    blob = rom.read_bytes()
    provenance = {}
    for name, start, end in [('wrapper', 0xc62b, 0xc682), ('empty_queue', 0x86da, 0x86ec),
                             ('fb46', 0xbd1b, 0xbe6a)]:
        data = blob[start & 0x7fff:(end & 0x7fff) + 1]
        provenance[name] = dict(first=0x800000 | start, last=0x800000 | end, bytes=len(data),
                                sha256=hashlib.sha256(data).hexdigest())
    proofs, validations = [], []
    for source in RESOURCES:
        report, payload, _ = run_probe(exe, rom, source, output)
        entry = next(r for r in boundaries if r['tag'] == 'codec.entry' and r['source'] == source)
        diagnostic, diagnostic_payload, prefix = run_probe(exe, rom, source, output, entry)
        require(payload == diagnostic_payload and
                all(report[k] == diagnostic[k] for k in ('cycles', 'master', 'slow', 'counts')),
                'native entry changed source work or payload')
        traces = json_lines(prefix.with_suffix('.jsonl'))
        for row in traces:
            if row.get('kind') == 'instruction':
                schema = dict(cycle=2**63-1, master=2**63-1, pc=0xffffff,
                              a=65535, x=65535, y=65535, sp=65535, db=255, ps=255)
            else:
                require(row.get('kind') == 'write', 'unknown C trace event')
                schema = dict(cycle=2**63-1, pc=0xffffff, address=0xffffff, value=255)
            exact_keys(row, {'kind', *schema}, 'C source event')
            for key, limit in schema.items():
                integer(row[key], 0, limit, 'C source ' + key)
        ci = [r for r in traces if r['kind'] == 'instruction']
        cw = [r for r in traces if r['kind'] == 'write']
        require(len(ci) + len(cw) == len(traces), 'unknown probe event')
        ni, nw, nmis = separate_native_effects(entry['call'], instructions, writes)
        require(len(ci) == len(ni) == report['instructions'] and len(cw) == len(nw), 'source event lengths differ')
        for index, (c, n) in enumerate(zip(ci, ni)):
            require(all(c[k] == n[k] for k in ('pc', 'a', 'x', 'y', 'sp', 'db', 'ps')),
                    f'source instruction/register mismatch {source:06X}/{index}')
        # CPU completion points must match individually, not just in aggregate.
        # For master clocks this remains a conservation check: logged NMI time
        # is removed and each remaining refresh contributes 40 clocks.
        end = boundaries[(entry['call'] - 1) * 2 + 1]
        ni_next = ni[1:] + [end]
        ci_next = ci[1:] + [dict(cycle=report['cycles'] + 1, master=report['master'])]
        observed_in = [r for r in rows if r['tag'] == 'nmi.entry' and
                       entry['master_clock'] < r['master_clock'] < end['master_clock']]
        observed_out = [r for r in rows if r['tag'] == 'nmi.exit' and
                        entry['master_clock'] < r['master_clock'] < end['master_clock']]
        require(len(observed_in) == len(observed_out), 'unpaired instruction-level NMI')
        for index, (c, cn, n, nn) in enumerate(zip(ci, ci_next, ni, ni_next)):
            interrupts = [(a, b) for a, b in zip(observed_in, observed_out)
                          if n['master_clock'] < a['master_clock'] < nn['master_clock']]
            cpu = nn['cpu_cycles'] - n['cpu_cycles'] - sum(b['cpu_cycles'] - a['cpu_cycles'] + 19 for a, b in interrupts)
            master = nn['master_clock'] - n['master_clock'] - sum(b['master_clock'] - a['master_clock'] + 142 for a, b in interrupts)
            require(cpu == cn['cycle'] - c['cycle'], f'instruction CPU work differs {source:06X}/{index}')
            refresh = master - (cn['master'] - c['master'])
            require(refresh >= 0 and refresh % 40 == 0,
                    f'instruction intrinsic master work differs {source:06X}/{index}')
        instruction_index = 0
        for index, (c, n) in enumerate(zip(cw, nw)):
            require(all(c[k] == n[k] for k in ('pc', 'address', 'value')),
                    f'ordered CPU write mismatch {source:06X}/{index}')
            while instruction_index + 1 < len(ci) and ci[instruction_index + 1]['cycle'] <= c['cycle']:
                instruction_index += 1
            require(c['cycle'] - ci[instruction_index]['cycle'] + 1 ==
                    n['cpu_cycles'] - ni[instruction_index]['cpu_cycles'],
                    f'CPU write bus-cycle position differs {source:06X}/{index}')
        require(Counter(f'{r["pc"]:06X}' for r in ni) == report['counts'], 'source path count mismatch')
        matches = []
        for start, end in zip(boundaries[::2], boundaries[1::2]):
            if start['source'] != source: continue
            size = len(payload)
            require((native / f'codec_{start["call"]:02d}_exit.wram').read_bytes()[0x12000:0x12000 + size] == payload,
                    'full native payload differs')
            relevant = [r for r in rows if start['master_clock'] < r['master_clock'] < end['master_clock']]
            nmi_in = [r for r in relevant if r['tag'] == 'nmi.entry']
            nmi_out = [r for r in relevant if r['tag'] == 'nmi.exit']
            require(len(nmi_in) == len(nmi_out), 'unpaired NMI interval')
            nmi_cpu = sum(b['cpu_cycles'] - a['cpu_cycles'] for a, b in zip(nmi_in, nmi_out))
            nmi_master = sum(b['master_clock'] - a['master_clock'] for a, b in zip(nmi_in, nmi_out))
            measured_cpu = end['cpu_cycles'] - start['cpu_cycles']
            require(measured_cpu == report['cycles'] + nmi_cpu + len(nmi_in) * 19,
                    'CPU work conservation differs')
            residual = end['master_clock'] - start['master_clock'] - nmi_master - len(nmi_in) * 142 - report['master']
            require(residual >= 0 and residual % 40 == 0, 'master work conservation has unexplained residue')
            matches.append(dict(call=start['call'], cpu_cycles=measured_cpu, nmi_count=len(nmi_in),
                logged_nmi_cpu=nmi_cpu, logged_nmi_master=nmi_master,
                residual_refresh_quanta=residual // 40,
                caveat='conservation using observed NMI intervals, not a forward phase prediction'))
        input_start = ((source >> 16) & 127) * 0x8000 + (source & 0x7fff)
        input_bytes = blob[input_start:input_start + report['cursor'] - (source & 65535) + 1]
        proofs.append(dict(source=source, source_work=report, input_bytes=len(input_bytes),
            input_sha256=hashlib.sha256(input_bytes).hexdigest(), payload_sha256=hashlib.sha256(payload).hexdigest()))
        validations.append(dict(source=source, exact_instructions=len(ni), exact_cpu_writes=len(nw),
            separately_validated_nmi_prologues=nmis, complete_payload_matches=len(matches), intervals=matches))
    identity = {name: dict(path=str(path), sha256=digest(path)) for name, path in {
        'rom':rom, 'native_manifest':native/'manifest.json', 'previous_manifest':args.previous_native/'manifest.json',
        'verifier':Path(__file__), 'strict_scheduler_verifier':Path(__file__).with_name('verify_setup_scheduler.py')}.items()}
    work_proof = dict(schema=1, scope='C62B entry through C682 entry; FB46 only; no final RTL',
        preconditions='native mode; M=X=D=0; FastROM; empty immediate queue; live WRAM/IO mirrors',
        identity=identity, build=build, routines=provenance, resources=proofs,
        limitation='intrinsic work only; no refresh, DMA, NMI/audio/SPC or end-to-end epochs')
    validation = dict(schema=1, accepted=True, identity=identity, resources=validations,
        previous_scheduler_events=len(previous), previous_invariance=invariance,
        limitation='leaf validation from typed entry registers; no native snapshot or clock is a work input')
    (output/'source-work-proof.json').write_text(json.dumps(work_proof, indent=2)+'\n')
    (output/'native-validation.json').write_text(json.dumps(validation, indent=2)+'\n')
    print('PASS: four FB46 work checksums; 112814 source instructions/register states; 28218 CPU writes; 16 native payloads')


if __name__ == '__main__':
    main()

