"""Seal connected boundary-reuse checks; the complete first-adaptation replay is separate."""
from pathlib import Path
import hashlib,json,re,sys,time
R=Path(__file__).resolve().parent
assert len(sys.argv)==4
cases=[Path(p).resolve() for p in sys.argv[1:]]
out=R/'boundary296_qualification.json';assert not out.exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
pins={};stages=[]
for number,case in enumerate(cases):
    assert case.parent==(R/'cases').resolve()
    status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified'] and not status['changed']
    assert json.loads((case/'terminal_handles.json').read_text())['all_owned_handles_terminal']
    command=json.loads((case/'command.json').read_text())
    ranks=6 if number==2 else 1
    assert command['build']=='build296' and command['ranks']==ranks
    selectors=' '.join(command['argv'])
    assert '[NativeGuidanceReuse3D]' in selectors and '[NativeSmoothingHistory3D]' in selectors
    log=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text())
    summaries=[list(map(int,x)) for x in re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',log)]
    assert len(summaries)==ranks and all(a>0 and b>=[10,99,132][number] for a,b in summaries)
    for p,h in json.loads((case/'pins.json').read_text()).items():assert pins.setdefault(p,h)==h
    for name in ('status.json','terminal_handles.json','command.json','pins.json','run.log'):pins[str(case/name)]=sha(case/name)
    stages.append(dict(case=str(case),status=status,summaries=summaries))
for name in ('build296_status.json','build296_terminal_handles.json','build296_source_pins.json',
             'certification296_validate.py','qualify_boundary296.py','run_m6.py'):
    pins[str((R/name).resolve())]=sha(R/name)
build=json.loads((R/'build296_status.json').read_text());assert build['completed_build'] and build['pins_verified']
cfd=R.parents[3]/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD'
assert sha(cfd)==build['production_cfd_sha256_after'];pins[str(cfd)]=sha(cfd)
assert pins=={p:sha(p) for p in pins}
result=dict(time=time.time(),all_required_checks_passed=True,mpi_ranks=6,
    source_pins_sha256=sha(R/'build296_source_pins.json'),stages=stages,qualification_pins=pins,
    full_mesh_accepted=False,performance_benefit_established=False,
    scope='Connected immutable old-star preparation, exact unchanged entity-face numeric guidance and dependency-specific rejected-star invalidation. Fresh changed-position evaluation and strict final proof remain mandatory. Focused/connected serial and one MPI6 milestone qualify changed behavior, not performance or complete M6 acceptance.')
out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('qualification_pins','stages')}))
