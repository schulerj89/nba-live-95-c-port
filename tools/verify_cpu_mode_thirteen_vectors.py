"""Strictly replay compact native `$86:A7DA-$A993` mode-thirteen witnesses."""

import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

from normalize_cpu_mode_thirteen_vectors import CASE_NAMES, FIELDS

EXPECTED_ROUTINE="$86:A7DA-$A993 mode-thirteen close finish"
EXPECTED_ROM="2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
EXPECTED_STARTS="8099d3c9748e73c57a37a44dc0ae436c6bc9b42d4ffbb641b60d4214ff7cb282"
EXPECTED_CALLS="f46f7e758936916a2213f3d9c90bfaa3e0e0ec5627a3b2623027c75ff10fd96b"
EXPECTED_DOMAIN=("represented parent entry/exit state plus direct gameplay child outputs; "
    "actor +$00 is identity-only; native slot/id 5 -> $343F -> player $446B and "
    "active lineup $4779=2 map to host actor[5], context1, roster slot2/persistent "
    "index14 (pack roster address $AC:F636); controllers {-1,0}; captured fixed inputs "
    "period $0926=0, difficulty $17AF=0, and both context +$08 basket fractions=0; "
    "B832 DP $46/$47 attachment scratch is retained, other DP/stack arithmetic scratch is excluded; "
    "table bytes remain pack-backed data")
EXPECTED_SOURCE={
    "vectors_sha256":"72cd5c59ef0f042afb2f6cee5f4c6709a7cd5ad4caf68f7ab7ccd02e6c482fee",
    "repeat_vectors_sha256":"b310ca85621089dffae7968b6cac9544ec6c457429b966c05f87061cc8b3bd46",
    "cases_sha256":"96b5ec651e58a606b8a705613831dfa8c6dd8404fd3767a0882a61076ed8e989",
    "paths_sha256":"7e95fca6af7adad344750d17562ea85868796fe67b0564e33e5f287c116bd5e3",
    "ghidra_functions_sha256":"c3ce476090ee9a9ec970ce9b9ccb2fdeea42936f8a3ff9184880b609fff3b199",
    "ghidra_instructions_sha256":"f642199de28e016061db579f28a28f60c772de70a6a2a6712ded609e03f7d63c",
}
EXPECTED_AGGREGATE={"restore_869846":8,"cancel_lower_87b555":6,
    "upper_animation_87b47a":12,"pose_87aec3":49,"attach_xy_87b649":27,
    "attach_z_87b66a":6,"launch_869d6e":6,"finish_86a9d0":8,
    "landing_86986d":8,"attach_point_87b832":49,"cancel_pass_86a613":2,
    "stats_869cdb":14,"effect_87a9e3":3}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--vectors",type=Path,required=True)
    p.add_argument("--probe",type=Path,required=True)
    p.add_argument("--pack",type=Path,required=True)
    p.add_argument("--skip-caller",action="store_true")
    a=p.parse_args(); document=json.loads(a.vectors.read_text())
    calls=document.get("calls",[]); fields=document.get("fields",[])
    calls_hash=hashlib.sha256(json.dumps(calls,separators=(",",":")).encode()).hexdigest()
    if (type(document.get("schema")) is not int or document.get("schema")!=1 or
        document.get("routine")!=EXPECTED_ROUTINE or
        document.get("rom_sha256")!=EXPECTED_ROM or document.get("domain")!=EXPECTED_DOMAIN or
        document.get("owned_starts_sha256")!=EXPECTED_STARTS or
        document.get("aggregate_child_calls")!=EXPECTED_AGGREGATE or
        document.get("source")!=EXPECTED_SOURCE or tuple(fields)!=FIELDS or
        tuple(c.get("name") for c in calls)!=CASE_NAMES or calls_hash!=EXPECTED_CALLS):
        raise ValueError("mode-thirteen fixture identity/provenance changed")
    payload=bytearray(); union=set(); exits=set(); aggregate={}
    for number,call in enumerate(calls,1):
        before=call.get("input",[]); expected=call.get("expected",[])
        if len(before)!=len(FIELDS) or len(expected)!=len(FIELDS) or any(
            type(v) is not int or not 0<=v<=0xffff for v in before+expected):
            raise ValueError(f"case {number} has an invalid field vector")
        executed=call.get("executed",[])
        if (call.get("call")!=number or call.get("entry_pc")!="86a7da" or
            call.get("exit_pc") not in {"86a866","86a922","86a92e"} or
            not executed or executed[0]!="86a7da" or executed[-1]!=call["exit_pc"]):
            raise ValueError(f"case {number} has an invalid native boundary")
        union.update(executed);exits.add(call["exit_pc"])
        for child,count in call.get("child_calls",{}).items():aggregate[child]=aggregate.get(child,0)+count
        payload.extend(struct.pack("<"+"H"*len(FIELDS),*before))
    union_hash=hashlib.sha256("\n".join(sorted(union)).encode()).hexdigest()
    if len(union)!=172 or union_hash!=EXPECTED_STARTS or exits!={"86a866","86a922","86a92e"} or aggregate!=EXPECTED_AGGREGATE:
        raise ValueError("mode-thirteen path/child contract changed")
    run=subprocess.run([str(a.probe),str(a.pack)],input=payload,capture_output=True,check=True)
    lines=[line for line in run.stdout.decode().splitlines() if len(line.split())==len(FIELDS)]
    if len(lines)!=len(calls):raise AssertionError(f"probe returned {len(lines)}/{len(calls)} rows")
    mismatches=[]
    for number,(call,line) in enumerate(zip(calls,lines),1):
        actual=[int(word,16) for word in line.split()]
        differences=[(fields[i],want,got) for i,(want,got) in enumerate(zip(call["expected"],actual)) if want!=got]
        if differences:mismatches.append((number,call["name"],differences))
    print(f"[CPU MODE THIRTEEN] {'PASS' if not mismatches else 'FAIL'}: calls={len(calls)} fields={len(fields)} mismatches={len(mismatches)}")
    for number,name,differences in mismatches:print(f"case {number} {name}: {differences[:16]}")
    if mismatches:raise SystemExit(1)
    stale=subprocess.run([str(a.probe),str(a.pack),"--stale-ball-owner"],input=payload,capture_output=True)
    stale_lines=[line for line in stale.stdout.decode().splitlines() if len(line.split())==len(FIELDS)]
    if stale.returncode or stale_lines!=lines:raise AssertionError("authoritative $093E replay depends on stale host ball owner")
    print("[CPU MODE THIRTEEN OWNER] PASS: stale host owner cannot replace authoritative $093E")
    if not a.skip_caller:
        caller=subprocess.run([str(a.probe),str(a.pack),"--self-test"],capture_output=True,text=True)
        if caller.returncode or "[CPU MODE THIRTEEN CALLER] PASS" not in caller.stdout:
            print(caller.stdout,end="");print(caller.stderr,end="");raise AssertionError("production caller check failed")
        print("[CPU MODE THIRTEEN CALLER] PASS")

if __name__=="__main__":main()
