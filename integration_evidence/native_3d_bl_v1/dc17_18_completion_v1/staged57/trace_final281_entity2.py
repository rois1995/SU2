"""One bounded actual final-entity query through the existing pinned runner."""
from pathlib import Path
import hashlib,json,os,sys,tempfile,resource
import run_cases
R=Path(__file__).resolve().parent
assert json.loads((R/'build281_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'cases/geometry_current_queries_20261108T204417_666256/terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'m6_runs/euler_build278_source_chord_certificate_mach_only_real_cfd50k/terminal_handles.json').read_text())['all_owned_handles_terminal']
D=R/'m6_runs/euler_build278_source_chord_certificate_mach_only_real_cfd50k/final_certificate_entity2_v1'
for key in list(os.environ):
 if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(SU2_NATIVE_TRANSACTION_REFERENCE=str(D/'m6.su2.native_ref'),SU2_NATIVE_GEOMETRY_QUERY_INPUT=str(D/'entity2.txt'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
os.environ['OMPI_MCA_osc']='pt2pt'
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='final276-',dir=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp')
resource.setrlimit(resource.RLIMIT_FSIZE,(128<<20,128<<20))
os.environ['SU2_NATIVE_GEOMETRY_WORK_TRACE']='1'
case='geometry_current_queries';run_cases.CASES[case]=(1,'[NativeM6FullGeometry3D]')
p=Path(__file__);before=hashlib.sha256(p.read_bytes()).hexdigest()
sys.argv=[str(R/'run_cases.py'),'build281',case]
try:run_cases.main()
finally:assert before==hashlib.sha256(p.read_bytes()).hexdigest()
