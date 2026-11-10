"""Focused affected cubic chord behavior, using the existing pinned bounded runner."""
from pathlib import Path
import json,os,sys,tempfile
import run_cases
R=Path(__file__).resolve().parent
assert json.loads((R/'build319_terminal_handles.json').read_text())['all_owned_handles_terminal']
previous=R/'m6_runs/rans_curved314_v1/terminal_handles_actual.json'
assert json.loads(previous.read_text())['all_owned_handles_terminal']
cell=R/'cases/cubic_cell_replay319_v1'
assert json.loads((cell/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
assert json.loads((cell/'status.json').read_text())['state']=='terminal'
cell_status=json.loads((cell/'status.json').read_text())
assert cell_status['exit_code'] in (0,124) and cell_status['pins_verified']
if cell_status['exit_code']==124:
    observed=json.loads((cell/'assessment.json').read_text())
    assert observed['outcome']=='DIAGNOSTIC_TIMEOUT_UNRESOLVED' and observed['passed'] is False
    assert observed['diagnostic_limit_seconds']==1800 and observed['native_limit_seconds']==5400
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='chord319-',dir=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp')
run_cases.CASES['curved319_mpi6']=(6,'[NativeCubicChordSpan3D],[NativeD1Law],[NativeCurvedBL3D],[NativeHeightOptimization3D],[NativeConcurrentComposed3D],[NativeConcurrentRepair3D],[NativeRepairBatch3D],[NativePhaseOwnership3D],[NativeGeometryBackend3D],[NativeClosestPrepared3D],[NativePreparedSource3D],[NativeSurfaceMemo3D]')
original_save=run_cases.save
original_sample=run_cases.sample_tree
seen={}
attempt=None
def identity(pid):
    try:return int(Path(f'/proc/{pid}/stat').read_text().rsplit(')',1)[1].split()[19])
    except (FileNotFoundError,ProcessLookupError):return None
def sample(pid):
    rows=original_sample(pid)
    for row in rows:
        row['start_ticks']=identity(row['pid'])
        if row['start_ticks'] is not None:seen[(row['pid'],row['start_ticks'])]=dict(row)
    return rows
run_cases.sample_tree=sample
helper=str(Path(__file__).resolve());digest=run_cases.sha(helper)
def save(path,value):
    global attempt
    attempt=Path(path).parent
    if Path(path).name=='pins.json':
        value[helper]=digest
        value[str(previous)]=run_cases.sha(previous)
    if Path(path).name=='status.json' and value.get('state')=='running':
        value['start_ticks']=identity(value['pid'])
        value['wrapper']={'pid':os.getpid(),'start_ticks':identity(os.getpid())}
    original_save(path,value)
run_cases.save=save
sys.argv=[run_cases.__file__,'build319','curved319_mpi6']
try:run_cases.main()
finally:
    if attempt:
        live=[row for (pid,ticks),row in seen.items() if identity(pid)==ticks]
        original_save(attempt/'terminal_handles.json',{'all_owned_handles_terminal':not live,'observed_owned_handles':list(seen.values()),'remaining_owned_handles':live,'scope':'Sampled PID/start identities; parent independently seals actual handles.'})
