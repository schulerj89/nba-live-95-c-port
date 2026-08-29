"""Guard the native whole-pass census and its portable output witnesses."""
import argparse,json
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--whole',required=True);p.add_argument('--preparation',required=True);p.add_argument('--direction',required=True);a=p.parse_args()
 whole=json.loads(Path(a.whole).read_text())['calls'];prep=json.loads(Path(a.preparation).read_text())['calls'];direction=json.loads(Path(a.direction).read_text())['calls']
 if len(whole)!=200 or len(prep)!=2000 or len(direction)!=2000:raise AssertionError('player draw witness census changed')
 if any(c['exit_pc'].lower()!='87a845' for c in whole):raise AssertionError('whole draw return changed')
 if len({tuple(c['queue_counts']) for c in whole})<8:raise AssertionError('native queue diversity collapsed')
 if len({tuple(c['presentation_copy']) for c in whole})<40:raise AssertionError('native presentation-copy diversity collapsed')
 if len({(c['input'][8],c['input'][9]) for c in prep})!=175:raise AssertionError('resource diversity collapsed')
 if len({c['input'][1] for c in direction})!=5:raise AssertionError('mode diversity collapsed')
 print(f"[PLAYER DRAW PIPELINE] PASS: whole={len(whole)} calls, queues={len({tuple(c['queue_counts']) for c in whole})}, copies={len({tuple(c['presentation_copy']) for c in whole})}, prepared={len(prep)}")
if __name__=='__main__':main()
