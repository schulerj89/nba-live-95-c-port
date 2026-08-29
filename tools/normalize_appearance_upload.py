"""Compact the natural `$86:E0B0-$E207` gameplay-load result."""
import argparse,json
from pathlib import Path
def word(m,a):return m[a]|m[a+1]<<8
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--tables',required=True);p.add_argument('--output',required=True);a=p.parse_args();rows=list(map(json.loads,Path(a.vectors).read_text().splitlines()))
 if len(rows)!=1:raise AssertionError('expected one gameplay load')
 r=rows[0];m=bytes.fromhex(r['exit']['mem']['0000']);addresses=(0x93e,0x93a,0x97e,0x910,0x952,0x956,0x936,0x92e,0x954,0x3eef,0x3ef3,0x3ef7,0x968,0x9f6,0x3efd,0x3ef9,0x3efb,0x918,0x91a)
 table_rows=list(map(json.loads,Path(a.tables).read_text().splitlines()))
 if len(table_rows)!=1:raise AssertionError('expected one appearance-table init')
 expected=[word(m,z) for z in addresses]
 Path(a.output).write_text(json.dumps({'routine':'$86:E0B0-$E24B gameplay load/resource-list/table result','provenance':'two natural ROM calls in Mesen; no PC/ROM patching','table_init_calls':1,'expected':expected},separators=(',',':'))+'\n')
 print('normalized natural gameplay-load output')
if __name__=='__main__':main()
