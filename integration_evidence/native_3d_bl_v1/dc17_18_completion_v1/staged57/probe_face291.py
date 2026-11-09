"""Existing pinned runner for fresh complete captured-face bounds and focused controls."""
from pathlib import Path
import os,sys,json,tempfile,hashlib,resource
import run_cases
R=Path(__file__).resolve().parent
assert json.loads((R/'build291_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k/terminal_handles.json').read_text())['all_owned_handles_terminal']
D=R/'m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k/final_face_queries_v1'
for key in list(os.environ):
 if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(SU2_NATIVE_TRANSACTION_REFERENCE=str(D/'m6.su2.native_ref'),SU2_NATIVE_GEOMETRY_QUERY_INPUT=str(D/'faces.bin'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='face291-',dir=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp')
resource.setrlimit(resource.RLIMIT_FSIZE,(128<<20,128<<20))
assert len(sys.argv)==2 and sys.argv[1] in ('faces','focused')
run_cases.CASES['geometry_current_queries']=(1,'[NativeM6FacewiseGeometry3D]' if sys.argv[1]=='faces' else '[NativeFacewiseCertificate3D],[NativeGeometryBackend3D],[NativeAcceptedFaceCertificate3D],[NativeSurfaceChord3D],[NativeFaceCoverage3D],[NativePinchedWitness3D]')
p=Path(__file__);before=hashlib.sha256(p.read_bytes()).hexdigest()
sys.argv=[str(R/'run_cases.py'),'build291','geometry_current_queries']
try:run_cases.main()
finally:assert before==hashlib.sha256(p.read_bytes()).hexdigest()
