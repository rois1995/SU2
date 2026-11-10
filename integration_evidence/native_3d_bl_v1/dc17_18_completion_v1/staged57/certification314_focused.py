"""Focused affected cubic chord behavior, using the existing pinned bounded runner."""
from pathlib import Path
import json,os,sys,tempfile
import run_cases
R=Path(__file__).resolve().parent
assert json.loads((R/'build314_terminal_handles.json').read_text())['all_owned_handles_terminal']
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='chord314-',dir=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp')
run_cases.CASES['geometry_backend']=(1,'[NativeCubicChordSpan3D]')
original_save=run_cases.save
helper=str(Path(__file__).resolve());digest=run_cases.sha(helper)
def save(path,value):
    if Path(path).name=='pins.json':value[helper]=digest
    original_save(path,value)
run_cases.save=save
sys.argv=[run_cases.__file__,'build314','geometry_backend']
run_cases.main()
