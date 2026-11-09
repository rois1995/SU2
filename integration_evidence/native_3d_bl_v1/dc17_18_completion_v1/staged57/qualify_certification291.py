"""Seal affected checks without promoting a work-limited face replay to acceptance."""
from pathlib import Path
import hashlib,json,re,sys,time
R=Path(__file__).resolve().parent
assert len(sys.argv)==5
cases=[Path(p).resolve() for p in sys.argv[1:]]
out=R/'certification291_qualification.json';assert not out.exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
pins={};stages=[]
for number,case in enumerate(cases):
    assert case.parent==(R/'cases').resolve()
    status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified'] and not status['changed']
    assert json.loads((case/'terminal_handles.json').read_text())['all_owned_handles_terminal']
    command=json.loads((case/'command.json').read_text())
    assert command['build']=='build291' and command['ranks']==(6 if number==3 else 1)
    text=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text())
    summaries=[list(map(int,x)) for x in re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',text)]
    assert len(summaries)==(6 if number==3 else 1)
    assert all(a>0 and b>=[28,1,91,125][number] for a,b in summaries)
    for p,h in json.loads((case/'pins.json').read_text()).items():assert pins.setdefault(p,h)==h
    for name in ('status.json','terminal_handles.json','command.json','pins.json','run.log'):pins[str(case/name)]=sha(case/name)
    stages.append(dict(case=str(case),status=status,summaries=summaries))
face=json.loads((cases[1]/'native_facewise_geometry_result.json').read_text())
assert not face['complete'] and face['work_exhausted'] and face['work']==face['work_limit']==20000000
assert face['faces']==69544 and face['processed']==17776 and face['over_tolerance']==0
assert face['maximum_upper']<=1e-4
assert json.loads((cases[1]/'comparison290.json').read_text())['exact_processed_prefix_and_counters']
for name in ('native_facewise_geometry_result.json','comparison290.json'):pins[str(cases[1]/name)]=sha(cases[1]/name)
for name in ('build291_status.json','build291_terminal_handles.json','build291_source_pins.json',
             'probe_face291.py','certification291_validate.py','qualify_certification291.py',
             'facewise290_work_diagnosis.json','prepare_final290_face_queries.py'):
    pins[str((R/name).resolve())]=sha(R/name)
build=json.loads((R/'build291_status.json').read_text());assert build['completed_build'] and build['pins_verified']
cfd=R.parents[3]/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD'
assert sha(cfd)==build['production_cfd_sha256_after'];pins[str(cfd)]=sha(cfd)
assert pins=={p:sha(p) for p in pins}
result=dict(time=time.time(),all_required_checks_passed=True,mpi_ranks=6,
    source_pins_sha256=sha(R/'build291_source_pins.json'),stages=stages,
    bounded_face_diagnostic=face,qualification_pins=pins,full_mesh_accepted=False,
    actual_saved_entity_certificate_complete=False,
    scope='Fresh per-face reverse bounds plus mandatory complete global forward proof, original Both fallback on uncertain numeric bounds. Focused and connected serial/MPI checks passed. Actual face diagnostic exhausted unchanged work cap; whole-field certificate UNPROVED. Full same-seed TWO-event M6, transfer, continued adapted CFD and isolated disk restart remain required.')
out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('qualification_pins','stages')}))
