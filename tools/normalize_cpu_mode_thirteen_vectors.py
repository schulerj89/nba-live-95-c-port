"""Normalize repeated genuine `$86:A7DA-$A993` mode-thirteen calls."""

import argparse
import hashlib
import json
from pathlib import Path

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
EXPECTED_STARTS_SHA256 = "8099d3c9748e73c57a37a44dc0ae436c6bc9b42d4ffbb641b60d4214ff7cb282"
SIZE = 0x10000
CASE_NAMES = (
    "lost_owner_same_group", "lost_owner_other_group", "early_wrong_upper",
    "early_upper_1f_boundary", "early_dx_negative_81", "ordinary_dx_negative_80",
    "ordinary_dx_positive_80", "early_dx_positive_81", "early_dy_negative_81",
    "ordinary_dy_negative_80", "ordinary_dy_positive_80", "early_dy_positive_81",
    "ordinary_disturbed_below_timer_gate", "ordinary_grounded_wrong_upper",
    "timer_24_special_zero_airborne_jump_270",
    "timer_24_special_six_face_three_jump_270",
    "timer_24_special_six_other_jump_264", "grounded_lower_25_skips_jump",
    "grounded_negative_odd_high_table_jump_264", "airborne_below_24_skips_jump",
    "negative_timer_result_8_baseline", "zero_timer_result_8_opposite",
    "negative_odd_table_index_1", "zero_odd_table_index_1",
    "negative_even_table_index_24", "zero_even_table_index_24",
    "zero_table_index_28_timer_36", "positive_selector_preserves_raw_facing",
    "timer_2_special_six_far_terminal", "timer_1_far_terminal_signed",
    "timer_0_far_terminal_signed", "timer_8002_far_terminal_signed",
    "timer_8001_wraps_positive", "close_terminal_pass_direction_hold",
    "close_terminal_variant_zero_landing", "close_terminal_variant_six_landing",
    "close_terminal_special_six_signed_ball",
)

GLOBAL_FIELDS = (
    ("rng_raw_07f6",0x07F6),("shot_origin_x_raw_0900",0x0900),
    ("shot_origin_y_raw_0902",0x0902),("ball_record_raw_0910",0x0910),
    ("roster_low_raw_0914",0x0914),("roster_bank_raw_0916",0x0916),
    ("bounce_timer_raw_091c",0x091C),("bounce_raw_0920",0x0920),
    ("previous_actor_x_raw_0922",0x0922),("period_raw_0926",0x0926),
    ("clock_raw_0928",0x0928),
    ("timeout_raw_0930",0x0930),("live_raw_0936",0x0936),
    ("offense_group_raw_093a",0x093A),("owner_raw_093e",0x093E),
    ("pass_actor_raw_0942",0x0942),("pass_aux_raw_0944",0x0944),
    ("pass_receiver_raw_0946",0x0946),("activity_raw_0948",0x0948),
    ("attempt_raw_094a",0x094A),("value_raw_094c",0x094C),
    ("close_timing_raw_094e",0x094E),("dead_raw_0966",0x0966),
    ("height_raw_0968",0x0968),("initial_raw_096a",0x096A),
    ("dead_raw_096c",0x096C),("free_throw_raw_0978",0x0978),
    ("attempts_raw_097a",0x097A),("aim_power_raw_0980",0x0980),
    ("aim_accuracy_raw_0982",0x0982),("inbound_transfer_raw_09b8",0x09B8),
    ("assistance_team_raw_09c0",0x09C0),("pass_active_raw_09c4",0x09C4),
    ("last_owner_raw_09c8",0x09C8),("attachment_raw_09f6",0x09F6),
    ("inner_veto_raw_09f8",0x09F8),("rim_event_raw_13e7",0x13E7),
    ("difficulty_raw_17af",0x17AF),("assistance_raw_17bf",0x17BF),
    ("shot_control_raw_17c3",0x17C3),
    ("rim_force_raw_1866",0x1866),("effect_gate_raw_3f33",0x3F33),
    ("basket_x_raw_3fef",0x3FEF),("basket_y_raw_3ff3",0x3FF3),
    ("effect_resource_raw_4015",0x4015),("effect_raw_401b",0x401B),
    ("effect_frame_raw_4025",0x4025),("effect_timer_raw_402d",0x402D),
    ("display_value_raw_4939",0x4939),("display_shooter_raw_493b",0x493B),
)
ACTOR_OFFSETS = (
    0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x16,0x18,0x1A,
    0x1C,0x1E,0x20,0x22,0x24,0x26,0x28,0x2A,0x2C,0x30,0x32,
    0x38,0x3A,0x3C,0x42,0x44,0x46,0x48,0x4A,0x4C,0x4E,0x50,
    0x52,0x56,0x58,0x5A,0x5E,0x60,0x64,0x66,0x6C,0x6E,0x72,0x7E,0x88,
    0x8A,0x8C,0xA8,0xB0,0xB2,0xBA,0xBC,
)
ACTOR_NAMES = ("actor_native_id_invariant_raw_00",) + tuple(
    f"actor_raw_{offset:02x}" for offset in ACTOR_OFFSETS[1:])
