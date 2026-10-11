"""Check the configured passive predicate TU under scalar and DD/AD macros."""
from pathlib import Path
import hashlib,json,os,shlex,subprocess,sys,time
root=Path(sys.argv[1]).resolve();folder=Path(sys.argv[2]).resolve();folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
build=root/'integration_evidence/build-integrated-v2'
entry=next(r for r in json.loads((build/'compile_commands.json').read_text()) if r['file'].endswith('CNativePredicates3D.cpp'))
args=shlex.split(entry['command']);command=[];i=0
while i<len(args):
 if args[i] in ('-MQ','-MF','-o'):i+=2
 elif args[i] in ('-MD','-c'):i+=1
 else:command.append(args[i]);i+=1
predicate=root/'Common/src/adaptation/CNativePredicates3D.cpp';header=root/'Common/include/adaptation/CNativeMesh3D.hpp'
pins={str(p):sha(p) for p in (predicate,header)};stages=[];status='FAIL'
(folder/'check.py').write_bytes(Path(__file__).read_bytes())
try:
 for mode,macro in [('scalar',None),('DD-macro','CODI_FORWARD_TYPE'),('AD-macro','CODI_REVERSE_TYPE')]:
  argv=['nice','-n','19']+command+['-fsyntax-only']+(['-D'+macro] if macro else [])
  start=time.monotonic();p=subprocess.run(argv,cwd=entry['directory'],capture_output=True,text=True,timeout=120,
   env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1'))
  (folder/(mode+'.stdout')).write_text(p.stdout);(folder/(mode+'.stderr')).write_text(p.stderr)
  stages.append(dict(mode=mode,command=argv,cwd=entry['directory'],seconds=time.monotonic()-start,exit_code=p.returncode))
  assert p.returncode==0,mode
 assert all(sha(Path(k))==v for k,v in pins.items())
 status='PASS'
finally:
 out=dict(status=status,source_sha256=pins[str(predicate)],header_sha256=pins[str(header)],stages=stages,
  scope='Configured CNativePredicates3D.cpp passive double/long-double TU only; CoDi macros do not introduce active arithmetic here. No AD solver link/run or derivative claim.',
  artifact_sha256={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()})
 (folder/'validation.json').write_text(json.dumps(out,indent=2)+'\n');print(status,folder,flush=True)
