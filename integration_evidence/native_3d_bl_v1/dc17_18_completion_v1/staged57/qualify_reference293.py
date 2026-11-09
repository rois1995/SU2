"""Seal bounded persistence checks, explicitly distinguishing the expected fatal refusal."""
from pathlib import Path
import hashlib,json,re,sys,time
R=Path(__file__).resolve().parent
assert len(sys.argv)==6
cases=[Path(p).resolve() for p in sys.argv[1:]]
out=R/'reference293_qualification.json';assert not out.exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
pins={};stages=[]
for number,case in enumerate(cases):
    assert case.parent==(R/'cases').resolve()
    status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['exit_code']==(1 if number==2 else 0)
    assert status['pins_verified'] and not status['changed']
    assert json.loads((case/'terminal_handles.json').read_text())['all_owned_handles_terminal']
    command=json.loads((case/'command.json').read_text())
    assert command['build']=='build293' and command['ranks']==(6 if number==4 else 1)
    log=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text())
    summaries=[list(map(int,x)) for x in re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',log)]
    if number==2:
        refusal=json.loads((case/'expected_refusal_validated.json').read_text())
        assert refusal['expected_refusal_validated'] and refusal['prior_file_unchanged'] and refusal['no_temporary_publication']
        assert not summaries and (case/'native3d_write_refusal.native_ref').read_text()=='prior-published-reference\n'
        for name in ('expected_refusal_validated.json','native3d_write_refusal.native_ref'):pins[str(case/name)]=sha(case/name)
    else:
        assert len(summaries)==(6 if number==4 else 1)
        assert all(a>0 and b>=[4,1,0,94,128][number] for a,b in summaries)
    for p,h in json.loads((case/'pins.json').read_text()).items():assert pins.setdefault(p,h)==h
    for name in ('status.json','terminal_handles.json','command.json','pins.json','run.log'):pins[str(case/name)]=sha(case/name)
    stages.append(dict(case=str(case),status=status,summaries=summaries,expected_negative=number==2))
authority=json.loads((cases[1]/'native_m6_reference_publication_result.json').read_text())
assert authority['roundtrip_complete'] and authority['reference_facets']==48954
assert authority['ceiling']==8<<30 and authority['wire_bytes']>1<<20
assert (cases[1]/'saved_m6_publication.native_ref').stat().st_size==authority['wire_bytes']
for name in ('native_m6_reference_publication_result.json','saved_m6_publication.native_ref'):pins[str(cases[1]/name)]=sha(cases[1]/name)
for name in ('build293_status.json','build293_terminal_handles.json','build293_source_pins.json',
             'probe_reference293.py','certification293_validate.py','qualify_reference293.py'):
    pins[str((R/name).resolve())]=sha(R/name)
build=json.loads((R/'build293_status.json').read_text());assert build['completed_build'] and build['pins_verified']
cfd=R.parents[3]/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD'
assert sha(cfd)==build['production_cfd_sha256_after'];pins[str(cfd)]=sha(cfd)
assert pins=={p:sha(p) for p in pins}
result=dict(time=time.time(),all_required_checks_passed=True,mpi_ranks=6,
    source_pins_sha256=sha(R/'build293_source_pins.json'),stages=stages,
    large_authority_roundtrip=authority,qualification_pins=pins,full_mesh_accepted=False,
    memory_note="load_peak_bytes is admitted storage, not physical/RSS peak; process receipts are separate.",
    current291_atlas_replayed=False,
    scope='Existing native ceiling propagated into bounded serialization/atlas/self-decoding; retained old/new reference, policy, reader and continuation storage admitted. Canonical validation preserved; temporary decoder retains no query cache. Negative writer refusal preserves prior file. Saved original M6 authority roundtrip is not the lost291 current atlas. Full same-seed TWO-event M6 and actual adapted disk restart remain required.')
out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('qualification_pins','stages')}))