BALL_OFFSETS = (0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12)
BALL_NAMES = tuple(f"ball_raw_{offset:02x}" for offset in BALL_OFFSETS)
CONTEXT_FIELDS = tuple(f"context{c}_raw_{o:02x}" for c in range(2) for o in (0,0x08,0x0A,0x43,0x45,0x47))
PLAYER_FIELDS = tuple(f"player_raw_{o:02x}" for o in (0,2,4,6,8,0x18))
CONTROLLER_FIELDS = tuple(f"controller0_raw_{o:02x}" for o in (0x12,0x14,0x16,0x18,0x1A))
POSE_SNAPSHOT_FIELDS = tuple(f"actor_raw_{o:02x}" for o in (0x34,0x36,0x3E,0x40))
FIELDS = (("slot",) + tuple(n for n,_ in GLOBAL_FIELDS) + ACTOR_NAMES +
          BALL_NAMES + CONTEXT_FIELDS + PLAYER_FIELDS + CONTROLLER_FIELDS +
          ("scratch_raw_0046","scratch_raw_0047") + POSE_SNAPSHOT_FIELDS)

def load_jsonl(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines() if line.strip()]

def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def memory(snapshot):
    raw, covered = bytearray(SIZE), bytearray(SIZE)
    for base,payload in snapshot["mem"].items():
        address=int(base,16); data=bytes.fromhex(payload)
        raw[address:address+len(data)]=data
        covered[address:address+len(data)]=b"\1"*len(data)
    return raw,covered

def word(pair,address):
    raw,covered=pair
    if not (covered[address] and covered[address+1]):
        raise ValueError(f"projection address {address:04x} was not captured")
    return raw[address] | raw[address+1]<<8

def projection(pair):
    slot=word(pair,0xC2); actor=0x34EB+slot*0x100
    values=[slot]
    values.extend(word(pair,a) for _,a in GLOBAL_FIELDS)
    values.extend(word(pair,actor+o) for o in ACTOR_OFFSETS)
    values.extend(word(pair,0x3EEB+o) for o in BALL_OFFSETS)
    for context in (0x46EB,0x476B):
        values.extend(word(pair,context+o) for o in (0,0x08,0x0A,0x43,0x45,0x47))
    native_id=word(pair,actor)
    player=word(pair,0x3435+native_id*2)
    values.extend(word(pair,player+o) for o in (0,2,4,6,8,0x18))
    values.extend(word(pair,0x47EB+o) for o in (0x12,0x14,0x16,0x18,0x1A))
    values.extend((word(pair,0x46),word(pair,0x47)))
    values.extend(word(pair,actor+o) for o in (0x34,0x36,0x3E,0x40))
    assert len(values)==len(FIELDS)
    return values

