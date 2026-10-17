"""Compile/run published and active-support finite-feature kernels once; bounded serial geometry work only."""
import csv,hashlib,json,os,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
paths=[Path(__file__).resolve(),HERE/'prepare.py',HERE/'replay.cpp',HERE/'legacy_envelope.hpp',HERE/'geometry.bin',ROOT/'Common/include/adaptation/CNativeChordEnvelope.hpp']
proof={str(p.relative_to(ROOT)):sha(p) for p in paths}
if (HERE/'provenance.json').exists():assert json.loads((HERE/'provenance.json').read_text())==proof
else:(HERE/'provenance.json').write_text(json.dumps(proof,indent=2)+'\n')
env=dict(os.environ,TMPDIR=str(ROOT/'integration_evidence/build-integrated-v2/compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
compile=['/usr/bin/time','-v','-o',str(HERE/'compile_resources.txt'),'nice','-n','19','c++','-std=c++17','-O3','-I',str(ROOT),str(HERE/'replay.cpp'),'-o',str(HERE/'replay')]
with (HERE/'compile.log').open('w') as log:subprocess.run(compile,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
start=time.monotonic()
command=['/usr/bin/time','-v','-o',str(HERE/'replay_resources.txt'),'taskset','-c','0','nice','-n','19','timeout','--kill-after=5s','120s',str(HERE/'replay'),str(HERE/'geometry.bin')]
with (HERE/'replay.csv').open('w') as out,(HERE/'replay.stderr').open('w') as err:
    code=subprocess.call(command,env=env,stdout=out,stderr=err)
result=dict(exit_code=code,seconds=time.monotonic()-start,command=command,compile_command=compile,
    binary_sha256=sha(HERE/'replay'),provenance_sha256=sha(HERE/'provenance.json'),csv_sha256=sha(HERE/'replay.csv'))
(HERE/'result.json').write_text(json.dumps(result,indent=2)+'\n')
assert code==0,code
with (HERE/'replay.csv').open() as f:rows=list(csv.DictReader(f))
assert len(rows)==2*json.loads((HERE/'inputs.json').read_text())['edges']
assert all(r['equal']=='1' for r in rows)
summary=[]
for p in ('0','1'):
    group=[r for r in rows if r['pass']==p]
    old=sum(float(r['legacy_seconds']) for r in group);new=sum(float(r['candidate_seconds']) for r in group)
    summary.append(dict(pass_index=int(p),legacy_seconds=old,candidate_seconds=new,ratio=old/new,
        queries=len(group),exhausted_queries=sum(bool(r['failure']) for r in group),active_distance_and_cut_comparison=True,
        max_distance_error=max(float(r['max_distance_error']) for r in group),
        max_active_cut_shift=max(float(r['max_active_cut_shift']) for r in group),
        legacy_cuts=sum(int(r['legacy_cuts']) for r in group),candidate_cuts=sum(int(r['cuts']) for r in group)))
result['summary']=summary;result['limits']='Bounded geometry sample on a contended host; no accepted mesh, seed-wide or lifecycle speedup claim.'
(HERE/'result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(summary,indent=2),flush=True)
