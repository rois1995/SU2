"""Three sequential fixed-binary coplanar MPI pilots; require a passed stable full regression first."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import time
root=Path(__file__).resolve().parents[2]
validated=Path(sys.argv[1]).resolve();folder=Path(sys.argv[2]).resolve()
print('Case working and retained folder:',folder,flush=True)
receipt=json.loads((validated/'validation.json').read_text());assert receipt['status']=='PASS' and not receipt['changed_during_run']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
binary=validated/'test_distributed';assert sha(binary)==receipt['binary_sha256']
for p,h in receipt['source_sha256'].items():assert sha(root/p)==h,p
folder.mkdir(exist_ok=False);runner=Path(__file__).resolve();starting_runner=runner.read_bytes()
(folder/'runner_snapshot.py').write_bytes(starting_runner)
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none')
stages=[];observations={};identities={};status='FAIL'
def run(command,label):
    print('Starting',label,flush=True);started=time.monotonic()
    p=subprocess.run(command,cwd=root,env=env,text=True,capture_output=True)
    (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr)
    stages.append(dict(stage=label,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-started))
    assert p.returncode==0,label
    return p.stdout
def host(label):
    for command,suffix in [(['uptime'],'load'),(['free','-m'],'memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'processes')]:
        p=subprocess.run(command,text=True,capture_output=True)
        (folder/(label+'_'+suffix+'.txt')).write_text(p.stdout if suffix!='processes' else '\n'.join(p.stdout.splitlines()[:16])+'\n')
try:
    for repeat in range(3):
        for ranks in (1,2,4):
            label='repeat%d_mpi%d'%(repeat+1,ranks);host('before_'+label)
            output=run(['nice','-n','19','mpirun','--oversubscribe','--bind-to','none','-np',str(ranks),str(binary),'[NativeCoplanar3D]','--use-colour','no'],label)
            assert output.count('All tests passed')==ranks
            result_folder=folder/(label+'_meshes')
            run(['nice','-n','19',sys.executable,'integration_evidence/native_3d_core_v1/audit_coplanar.py',str(folder/(label+'.stdout')),str(result_folder),str(ranks)],'audit_'+label)
            audit=json.loads((result_folder/'independent_audit.json').read_text());assert audit['status']=='PASS'
            identity={p.name:sha(p) for p in result_folder.iterdir() if p.suffix in ('.su2','.json') and p.name!='independent_audit.json'}
            work=sorted(tuple(sorted((k,v) for k,v in d.items() if not k.endswith('_s'))) for d in audit['engine_profiles'])
            if ranks in identities:assert identities[ranks]==(identity,work),'Changed mesh/edit/query workload at MPI'+str(ranks)
            else:identities[ranks]=(identity,work)
            observations[label]=audit
    status='PASS'
finally:
    changed=[p for p,h in receipt['source_sha256'].items() if sha(root/p)!=h]
    if changed or sha(binary)!=receipt['binary_sha256'] or runner.read_bytes()!=starting_runner:status='FAIL'
    summary=[]
    for ranks in (1,2,4):
        samples=[a for name,a in observations.items() if name.endswith('_mpi'+str(ranks))]
        if len(samples)!=3:continue
        for label in sorted({d['case'] for d in samples[0]['events']}):
            values=[max(float(d['parent_s']) for d in a['events'] if d['case']==label) for a in samples]
            engines=[[d for d in a['engine_profiles'] if d['case']==label] for a in samples]
            accepted=[sum(int(d['accepted']) for d in rows) for rows in engines]
            summary.append(dict(ranks=ranks,case=label,parent_rank_max_seconds=values,median=statistics.median(values),minimum=min(values),maximum=max(values),
                accepted=accepted,useful_edits_per_second=[n/t for n,t in zip(accepted,values)],
                maximum_engine_requested_work_bytes=max(int(d['work_bytes']) for rows in engines for d in rows)))
    result=dict(status=status,source_validation_sha256=sha(validated/'validation.json'),binary_sha256=sha(binary),
        runner_sha256=hashlib.sha256(starting_runner).hexdigest(),source_sha256=receipt['source_sha256'],changed_during_run=changed,
        repetitions=3,stages=stages,summary=summary,
        scope='Three sequential MPI1/2/4 fixed-binary repeats with exact mesh/edit/query-work identity within each rank choice, N=M/M<N strict independent audits and closed per-rank fixture phases including migration/worker communicator/return. Parent values are per-rank maxima, not sums of phase maxima. This host is shared; changed partition trajectories across rank choices are not fixed-work scaling. Engine requested bytes are not RSS. Geometry construction is separately reported; no ParMETIS/CFD transfer/CGNS/production affordability qualification.')
    result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
    if status!='PASS':raise RuntimeError('Pilot failed; retained evidence '+str(folder))