def listed_starts(path):
    result=set()
    for line in Path(path).read_text().splitlines():
        address=line.split("\t",1)[0].lower()
        if "86a7da" <= address <= "86a991": result.add(address)
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ("vectors","cases","paths","repeat-vectors","repeat-cases","repeat-paths"):
        p.add_argument("--"+name,type=Path,required=True)
    p.add_argument("--ghidra-functions",type=Path,required=True)
    p.add_argument("--ghidra-instructions",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    if a.cases.read_bytes()!=a.repeat_cases.read_bytes() or a.paths.read_bytes()!=a.repeat_paths.read_bytes():
        raise ValueError("repeat case/path capture differs")
    vectors,repeat,cases,paths=map(load_jsonl,(a.vectors,a.repeat_vectors,a.cases,a.paths))
    if not all(len(x)==len(CASE_NAMES) for x in (vectors,repeat,cases,paths)):
        raise ValueError("expected 37 aligned witnesses")
    calls=[]; union=set(); exits=set(); aggregate={}
    for number,(v,r,c,path,name) in enumerate(zip(vectors,repeat,cases,paths,CASE_NAMES),1):
        if (v["call"],c["case"],path["case"],c["name"],path["name"]) != (number,number,number,name,name):
            raise ValueError(f"case {number} alignment changed")
        if v["entry_pc"]!="86a7da" or v["entry_frame"]!=v["exit_frame"] or v["exit_pc"]!=path["exit_pc"]:
            raise ValueError(f"case {number} invalid native boundary")
        if r["call"]!=number or r["entry_pc"]!="86a7da" or r["entry_frame"]!=r["exit_frame"] or r["exit_pc"]!=path["exit_pc"]:
            raise ValueError(f"case {number} invalid repeat boundary")
        before,after,rb,ra=map(memory,(v["entry"],v["exit"],r["entry"],r["exit"]))
        if projection(before)!=projection(rb) or projection(after)!=projection(ra):
            raise ValueError(f"case {number} relevant projection differs")
        slot=projection(before)[0]; actor=0x34EB+slot*0x100
        if word(before,0x96)!=actor or word(before,0xC6)!=2 or word(before,actor+0x5E)!=13:
            raise ValueError(f"case {number} is not a genuine mode-thirteen entry")
        if word(before,actor)!=5 or word(before,0x3435+10)!=0x446B:
            raise ValueError(f"case {number} native roster mapping changed")
        if word(before,0xE0)!=0xF636 or word(before,0xE2)!=0x00AC:
            raise ValueError(f"case {number} native ROM roster pointer changed")
        if word(before,0x4779)!=2:
            raise ValueError(f"case {number} active lineup roster mapping changed")
        if (word(before,0x0926)!=0 or word(before,0x17AF)!=0 or
            word(before,0x46F3)!=0 or word(before,0x4773)!=0):
            raise ValueError(f"case {number} fixed captured launch domain changed")
        if word(before,actor+0x16) not in (0,0xFFFF):
            raise ValueError(f"case {number} controller domain changed")
        executed=path["executed"]
        if not executed or executed[0]!="86a7da" or executed[-1]!=path["exit_pc"]:
            raise ValueError(f"case {number} incomplete PC path")
        union.update(executed); exits.add(path["exit_pc"])
        for child,count in path["child_calls"].items(): aggregate[child]=aggregate.get(child,0)+count
        calls.append({"call":number,"name":name,"entry_pc":v["entry_pc"],"exit_pc":v["exit_pc"],
                      "executed":executed,"child_calls":path["child_calls"],
                      "input":projection(before),"expected":projection(after)})
    expected_aggregate={"restore_869846":8,"cancel_lower_87b555":6,"upper_animation_87b47a":12,
        "pose_87aec3":49,"attach_xy_87b649":27,"attach_z_87b66a":6,"launch_869d6e":6,
        "finish_86a9d0":8,"landing_86986d":8,"attach_point_87b832":49,
        "cancel_pass_86a613":2,"stats_869cdb":14,"effect_87a9e3":3}
    digest=hashlib.sha256("\n".join(sorted(union)).encode()).hexdigest()
    if union!=listed_starts(a.ghidra_instructions) or len(union)!=172 or digest!=EXPECTED_STARTS_SHA256:
        raise ValueError("parent owned-start census changed")
    if exits!={"86a866","86a922","86a92e"} or aggregate!=expected_aggregate:
        raise ValueError(f"exit/child contract changed: {exits} {aggregate}")
    doc={"schema":1,"routine":"$86:A7DA-$A993 mode-thirteen close finish",
         "rom_sha256":ROM_SHA256,
         "provenance":"two controlled genuine-entry Mesen captures with identical relevant projections; documented WRAM inputs only; no PC, stack, ROM, RNG, or child-result patching; inline $86:A994-$A9CF bytes excluded",
         "domain":"represented parent entry/exit state plus direct gameplay child outputs; actor +$00 is identity-only; native slot/id 5 -> $343F -> player $446B and active lineup $4779=2 map to host actor[5], context1, roster slot2/persistent index14 (pack roster address $AC:F636); controllers {-1,0}; captured fixed inputs period $0926=0, difficulty $17AF=0, and both context +$08 basket fractions=0; B832 DP $46/$47 attachment scratch is retained, other DP/stack arithmetic scratch is excluded; table bytes remain pack-backed data",
         "fields":list(FIELDS),"owned_starts_sha256":digest,"aggregate_child_calls":aggregate,
         "source":{"vectors_sha256":sha256(a.vectors),"repeat_vectors_sha256":sha256(a.repeat_vectors),
             "cases_sha256":sha256(a.cases),"paths_sha256":sha256(a.paths),
             "ghidra_functions_sha256":sha256(a.ghidra_functions),
             "ghidra_instructions_sha256":sha256(a.ghidra_instructions)},"calls":calls}
    a.output.write_text(json.dumps(doc,separators=(",",":"))+"\n",encoding="utf-8")
    print(f"[CPU MODE THIRTEEN NORMALIZE] calls=37 fields={len(FIELDS)} starts=172 exits=3")

if __name__=="__main__": main()
