import argparse,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--vectors',required=True);p.add_argument('--probe',required=True);p.add_argument('--pack',required=True);a=p.parse_args();expected=json.loads(Path(a.vectors).read_text())['expected']
 fixture=json.loads(Path(a.vectors).read_text());
 if fixture.get('table_init_calls')!=1:raise AssertionError('appearance table census changed')
 r=subprocess.run([a.probe,a.pack],text=True,capture_output=True,check=True);actual=[int(v,16) for v in r.stdout.splitlines()[-1].split()]
 print(f"[GAMEPLAY LOAD] {'PASS' if actual==expected else 'FAIL'}: represented_words={len(expected)}")
 if actual!=expected:print('rom',expected,'port',actual);raise SystemExit(1)
if __name__=='__main__':main()
