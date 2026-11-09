"""Run curved-wall BL checks through the existing provenance/resource-aware runner."""
from pathlib import Path
import hashlib,json,os,sys,tempfile
import run_cases
R=Path(__file__).resolve().parent
assert len(sys.argv)==2 and sys.argv[1] in ('focused','serial','mpi6')
assert json.loads((R/'build303_terminal_handles.json').read_text())['all_owned_handles_terminal']
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
focused='[NativeCurvedMappedGuards3D],[NativeCurvedBL3D],[NativeComposite3D],[NativeVolumeBL3D],[NativeCurvedBLVolume3D],[NativeInteractingBLVolume3D]'
case={'focused':'geometry_backend','serial':'serial','mpi6':'mpi6'}[sys.argv[1]]
ranks,filters=run_cases.CASES[case]
run_cases.CASES[case]=(ranks,'[NativeSharedGeometryBudget3D]' if sys.argv[1]=='focused' else filters+','+focused+',[NativeSharedGeometryBudget3D]'+',[NativeGeometryBackend3D],[NativeParallelBL3D],[NativeReferenceBudget3D],[NativeDeadline3D]')
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
os.environ.setdefault('OMPI_MCA_osc','pt2pt')
os.environ.setdefault('OMPI_MCA_orte_tmpdir_base',tempfile.mkdtemp(prefix='curvedBL303-',dir=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'))
digest=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
sys.argv=[run_cases.__file__,'build303',case]
try:run_cases.main()
finally:assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==digest
