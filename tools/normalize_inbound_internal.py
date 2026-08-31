"""Losslessly compact native internal-stage snapshots; never run C for goldens."""
import argparse
import hashlib
import json
from pathlib import Path
from differential_compare import object_without_duplicates
from verify_inbound_internal import SNAPSHOT_FIELDS,json_read,validate_capture_rows


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--capture',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    if a.output.exists():
        raise ValueError('refuse to overwrite an existing native fixture')
    manifest=json_read(a.capture.parent/'run.json')
    metadata=json_read(a.capture.parent/'inbound-internal.meta.json')
    rows=[json.loads(line,object_pairs_hook=object_without_duplicates)
          for line in a.capture.read_text().splitlines()]
    validate_capture_rows(rows)
    if metadata.get('schema')!='nba95-inbound-internal-v1' or \
            manifest.get('runtime_state_injection') is not False or \
            metadata.get('runtime_state_injection') is not False or \
            manifest['trace_sha256']!=hashlib.sha256(a.capture.read_bytes()).hexdigest() or \
            manifest['calls']!=len(rows) or metadata['requested_calls']!=len(rows):
        raise ValueError('native source identity/population mismatch')
    source={key:manifest[key] for key in ('captured_utc','calls','oracle',
        'rom_sha256','mesen_sha256','script_sha256','driver_sha256','trace_sha256',
        'runtime_state_injection','setup_mode_injection','controller_mode','git_head')}
    source['capture_script']='tools/mesen_inbound_internal.lua'
    source['normalizer']='tools/normalize_inbound_internal.py'
    source['scope']='all captured fields at all stages; no C-derived expectations'
    data={'schema':'nba95-inbound-internal-compact-v1','source':source,
          'snapshot_fields':list(SNAPSHOT_FIELDS)}
    prefix=json.dumps(data,indent=2)[:-2]
    compact=[]
    for row in rows:
        item={'call':row['call']}
        item.update({stage:[snapshot[field] for field in SNAPSHOT_FIELDS]
                     for stage,snapshot in row.items() if stage not in ('call','schema')})
        compact.append(json.dumps(item,separators=(',',':')))
    a.output.write_text(prefix+',\n  "cases": [\n'+',\n'.join(compact)+'\n  ]\n}\n')
    print(f'Losslessly retained {len(rows)} native calls in {a.output.stat().st_size} bytes')


if __name__=='__main__':main()
