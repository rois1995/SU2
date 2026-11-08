"""One pinned serial actual-cavity source/geometry diagnostic; no CFD or MPI campaign."""
from pathlib import Path
import hashlib,json,os,sys,tempfile
import run_cases
R=Path(__file__).resolve().parent;receipt=R/'closure265_serial_wrapper.json';assert not receipt.exists()
assert json.loads((R/'build265_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'build265_status.json').read_text())['production_cfd_byte_identical']
for key in list(os.environ):
 if key.startswith('SU2_NATIVE_'):del os.environ[key]
scratch=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(scratch),OMPI_MCA_osc='pt2pt')
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='closure265-',dir=scratch)
os.environ['SU2_NATIVE_FROZEN_CONFIG']=str(R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/current_witnesses_v1/run.cfg')
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest();before=sha(__file__)
previous=set((R/'cases').glob('current_production_*'));sys.argv=[str(Path(run_cases.__file__)),'build265','current_production']
try:run_cases.main()
finally:
 assert before==sha(__file__);created=set((R/'cases').glob('current_production_*'))-previous;assert len(created)==1
 receipt.write_text(json.dumps({'case':str(next(iter(created))),'wrapper_sha256_before':before,'wrapper_sha256_after':before,'scope':'Oneactual42cell context;unknownoracles explicit,production current-source rebasing,existingoperators/budgets/gates.Nocapability/acceptanceclaim.'},indent=2)+'\n')
