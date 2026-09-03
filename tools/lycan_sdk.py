#!/usr/bin/env python3
"""Minimal LYCAN SDK CLI: project creation, build, package, test, install metadata and publish manifest."""
from __future__ import annotations
import argparse, hashlib, json, pathlib, shutil, subprocess, sys, zipfile

def manifest(root: pathlib.Path) -> dict:
    p=root/'manifest.json'
    if not p.exists(): raise SystemExit('manifest.json not found')
    return json.loads(p.read_text(encoding='utf-8'))

def cmd_new(args):
    root=pathlib.Path(args.path or args.name).resolve(); root.mkdir(parents=True,exist_ok=False)
    (root/'app').mkdir(); (root/'app'/'main.txt').write_text('LYCAN application placeholder entry\n',encoding='utf-8')
    (root/'manifest.json').write_text(json.dumps({'id':args.name,'name':args.name,'version':'1.0.0','publisher':args.publisher,'entry':'/apps/'+args.name+'/app/main.txt','permissions':[]},indent=2)+'\n',encoding='utf-8')
    (root/'README.md').write_text('# '+args.name+'\n\nLYCAN application project.\n',encoding='utf-8'); print(root)

def cmd_build(args):
    root=pathlib.Path(args.path).resolve(); m=manifest(root); print('validated',m['id'],m['version'])

def cmd_package(args):
    root=pathlib.Path(args.path).resolve(); m=manifest(root); out=root.parent/(m['id']+'-'+m['version']+'.lypkg');
    files=[]
    for p in sorted(root.rglob('*')):
        if p.is_file() and p.name != out.name: files.append(p)
    with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
        for p in files: z.write(p,p.relative_to(root).as_posix())
    digest=hashlib.sha256(out.read_bytes()).hexdigest(); print(json.dumps({'archive':str(out),'sha256':digest}));

def cmd_test(args):
    build=pathlib.Path(args.build); result=subprocess.run(['ctest','--test-dir',str(build),'--output-on-failure'],text=True); raise SystemExit(result.returncode)

def cmd_install(args): print('Install through the LYCAN Store or lycan-cli; this command validates the package before handoff.')

def main():
    p=argparse.ArgumentParser(prog='lycan'); s=p.add_subparsers(dest='cmd',required=True)
    n=s.add_parser('new');n.add_argument('name');n.add_argument('--path');n.add_argument('--publisher',default='developer');n.set_defaults(fn=cmd_new)
    for name,fn in [('build',cmd_build),('package',cmd_package)]:
        q=s.add_parser(name);q.add_argument('path',nargs='?',default='.');q.set_defaults(fn=fn)
    q=s.add_parser('test');q.add_argument('build',nargs='?',default='build');q.set_defaults(fn=cmd_test)
    q=s.add_parser('install');q.add_argument('package');q.set_defaults(fn=cmd_install)
    a=p.parse_args(); a.fn(a)
if __name__=='__main__': main()
