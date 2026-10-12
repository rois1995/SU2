"""Verify isolated rotating-reservation full regression without promoting production policy."""
from pathlib import Path
import hashlib,json,re
BASE=Path(__file__).resolve().parent;ROOT=BASE.parents[1];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    build_folder=BASE/'rotating_reservation_full_build_v1';build_path=build_folder/'validation.json'
    build=json.loads(build_path.read_text());assert build['status']=='PASS'
    assert not build['source_drift'] and not build['object_drift'] and not build['library_drift']
    for name,h in build['artifact_sha256'].items():assert sha(build_folder/name)==h,name
    assert sha(build_folder/'directory_rotating.cpp')==sha(BASE/'rotating_reservation_probe_v1/directory_rotating.cpp')
    for name,h in build['source_sha256'].items():assert sha(ROOT/name)==h,name
    production=json.loads((BASE/'completion_diagnostic_build_v1/validation.json').read_text())
    assert all(sha(ROOT/name)==h for name,h in production['source_sha256'].items())
    assert sha(ROOT/'integration_evidence/build-integrated-v2/UnitTests/test_driver')==production['binary_sha256']
    cases=[]
    for ranks in (1,2,4):
        folder=BASE/('rotating_reservation_full_mpi%d_v1'%ranks)
        receipt=json.loads((folder/'validation.json').read_text());assert receipt['status']=='PASS'
        for key in ('source_drift','object_drift','archive_drift','tool_drift'):assert not receipt[key]
        assert receipt['binary_sha256']==build['binary_sha256']
        assert receipt['isolated_variant_receipt_sha256']==sha(build_path)
        assert receipt['isolated_variant_source_sha256']==sha(build_folder/'directory_rotating.cpp')
        for name,h in receipt['artifact_sha256'].items():assert sha(folder/name)==h,(ranks,name)
        assertions=re.findall(r'All tests passed \((\d+) assertions in 97 test cases\)',(folder/'case.stdout').read_text())
        assert len(assertions)==ranks
        audit=json.loads((folder/'independent_audit.json').read_text());distributed=json.loads((folder/'distributed_audit.json').read_text())
        assert audit['status']==distributed['status']=='PASS' and len(distributed['stages'])==7
        assert len(audit['frames'])==12*ranks and len(audit['states'])==6*ranks and len(audit['actual_output_roundtrips'])==6*ranks
        cases.append(dict(ranks=ranks,validation_sha256=sha(folder/'validation.json'),assertions=list(map(int,assertions)),
            audit_sha256=sha(folder/'independent_audit.json'),distributed_audit_sha256=sha(folder/'distributed_audit.json'),
            frames=len(audit['frames']),states=len(audit['states']),output_roundtrips=len(audit['actual_output_roundtrips'])))
    result=dict(status='PASS',checker_sha256=sha(Path(__file__).resolve()),build_validation_sha256=sha(build_path),
        variant_source_sha256=sha(build_folder/'directory_rotating.cpp'),binary_sha256=build['binary_sha256'],
        production_binary_sha256=production['binary_sha256'],cases=cases,
        scope='Actual isolated rotating-directory full97-case MPI1/2/4 regression and fresh exact bridge/distributed/repair/coarsening/rollback/state/SU2CGNS/MMG checks. Original production source/binary/objects/archives unchanged. Different trajectories independently audited, not old byte-identity or speed/scaling claims. Stronger whole/restart/lifecycle/production-limit and repeated cluster costs remain needed before promotion; no fullGoal1 completion.')
    (BASE/'ROTATING_RESERVATION_FULL_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS:97cases per MPI1/2/4 rank; fresh bridge and seven distributed audits; production unchanged')

if __name__=='__main__':main()
