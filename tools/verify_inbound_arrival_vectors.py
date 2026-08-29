"""Replay native inbound-arrival outputs through the production helper."""
import argparse,json,subprocess
from pathlib import Path
def main():
    p=argparse.ArgumentParser();p.add_argument("--vectors",required=True)
    p.add_argument("--probe",required=True);a=p.parse_args()
    fixture=json.loads(Path(a.vectors).read_text()); calls=fixture["calls"]
    if fixture["raw_calls"]!=500 or fixture["arrival_calls"]!=387 or \
       fixture["native_launch_calls"]!=2:
        raise AssertionError("inbound arrival fixture population changed")
    payload="\n".join(" ".join(f"{v&0xffff:x}" for v in c["input"])
                       for c in calls)+"\n"
    r=subprocess.run([a.probe],input=payload,text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in x.split()] for x in r.stdout.splitlines()]
    bad=[(c["call"],c["expected"],x) for c,x in zip(calls,actual)
         if c["expected"]!=x]
    initial=sum(c["input"][5]==0 for c in calls)
    negative=sum(c["input"][9]&0x8000!=0 for c in calls)
    print(f"[INBOUND ARRIVAL] {'PASS' if not bad else 'FAIL'}: "
          f"calls={len(calls)} mismatches={len(bad)} native_launches=2 "
          f"first_arrivals={initial} negative_receivers={negative}")
    for row in bad[:10]: print("  call=%d rom=%s port=%s"%row)
    if len(actual)!=len(calls) or bad: raise SystemExit(1)
if __name__=="__main__": main()
