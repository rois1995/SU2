"""One sequential interface/syntax pass using the configured production compile commands."""
import hashlib,json,os,shlex,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];BUILD=ROOT/'integration_evidence/build-integrated-v2'
commands=json.loads((BUILD/'compile_commands.json').read_text());records=[]
for name in ('CSinglezoneDriver.cpp','CNativeImport3D_tests.cpp'):
 row=next(x for x in commands if x['file'].endswith('/'+name));args=shlex.split(row['command']);clean=[];skip=False
 for arg in args:
  if skip:skip=False;continue
  if arg in ('-o','-MF','-MQ','-MT'):skip=True;continue
  if arg in ('-c','-MD','-MMD','-MP'):continue
  clean.append(arg)
 command=['nice','-n','19','timeout','--kill-after=5s','120s']+clean+['-fsyntax-only','-O0','-g0']
 env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(BUILD/'compile-tmp'))
 start=time.monotonic()
 with (HERE/('syntax_interfaces_'+name+'.log')).open('w') as out:result=subprocess.run(command,cwd=row['directory'],env=env,stdout=out,stderr=subprocess.STDOUT)
 report={'file':row['file'],'command':command,'exit_code':result.returncode,'seconds':time.monotonic()-start,
  'source_sha256':hashlib.sha256((Path(row['directory'])/row['file']).resolve().read_bytes()).hexdigest()}
 records.append(report);(HERE/'syntax_interfaces.json').write_text(json.dumps(records,indent=2)+'\n');print(name,result.returncode,flush=True)
 if result.returncode:raise SystemExit(result.returncode)
