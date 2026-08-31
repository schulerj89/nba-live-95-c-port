"""Controlled literal boundary witnesses; no natural reachability claim."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import subprocess


def main():
    p=argparse.ArgumentParser()
    for key in ('probe','assets','output'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();assert not a.output.exists();cases=[]
    def add(name,mode,words,want):cases.append(dict(name=name,mode=mode,words=words,want=want))
    def sbc(a,b,carry):
        total=a+(b^65535)+carry
        return total&65535,int(total>65535)
    edges=[0,1,2,3,4,5,6,7,8,15,16,31,32,63,127,255,256,0x3fff,0x4000,0x7ffe,0x7fff,0x8000,0xfffe,0xffff]
    for fine,movement in itertools.product(edges,repeat=2):
        first,carry=sbc(fine,movement,1);selector,_=sbc(first,movement,carry)
        if selector==0:want=[1,0x2c,1]
        else:want=[1,0x30 if ((selector<3)^(movement<3))else 0x31,65535]
        add('two SBC '+str((fine,movement)),1,[fine,movement,0],want)
    for relative in (1,2,3,0x8000,0xffff):
        for movement in (2,3):add('nonzero relative '+str((relative,movement)),1,[0xffff,movement,relative],[1,0x30 if ((relative<3)^(movement<3))else 0x31,65535])
    def lane():return dict(actors=[[0,0,0],[100,100,0],*[[999,999,0]for _ in range(9)]],order=[65535,*range(11),65535],source=0,receiver=1,sc=1,rc=2)
    def lane_case(name,state,want,ok=1):
        words=[v for row in state['actors']for v in row]+state['order']+[state[k]for k in('source','receiver','sc','rc')]
        add(name,0,[v&65535 for v in words],[ok,want])
    for reverse in (False,True):
        for x,y in itertools.product((-25,-24,-23,0,123,124,125),repeat=2):
            s=lane()
            if reverse:s['actors'][0][:2],s['actors'][1][:2]=[100,100],[0,0]
            s['actors'][2]=[x,y,1];lane_case('half-open '+str((reverse,x,y)),s,int(-24<=x<124 and -24<=y<124))
    for first,last,points in [(0x7ff0,0x800f,[(0x7fd7,0),(0x7fd8,1),(0x7fff,1),(0x8000,1),(0x8026,1),(0x8027,0)]),
                              (0xffff,1,[(0xffe6,0),(0xffe7,1),(0xffff,1),(0,1),(0x18,1),(0x19,0)])]:
        for x,want in points:
            s=lane();s['actors'][0][0]=first;s['actors'][1][0]=last;s['actors'][2]=[x,50,1];lane_case('wrapped X '+str((first,last,x)),s,want)
    s=lane();s['actors'][2]=[999,50,1];s['actors'][3]=[50,50,1];lane_case('forward X miss stops before later blocker',s,0)
    s['actors'][2][2]=0;lane_case('same-team outside X skipped',s,1)
    s=lane();s['order'][3],s['order'][11]=10,2;s['actors'][10]=[999,50,1];s['actors'][3]=[50,50,1];lane_case('ball outside X skipped',s,1)
    s=lane();s['actors'][1][2]=1;lane_case('receiver skipped by cursor',s,0)
    s=lane();s['order']=[65535,3,2,4,0,1,5,6,7,8,9,10,65535];s['sc']=4;s['rc']=5;s['actors'][3]=[50,50,1];s['actors'][2]=[999,50,1];lane_case('backward X miss stops before later blocker',s,0)
    s['actors'][2][2]=0;lane_case('backward same-team skip reaches blocker',s,1)
    for key,value in [('sc',0),('rc',12)]:
        s=lane();s[key]=value;lane_case('invalid cursor '+key,s,0x1234,0)
    # AEDD family write must precede every unexecuted route.
    installs=[('negative relative', [0xffff,0x2c,0,0,0,0,0,0xbeef],[3,0x2c,0xbeef,5]),
              ('near2C promotion',[0,0x2c,0xf0,0,0,0,0,1],[1,0x2f,1,0x2f]),
              ('near2A unchanged',[0,0x2a,0xf0,0,0,0,0,0],[1,0x2a,0,0x2a]),
              ('far stationary both',[0,0x2c,0xf1,0,0,0,0,1],[2,0x2c,1,5]),
              ('far inbound upper',[0,0x2c,0xf1,0x82,0,0,0,1],[1,0x2c,1,0x2c]),
              ('far negative velocity upper',[0,0x2c,0xf1,0,0x8000,0,0,1],[1,0x2c,1,0x2c]),
              ('far nonzero Z upper',[0,0x2c,0xffff,0,0,0,0x8000,1],[1,0x2c,1,0x2c]),
              ('request30 bypasses both',[0,0x30,0xffff,0,0,0,0,0xffff],[1,0x30,0xffff,0x30])]
    for name,words,want in installs:add(name,2,words,want)
    catch=[('all catch',[0,0x18,0xc7,0,0],True),('live equal',[0x80,0x18,0xc7,0,0],False),
           ('live wrapped negative',[0x807f,0x18,0xc7,0,0],False),('live wrapped boundary',[0x8080,0x18,0xc7,0,0],True),
           ('band equal',[0,0x19,0xc7,0,0],False),('anchor equal',[0,0x18,0xc8,0,0],False),
           ('receiverZ',[0,0x18,0xc7,0x8000,0],False),('receiverVZ',[0,0x18,0xc7,0,0xffff],False)]
    for name,words,yes in catch:add(name,3,words,[0,0,0]if yes else[1,0x31,0xffff])
    choices=[('receiver14 skips lane',[14,1,0,0,0,85,110,32],[1,0x2c,1,0x1234]),
             ('clear RNG00',[0,0,0,0,0,85,110,32],[1,0x2b,0xffff,0]),
             ('clear RNG30',[0,0,0x30,0,0,85,110,32],[1,0x2a,0,0]),
             ('blocked ordinary',[0,1,0,0,0,85,110,32],[1,0x2a,0,1]),
             ('blocked inbound layout1',[0,1,0,0x82,1,85,110,32],[1,0x2b,0xffff,1]),
             ('blocked inbound layout2',[0,1,0,0x82,2,85,110,32],[1,0x2c,1,1]),
             ('profile below threshold',[0,1,0,0,0,0x4f,110,32],[1,0x2c,1,1]),
             ('distance below lower',[0,1,0,0,0,0x50,0x2f,32],[1,0x2c,1,1]),
             ('distance lower equality',[0,1,0,0,0,0x50,0x30,32],[1,0x2a,0,1]),
             ('distance upper equality',[0,1,0,0,0,0x50,0x79,32],[1,0x2c,1,1]),
             ('anchor below threshold',[0,1,0,0,0,0x50,110,31],[1,0x2c,1,1])]
    for name,words,want in choices:add(name,4,words,want)
    result=subprocess.run([str(a.probe.resolve()),str(a.assets.resolve())],input='\n'.join(' '.join(f'{v:x}'for v in[c['mode'],*c['words']])for c in cases)+'\n',text=True,capture_output=True,check=True)
    lines=result.stdout.splitlines();assert len(lines)==len(cases)+1 and lines[0].startswith('[ASSETS] Loaded asset pack:')
    failures=[]
    for c,line in zip(cases,lines[1:]):
        c['actual']=[int(x)for x in line.split()]
        if c['actual']!=c['want']:failures.append(c)
    report=dict(passed=not failures,cases=len(cases),failures=failures,observations=cases)
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=not failures,cases=len(cases),failures=failures)))
    return 0 if not failures else 1


if __name__=='__main__':raise SystemExit(main())
