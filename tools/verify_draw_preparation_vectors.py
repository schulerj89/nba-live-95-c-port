"""Replay native ordinary-player draw preparation through production C."""
import argparse, json, subprocess
from pathlib import Path


def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--vectors",required=True)
    parser.add_argument("--probe",required=True); args=parser.parse_args()
    fixture=json.loads(Path(args.vectors).read_text()); calls=fixture["calls"]
    if fixture["raw_calls"]!=2000 or len(calls)!=2000:
        raise AssertionError("draw-preparation fixture population changed")
    payload="\n".join(" ".join(f"{v&0xffff:x}" for v in c["input"])
                       for c in calls)+"\n"
    result=subprocess.run([args.probe],input=payload,text=True,
                          capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
    mismatches=[(c["call"],c["expected"],a) for c,a in zip(calls,actual)
                if c["expected"]!=a]
    priorities={c["expected"][5]-c["input"][16] for c in calls}
    resources=len({(c["input"][8],c["input"][9]) for c in calls})
    print(f"[DRAW PREPARATION] {'PASS' if not mismatches else 'FAIL'}: "
          f"calls={len(calls)} mismatches={len(mismatches)} "
          f"resource_pairs={resources} priorities={sorted(priorities)}")
    for mismatch in mismatches[:10]: print("  call=%d rom=%s port=%s"%mismatch)
    if len(actual)!=len(calls) or mismatches: raise SystemExit(1)


if __name__=="__main__": main()
