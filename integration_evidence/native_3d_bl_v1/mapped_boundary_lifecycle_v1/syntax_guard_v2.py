import hashlib,json,os,shlex,subprocess,time
from pathlib import Path
root=Path.cwd();build=root/'integration_evidence/build-integrated-v2';out=root/'integration_evidence/native_3d_bl_v1/mapped_boundary_lifecycle_v1/syntax_v2';out.mkdir()
files=['Common/src/adaptation/CNativePredicates3D.cpp','Common/src/adaptation/CNativeAffineReference.cpp','Common/src/adaptation/CNativeBoundaryParameters3D.cpp','Common/src/adaptation/CNativeBoundary3D.cpp','UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
pins={str(p.relative_to(root)):sha(p) for folder in ('Common/include/adaptation','Common/src/adaptation') for p in (root/folder).glob('CNative*') if p.suffix in ('.cpp','.hpp')}
pins.update({p:sha(root/p) for p in files});commands=json.loads((build/'compile_commands.json').read_text())
env=os.environ.copy();env.update(TMPDIR=str(build/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',MALLOC_ARENA_MAX='2')
results=[]
for name in files:
 row=next(r for r in commands if (Path(r['directory'])/r['file']).resolve()==root/name)
 args=row.get('arguments') or shlex.split(row['command'])
 for flag in ('-o','-MF','-MQ'):
  if flag in args:i=args.index(flag);del args[i:i+2]
 for flag in ('-c','-MD','-MMD'):
  if flag in args:args.remove(flag)
 args+=['-fsyntax-only'];started=time.monotonic()
 with (out/(Path(name).name+'.log')).open('w') as log:
  proc=subprocess.run(['nice','-n','19','timeout','--kill-after=5s','180s']+args,cwd=row['directory'],env=env,stdout=log,stderr=subprocess.STDOUT)
 results.append(dict(source=name,exit_code=proc.returncode,seconds=time.monotonic()-started));print(name,proc.returncode,flush=True)
 if proc.returncode:break
assert all(sha(root/p)==h for p,h in pins.items()),'Pinned C++ changed during syntax check'
receipt=dict(status='PASS' if len(results)==len(files) and all(r['exit_code']==0 for r in results) else 'FAILED',steps=results,source_sha256=pins)
(out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n')
raise SystemExit(0 if receipt['status']=='PASS' else 1)
