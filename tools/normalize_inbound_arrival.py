"""Reduce natural `$86:F43A` captures to native arrived-state witnesses."""
import argparse, hashlib, json
from pathlib import Path

def memory(snapshot):
    out={}
    for base,payload in snapshot.items():
        start=int(base,16)
        out.update((start+i,v) for i,v in enumerate(bytes.fromhex(payload)))
    return out
def word(mem,address): return mem[address] | mem[address+1]<<8
def signed(value): return value-0x10000 if value&0x8000 else value

def main():
    p=argparse.ArgumentParser(); p.add_argument("--vectors",required=True)
    p.add_argument("--output",required=True); args=p.parse_args()
    source=Path(args.vectors)
    rows=[json.loads(line) for line in source.read_text().splitlines()]
    if not rows: raise ValueError('empty native arrival capture')
    if [row['call'] for row in rows] != list(range(1,len(rows)+1)):
        raise ValueError('missing, duplicated or reordered native calls')
    calls=[]; launches=0
    for row in rows:
        if row['entry_pc'].lower() != '86f43a' or \
                row['exit_pc'].lower() not in ('86f439','86f653'):
            raise ValueError('unexpected native routine boundary')
        before=memory(row["entry"]["mem"])
        after=memory(row["exit"]["mem"]); actor=word(before,0x96)
        launches += int(row['exit_pc'].lower() == '86f653')
        dx=signed((word(before,0x958)-word(before,actor+4))&0xffff)
        dy=signed((word(before,0x95a)-word(before,actor+8))&0xffff)
        if not (-9<=dx<9 and -9<=dy<9): continue
        # The two F653 exits continue through AB2D, which intentionally
        # changes actor flags/transfer after this helper's F58E boundary.
        # They are retained by the selector/launch fixture, not compared as
        # arrival-boundary outputs here.
        if row["exit_pc"].lower()=="86f653": continue
        inputs=[word(before,z) for z in (0x968,0x9f6)] + [
            word(before,actor+0x7e),word(before,actor+0x0e),
            word(before,actor+0x10),word(before,0x9ba),
            word(before,0x9b6),word(before,0x964),word(before,0x9b8),
            word(before,0x946),word(before,0x95c),word(before,actor+0x4e)]
        expected=[word(after,z) for z in (0x968,0x9f6)] + [
            word(after,actor+0x7e),word(after,actor+0x0e),
            word(after,actor+0x10),word(after,0x9ba),
            word(after,0x9b6),word(after,0x964),word(after,0x9b8),
            word(after,actor+0x4e)]
        calls.append({"call":row["call"],"frame":row["entry_frame"],
                      "input":inputs,"expected":expected})
    Path(args.output).write_text(json.dumps({
        "routine":"$86:F4F2-$F58E CPU inbound arrival/readiness",
        "provenance":"natural ROM execution in Mesen; no PC/ROM patching",
        "source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
        "raw_calls":len(rows),"native_launch_calls":launches,
        "arrival_calls":len(calls),"calls":calls
    },separators=(",",":"))+"\n")
    print(f"normalized {len(calls)} arrived calls")
if __name__=="__main__": main()
