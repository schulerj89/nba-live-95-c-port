"""Launch a private Mesen process with an explicit per-capture environment."""
import argparse
import os
from pathlib import Path
import subprocess


def environment(inherited,pairs):
    result={key:value for key,value in inherited.items() if not key.startswith('NBA95_')}
    names=set()
    for pair in pairs:
        key,separator,value=pair.partition('=')
        if not separator or not key.startswith('NBA95_') or key in names:
            raise ValueError('invalid or duplicate capture environment')
        names.add(key);result[key]=value
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('executable','cwd','stdout','stderr'):parser.add_argument('--'+key,required=True)
    parser.add_argument('--env',action='append',default=[])
    parser.add_argument('arguments',nargs=argparse.REMAINDER)
    args=parser.parse_args()
    arguments=args.arguments
    if arguments[:1]==['--']:arguments=arguments[1:]
    if not arguments:raise ValueError('missing emulator arguments')
    env=environment(os.environ,args.env)
    # CREATE_NO_WINDOW avoids exposing an incidental console window. No shell
    # constructs arguments, and neither this process nor its parent mutates
    # their global environment during parallel captures.
    with Path(args.stdout).open('wb') as out,Path(args.stderr).open('wb') as err:
        run=subprocess.run([args.executable,*arguments],cwd=args.cwd,env=env,
            stdout=out,stderr=err,creationflags=0x08000000 if os.name=='nt' else 0)
    return run.returncode


if __name__=='__main__':raise SystemExit(main())
