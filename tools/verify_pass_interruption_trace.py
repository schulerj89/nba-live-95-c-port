"""Stream the actual C trace through the C1 interruption/identity guards."""
import argparse,hashlib,json
from pathlib import Path
from pass_interruption_trace import PassInterruptionGuard
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    g=PassInterruptionGuard();old=None;count=0;h=hashlib.sha256();failure=None
    with a.trace.open('rb')as f:
        for line in f:
            h.update(line);r=json.loads(line);count+=1
            if old and count>=220:
                try:
                    g.observe(old,r)
                    v=r['possession'];actor=v['pass_actor_raw'];receiver=v['pass_receiver_raw']
                    if v['pass_active_raw']:
                        if actor<0 or receiver<0:
                            if not(actor==-1 and receiver==-1)and not g.receiver_only_clear(old,r):
                                raise AssertionError('unclassified partial pass identity clear')
                        elif r['actors'][actor]['raw']['control_mode']==15 and not r['actors'][actor]['raw']['pass_released']:
                            if r['actors'][actor]['animation']not in(0x18,*range(0x2a,0x32)):
                                raise AssertionError('executing mode15 has non-pass pose')
                except AssertionError as e:
                    failure={'frame':r['frame'],'error':str(e),'possession':r['possession']};break
            old=r
    report={'trace':str(a.trace.resolve()),'read_bytes_sha256':h.hexdigest(),'frames':count,'entries':g.entries,'recoveries':g.recoveries,'receiver_only_clears':g.receiver_clears,'unfinished':g.interrupted,'first_failure':failure,'passed':failure is None and not g.interrupted and count==63800,'scope':'actual C trace regression only; no native reachability claim'}
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));assert report['passed']
if __name__=='__main__':main()
