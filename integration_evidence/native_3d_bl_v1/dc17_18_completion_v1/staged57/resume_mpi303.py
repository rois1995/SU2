"""Resume only unfinished cases of the connected MPI303 qualification."""
from pathlib import Path
import json,os,sys,tempfile
import run_cases
R=Path(__file__).resolve().parent
prior=R/'cases/mpi6_20261109T173409_3860527'
control=R/'cases/archived296_failures303_mpi6_20261109T174641_3889081'
for folder in (prior,control):
    assert json.loads((folder/'terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((folder/'status.json').read_text())['state']=='terminal'
comparison=R/'mpi303_baseline_failure_comparison.json'
assert json.loads(comparison.read_text())['failure_locations_match']
names=json.loads((prior/'unfinished_test_names.json').read_text())
assert len(names)==len(set(names))==118
filters=','.join(name.replace('\\','\\\\').replace(',','\\,') for name in names)
run_cases.CASES['mpi6_resume303']=(6,filters)
original_save=run_cases.save
extra=[Path(__file__),comparison,prior/'unfinished_test_names.json',prior/'terminal_handles.json',control/'terminal_handles.json']
pins={str(path.resolve()):run_cases.sha(path) for path in extra}
def save(path,value):
    if Path(path).name=='pins.json':value.update(pins)
    original_save(path,value)
run_cases.save=save
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
scratch=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt',TMPDIR=str(scratch),OMPI_MCA_orte_tmpdir_base=tempfile.mkdtemp(prefix='resume-mpi303-',dir=scratch))
sys.argv=[run_cases.__file__,'build303','mpi6_resume303']
run_cases.main()
