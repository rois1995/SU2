"""Bounded actual M6 cubic-wall policy probe through the existing pinned runner."""
from pathlib import Path
import hashlib,json,os,sys,tempfile,time
import run_cases
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
assert json.loads((R/'build303_terminal_handles.json').read_text())['all_owned_handles_terminal']
receipt=R/'m6_bl_inputs/curved_policy302_v1/provenance.json';prepared=json.loads(receipt.read_text())
inputs={prepared['config']:prepared['config_sha256'],prepared['reference']:prepared['reference_sha256'],str(receipt):sha(receipt),str(Path(__file__)):sha(__file__)}
assert all(sha(p)==h for p,h in inputs.items())
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt',
    SU2_NATIVE_FROZEN_CONFIG=prepared['config'],SU2_NATIVE_BL_REFERENCE=prepared['reference'])
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='actualBL303-',dir=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp')
run_cases.CASES['actual_cubic_bl']=(1,'[NativeActualCubicBLPolicy3D]')
before=set((R/'cases').glob('actual_cubic_bl_*'));started=time.time();sys.argv=[run_cases.__file__,'build303','actual_cubic_bl']
try:run_cases.main()
finally:
    cases=set((R/'cases').glob('actual_cubic_bl_*'))-before;assert len(cases)==1
    case=cases.pop();changed=[p for p,h in inputs.items() if sha(p)!=h]
    (case/'input_provenance.json').write_text(json.dumps(dict(started_at=started,input_pins=inputs,inputs_verified=not changed,changed=changed,scope=prepared['scope']),indent=2)+'\n')
    assert not changed
