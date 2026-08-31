"""Corrupt parsed views only; native evidence and generated traces stay immutable."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from unittest.mock import patch


def main():
    p=argparse.ArgumentParser()
    for name in ('verifier','native','previous','rom','exe','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(a.verifier.resolve().parent))
    spec=importlib.util.spec_from_file_location('sound_prefix_mutation_target',a.verifier)
    v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
    args=['verify','--native',str(a.native.resolve()),'--previous-native',str(a.previous.resolve()),'--rom',str(a.rom.resolve()),'--exe',str(a.exe.resolve())]
    def run(name):
        with patch.object(sys,'argv',args+['--output',str((a.output/name).resolve())]):v.main()
    run('baseline');reader=v.json_lines
    def reverse(rows,key):
        rows[1],rows[2]=rows[2],rows[1]
        if key:rows[1][key]=1;rows[2][key]=2
    def change(rows,index,key,value):rows[index][key]=value
    def data_read(rows):
        row=next(r for r in rows if r['kind']=='bus' and r['access']==0 and r['address']==0x53)
        row['value']^=1
    def unresolved(rows):
        row=next(r for r in rows if r['kind']=='read' and r['address']==0x822140);row['value']=255
    cases=[('native_bus_clock','sound_prefix_bus.jsonl',lambda r:change(r,0,'master_clock',0),True),
           ('native_bus_pc','sound_prefix_bus.jsonl',lambda r:change(r,0,'pc',0),True),
           ('native_instruction_reorder','sound_prefix_instructions.jsonl',lambda r:reverse(r,'instruction'),True),
           ('native_bus_reorder','sound_prefix_bus.jsonl',lambda r:reverse(r,'event'),True),
           ('native_wrong_scope','sound_prefix_bus.jsonl',lambda r:change(r,0,'scope',2),True),
           ('C_bus_clock','call_01.jsonl',lambda r:change(r,1,'master',0),True),
           ('C_bus_pc','call_01.jsonl',lambda r:change(r,1,'pc',0),True),
           ('C_data_read','call_01.jsonl',data_read,True),
           ('C_instruction_end','call_01.jsonl',lambda r:change(r,1,'end',1),True),
           ('C_mixed_order','call_01.jsonl',lambda r:reverse(r,None),True),
           ('unresolved_value_not_input','sound_prefix_bus.jsonl',unresolved,False)]
    result=[]
    for name,file,mutate,expected in cases:
        mutations=[]
        def altered(path):
            rows=reader(path)
            if path.name==file:
                before=json.dumps(rows,sort_keys=True);mutate(rows)
                assert before!=json.dumps(rows,sort_keys=True),'mutation ineffective'
                mutations.append(file)
            return rows
        try:
            with patch.object(v,'json_lines',side_effect=altered):run(name)
        except (ValueError,AssertionError,KeyError,TypeError)as e:rejected=True;reason=str(e)
        else:rejected=False;reason=''
        assert mutations,'mutation unreachable'
        if not expected and not rejected:
            assert (a.output/name/'call_01.jsonl').read_bytes()==(a.output/'baseline/call_01.jsonl').read_bytes()
            assert (a.output/name/'call_01.wram').read_bytes()==(a.output/'baseline/call_01.wram').read_bytes()
        result.append(dict(name=name,rejected=rejected,expected_rejection=expected,reason=reason,passed=rejected==expected))
        print(name,rejected==expected,flush=True)
    report=dict(passed=all(r['passed']for r in result),cases=result,verifier_sha256=hashlib.sha256(a.verifier.read_bytes()).hexdigest(),native_manifest_sha256=hashlib.sha256((a.native/'manifest.json').read_bytes()).hexdigest())
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
