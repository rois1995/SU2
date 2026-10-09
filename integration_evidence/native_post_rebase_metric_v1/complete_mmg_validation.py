from pathlib import Path
import hashlib, json, os, re, shlex, shutil, signal, subprocess, sys, time

root = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
os.chdir(root)
sys.path.insert(0,str(root/'integration_evidence'))
from native_process_sampler import compute_processes
out=root/'integration_evidence/native_post_rebase_metric_v1'
build=out/'build_v2'
receipt=json.loads((build/'evidence.json').read_text())
assert receipt['status']=='PASS'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(build/'SU2_CFD')==receipt['binary_sha256'] and sha(build/'test_driver')==receipt['test_driver_sha256']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none')
record=dict(status='RUNNING',scope='Merged-source correctness completion; four CPUs avoid two-core MPI oversubscription; no performance assessment',
            controller_pid=os.getpid(),build_sha256=sha(build/'evidence.json'),stages=[],machine_samples=[])
def save():
    (out/'validation_mmg_final_completion.json').write_text(json.dumps(record,indent=2)+'\n')
def cpu():
    return list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:9]))
def gate(ranks):
    return
def run(label, command, cwd, footprint=1, timeout=300, extra_env=None):
    cwd.mkdir(parents=True,exist_ok=True)
    gate(footprint)
    assert all(sha(root/name)==digest for name,digest in receipt['source_sha256'].items())
    row=dict(label=label,command=command,working_directory=str(cwd),status='RUNNING')
    record['stages'].append(row);record.update(phase=label);save()
    with (cwd/(label+'.log')).open('x') as log:
        child=subprocess.Popen(['taskset','-c','4,5,6,7','nice','-n','10',*command],cwd=cwd,env=dict(env,**(extra_env or {})),
                               stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        record['child_pid']=child.pid;save()
        try:code=child.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid,signal.SIGTERM)
            try:child.wait(timeout=10)
            except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
            code=124
    record.pop('child_pid',None)
    row.update(status='PASS' if code==0 else 'FAIL',exit_code=code,log_sha256=sha(cwd/(label+'.log')))
    text=(cwd/(label+'.log')).read_text()
    row['test_summaries']=re.findall(r'All tests passed \(([^\n]+)\)',text)
    save();print(label,code,row['test_summaries'],flush=True)
    if code:
        record.update(status='FAIL',phase='terminal');save();raise SystemExit(code)
    return text
def mpi(ranks,binary,*tail):
    return ['mpirun','--host','localhost:4','-np',str(ranks),str(binary),*tail]

for row in json.loads((out/'prepared_smokes.json').read_text()):
    if row['kind'] not in ('PREDICT','FIXED_POINT'): continue
    source=root/row['case'];case=out/('vortex-'+row['kind'].lower().replace('_','-')+'-mmg-n4-v2')
    if case.exists():
        assert row['kind']=='PREDICT'
        text=(case/'solver.log').read_text()
    else:
        case.mkdir(exist_ok=False)
        shutil.copy2(source/'input.su2',case/'input.su2')
        cfg=(source/'run.cfg').read_text()
        cfg=re.sub(r'^ADAP_NATIVE_(RANKS|REPARTITION)\s*=.*\n?', '', cfg,flags=re.M)
        cfg=re.sub(r'^ADAP_REMESHER\s*=.*$', 'ADAP_REMESHER= MMG',cfg,flags=re.M)
        (case/'run.cfg').write_text(cfg)
        text=run('solver',mpi(4,build/'SU2_CFD','run.cfg'),case,4,timeout=600)
        assert 'Exit Success' in text
        (case/'run_evidence.json').write_text(json.dumps(dict(phase='terminal',solver_exit=0,ranks=4,
               binary_sha256=receipt['binary_sha256'],scope='Bounded unsteady correctness smoke; no timing assessment'),indent=2)+'\n')
    if row['kind']!='FIXED_POINT':
        run('independent_mmg_unsteady_v2',['python3',str(out/'check_mmg_unsteady.py'),str(case)],case)
    if row['kind']=='PREDICT':
        assert re.findall(r'Metric snapshot of time step (\d+)',text)==['0','2','3','5','6','8']
    elif row['kind']=='FIXED_POINT':
        assert len(re.findall(r'Metric of the time window: mean \|Hessian\|.*over 3 time steps',text))==2
    else:
        assert len(re.findall(r'Metric of the time window: mean \|Hessian\|.*over 3 time steps',text))==3
record.update(status='PASS',phase='terminal');save()
print('INTEGRATED CORRECTNESS VALIDATION PASS',len(record['stages']),'stages; no local speedup claim',flush=True)
