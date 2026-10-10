"""Three sequential paired MPI1 fixed-work observations of two pinned engine binaries."""
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time

root=Path(__file__).resolve().parents[2]
baseline,candidate,folder=map(lambda p:Path(p).resolve(),sys.argv[1:])
print('Case working and retained folder:',folder,flush=True)
folder.mkdir(exist_ok=False)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
         OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none')
receipts={name:json.loads((path/'validation.json').read_text()) for name,path in
          [('baseline',baseline),('candidate',candidate)]}
binaries={'baseline':baseline/'test_distributed','candidate':candidate/'test_distributed'}
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
initial={name:digest(path) for name,path in binaries.items()}
rows=[];stages=[];mesh=None;work=None;status='FAIL'
try:
    for name,receipt in receipts.items():
        assert receipt['status']=='PASS' and not receipt['changed_during_run']
        assert initial[name]==receipt['binary_sha256']
    assert all(digest(root/p)==h for p,h in receipts['candidate']['source_sha256'].items())
    for trial in range(3):
        order=['baseline','candidate'] if trial%2==0 else ['candidate','baseline']
        for name in order:
            label=name+'_'+str(trial)
            for command,suffix in [(['uptime'],'load'),(['free','-m'],'memory'),
                                   (['ps','-eo','pid,ppid,ni,pcpu,args','--sort=-pcpu'],'processes')]:
                host=subprocess.run(command,text=True,capture_output=True)
                (folder/(label+'_'+suffix+'.txt')).write_text(host.stdout if suffix!='processes' else
                    '\n'.join(host.stdout.splitlines()[:16])+'\n')
            command=['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np','1',str(binaries[name]),
                     'Native incremental MPI refinement reaches complete frozen metric gates','--use-colour','no']
            print('Starting',label,flush=True)
            start=time.monotonic()
            p=subprocess.run(command,cwd=root,env=env,text=True,capture_output=True)
            elapsed=time.monotonic()-start
            (folder/(label+'.stdout')).write_text(p.stdout)
            (folder/(label+'.stderr')).write_text(p.stderr)
            stages.append(dict(stage=label,command=command,exit_code=p.returncode,seconds=elapsed))
            assert p.returncode==0 and 'All tests passed' in p.stdout,label+' failed'
            lines=p.stdout.splitlines()
            engine=[dict(x.split('=',1) for x in line.split()[1:]) for line in lines if line.startswith('ENGINE ')]
            assert len(engine)==1 and engine[0]['valid']=='1'
            current=[line for line in lines if line.startswith('MESH engine_full ')]
            assert len(current)==48
            accepted={k:engine[0][k] for k in ['initial','updated','final','rounds','proposed','attempts','accepted','rejected']}
            if mesh is None:mesh=current;work=accepted
            assert mesh==current and work==accepted,'Fixed work or exact mesh differs'
            rows.append(dict(variant=name,trial=trial,profile=engine[0],external_test_seconds=elapsed,
                             accepted_edits_per_engine_second=int(engine[0]['accepted'])/float(engine[0]['total_s'])))
    assert all(digest(path)==initial[name] for name,path in binaries.items())
    assert all(digest(root/p)==h for p,h in receipts['candidate']['source_sha256'].items())
    status='PASS'
finally:
    summary={}
    for name in binaries:
        times=[float(row['profile']['total_s']) for row in rows if row['variant']==name]
        if times:summary[name]=dict(minimum_s=min(times),median_s=statistics.median(times),maximum_s=max(times))
    result=dict(status=status,rows=rows,stages=stages,summary=summary,binary_sha256=initial,
                receipt_sha256={name:digest(path/'validation.json') for name,path in [('baseline',baseline),('candidate',candidate)]},
                script_sha256=digest(Path(__file__)),mesh_sha256=hashlib.sha256('\n'.join(mesh or []).encode()).hexdigest(),
                fixed_work=work,scope='Three paired sequential local MPI1 engine profile observations on identical accepted work and exact mesh, under observed uncontrolled host contention. The external test includes MPI startup, audits and an extra detail-OFF event. Engine profiles exclude final object destruction and do not cover CFD, metric preparation, ParMETIS, return/rebuild, solution transfer or output. No general scaling or affordability claim.')
    result['artifact_sha256']={p.name:digest(p) for p in folder.iterdir() if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(status,summary,flush=True)
