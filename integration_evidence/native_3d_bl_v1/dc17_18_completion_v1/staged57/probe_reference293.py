"""Pinned existing runners for bounded reference roundtrip and expected write refusal."""
from pathlib import Path
import hashlib,json,os,resource,sys,tempfile
import run_cases
R=Path(__file__).resolve().parent
assert json.loads((R/'build293_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'m6_runs/euler_build291_facewise_certificate_mach_only_real_cfd50k/terminal_handles.json').read_text())['all_owned_handles_terminal']
assert len(sys.argv)==2 and sys.argv[1] in ('focused','authority','refusal')
mode=sys.argv[1]
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
D=R/'m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k/final_face_queries_v1'
os.environ.update(SU2_NATIVE_TRANSACTION_REFERENCE=str(D/'m6.su2.native_ref'),
    SU2_NATIVE_GEOMETRY_QUERY_INPUT=str(D/'faces.bin'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',
    MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt')
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='reference293-',dir=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp')
resource.setrlimit(resource.RLIMIT_FSIZE,(128<<20,128<<20))
case='reference_write_refusal' if mode=='refusal' else 'geometry_current_queries'
filters={'focused':'[NativeReferenceIO3D],[NativeAtlasReferenceIO3D],[NativeReferenceBudget3D],[NativeGeometryBackend3D]',
         'authority':'[NativeM6ReferencePublication3D]','refusal':'[NativeReferenceWriteRefusal3D]'}
run_cases.CASES[case]=(1,filters[mode])
before_cases=set((R/'cases').iterdir())
before=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
sys.argv=[str(R/'run_cases.py'),'build293',case]
try:
    run_cases.main()
except SystemExit as error:
    if mode!='refusal':raise
    new_cases=set((R/'cases').iterdir())-before_cases;assert len(new_cases)==1
    attempt=new_cases.pop();status=json.loads((attempt/'status.json').read_text())
    assert error.code==status['exit_code']==1 and status['pins_verified'] and not status['changed']
    assert 'Native3D reference encoding retained state exceeds storage budget.' in (attempt/'run.log').read_text()
    assert (attempt/'native3d_write_refusal.native_ref').read_text()=='prior-published-reference\n'
    assert not (attempt/'native3d_write_refusal.native_ref.tmp').exists()
    result=dict(expected_refusal_validated=True,solver_exit_code=1,prior_file_unchanged=True,
        no_temporary_publication=True,scope='Dedicated negative write case; nonzero solver exit is required, not an accepted mesh or passing ordinary unit case.')
    (attempt/'expected_refusal_validated.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
finally:
    assert before==hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
