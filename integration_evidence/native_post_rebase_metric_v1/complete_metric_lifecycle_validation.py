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
    (out/'validation_lifecycle_completion.json').write_text(json.dumps(record,indent=2)+'\n')
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

commands=json.loads((root/'integration_evidence/build-integrated-v2/compile_commands.json').read_text())
item=next(c for c in commands if c['file'].endswith('/solvers/CSolver.cpp'))
original=item.get('arguments') or shlex.split(item['command'])
args=[];i=0
while i<len(original):
    if original[i] in ('-o','-MF','-MQ','-MT'):i+=2;continue
    if original[i] in ('-c','-MD','-MMD'):i+=1;continue
    args.append(original[i]);i+=1
args+=['-fsyntax-only','-I'+str(root/'externals/codi/include'),
       '-I/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/externals/medi/include',
       '-I/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/externals/opdi/include']
for mode in ('REVERSE',):
    extra=['-DCODI_'+mode+'_TYPE']
    if mode=='REVERSE':extra+=['-DCODI_JACOBIAN_LINEAR_TAPE']
    run('CSolver_AD_'+mode.lower()+'_no_omp',args+extra,Path(item['directory']))

case=out/'rae-steady-smoke-n4m3'
text=run('solver',mpi(4,build/'SU2_CFD','run.cfg'),case,4,timeout=600)
assert 'Exit Success' in text
(case/'run_evidence.json').write_text(json.dumps(dict(phase='terminal',solver_exit=0,ranks=4,
       binary_sha256=receipt['binary_sha256'],scope='One remesh, one flow step per mesh; correctness only'),indent=2)+'\n')
run('pairing',['python3',str(root/'integration_evidence/inspect_rae2822_solution.py'),str(case),'--cycles','0','1'],case)
run('metric_audit',['python3',str(root/'integration_evidence/audit_native_composite_rae.py'),str(case),'--cycles','1'],case,timeout=600)

for fixture in ('frozen_euler_to_bl','frozen_bl_to_euler'):
    case=out/(fixture+'-n4m3');case.mkdir(exist_ok=False)
    source=root/'integration_evidence/native_cluster_campaign_v1/inputs'/fixture
    for p in source.iterdir():shutil.copy2(p,case/p.name)
    with (case/'run.cfg').open('a') as f:f.write('\nADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= 3\n')
    text=run('solver',mpi(4,build/'test_driver','[NativeFrozenAirfoil2D]','--use-colour','no'),case,4,timeout=600,
            extra_env=dict(SU2_NATIVE_AIRFOIL_CONFIG=str(case/'run.cfg'),SU2_NATIVE_FROZEN_METRIC=str(case/'frozen_sensor.csv')))
    assert text.count('All tests passed')==4
    (case/'run_evidence.json').write_text(json.dumps(dict(command=['mpirun','-n','4'],
            inputs_sha256={'frozen_sensor.csv':sha(case/'frozen_sensor.csv')}),indent=2)+'\n')
    name='rae_euler_to_bl' if fixture.endswith('euler_to_bl') else 'rae_bl_to_euler'
    run('independent_frozen',['python3',str(root/'integration_evidence/audit_native_frozen_case.py'),str(case),name,'--self-contained'],case,timeout=600)

for row in json.loads((out/'prepared_smokes.json').read_text()):
    case=root/row['case']
    text=run('solver',mpi(4,build/'SU2_CFD','run.cfg'),case,4,timeout=600)
    assert 'Exit Success' in text
    (case/'run_evidence.json').write_text(json.dumps(dict(phase='terminal',solver_exit=0,ranks=4,
           binary_sha256=receipt['binary_sha256'],scope='Bounded unsteady correctness smoke; no timing assessment'),indent=2)+'\n')
    if row['kind']!='FIXED_POINT':
        run('independent_unsteady',['python3',str(root/'integration_evidence/audit_native_unsteady.py'),str(case)],case)
    if row['kind']=='PREDICT':
        assert re.findall(r'Metric snapshot of time step (\d+)',text)==['0','2','3','5','6','8']
    elif row['kind']=='FIXED_POINT':
        assert len(re.findall(r'Metric of the time window: mean \|Hessian\|.*over 3 time steps',text))==2
    else:
        assert len(re.findall(r'Metric of the time window: mean \|Hessian\|.*over 3 time steps',text))==3
record.update(status='PASS',phase='terminal');save()
print('INTEGRATED CORRECTNESS VALIDATION PASS',len(record['stages']),'stages; no local speedup claim',flush=True)
