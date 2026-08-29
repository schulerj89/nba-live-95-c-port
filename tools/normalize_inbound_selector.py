"""Retain natural native `$86:F5C7-$F653` launch-selector witnesses."""
import argparse,json
from pathlib import Path
def memory(snapshot):
    out={}
    for base,payload in snapshot.items():
        start=int(base,16);out.update((start+i,v) for i,v in enumerate(bytes.fromhex(payload)))
    return out
def word(mem,address): return mem[address]|mem[address+1]<<8
def main():
    p=argparse.ArgumentParser();p.add_argument("--vectors",required=True)
    p.add_argument("--output",required=True);a=p.parse_args();calls=[]
    for line in Path(a.vectors).read_text().splitlines():
        r=json.loads(line)
        if r["exit_pc"].lower()!="86f653":continue
        before=memory(r["entry"]["mem"]);after=memory(r["exit"]["mem"])
        actor_ptr=word(before,0x96); inbounder=word(before,actor_ptr)
        values=[inbounder,word(before,0x92e),word(before,0x9aa),
                word(before,0x9ac),word(before,0x9ae)]
        for i in range(10):
            base=0x34eb+i*0x100
            values += [word(before,base+4),word(before,base+8),
                       word(before,base+0x30),word(before,base+0x86),
                       word(before,base+0x8a)]
        calls.append({"call":r["call"],"input":values,
                      "expected":word(after,0x946)})
    Path(a.output).write_text(json.dumps({
        "routine":"$86:F5C7-$F653 natural CPU inbound selector/launch",
        "provenance":"natural ROM execution in Mesen; no PC/ROM patching",
        "raw_calls":500,"launch_calls":len(calls),"calls":calls
    },separators=(",",":"))+"\n")
    print(f"normalized {len(calls)} native launches")
if __name__=="__main__":main()
