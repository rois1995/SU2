"""Replay offline summaries against pinned historical PASS logs, without new numerical qualification."""
from pathlib import Path
import hashlib,json,sys
ROOT=Path(__file__).resolve().parents[3]
sys.path[:0]=[str(ROOT/'integration_evidence/native_cluster_3d_core_v1'),str(ROOT/'integration_evidence/native_3d_core_v1')]
from profile_summary import profiles
from audit_lifecycle import parse

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    destination=Path(__file__).resolve().parent
    if (destination/'validation.json').exists():raise FileExistsError('Preserve the previous replay')
    records=[];outputs=[]
    for label,relative,audit_name,log_name,receipt_relative in (
        ('n4_wedge_n4_m2','reference_lookup_n4_n4_m2_v1','independent_audit.json','case.stdout','reference_lookup_n4_n4_m2_v1/validation.json'),
        ('lifecycle_n4','profile_runtime_v2/lifecycle_mpi4','lifecycle_audit.json','../lifecycle_mpi4.stdout','profile_runtime_v2/validation.json')):
        folder=ROOT/'integration_evidence/native_3d_core_v1'/relative
        receipt_path=ROOT/'integration_evidence/native_3d_core_v1'/receipt_relative
        receipt=json.loads(receipt_path.read_text())
        assert receipt['status']=='PASS' and not receipt['changed_during_run']
        paths=[(folder/audit_name).resolve(),(folder/log_name).resolve()]
        for p in paths:
            key=str(p.relative_to(receipt_path.parent))
            assert sha(p)==receipt['artifact_sha256'][key],key
        audit=json.loads(paths[0].read_text());assert audit['status']=='PASS'
        lines=paths[1].read_text().splitlines()
        native=[parse(s) for s in lines if s.startswith('NATIVE3D ')]
        details=audit.get('nested_rank_details')
        if details is None:details=[parse(s) for s in lines if s.startswith('NATIVE3D_DETAIL ')]
        result=profiles(native,audit['rank_stages'],details)
        out=destination/(label+'.json')
        out.write_text(json.dumps(result,indent=2)+'\n')
        for p in paths:assert sha(p)==receipt['artifact_sha256'][str(p.relative_to(receipt_path.parent))]
        records.append(dict(label=label,receipt=str(receipt_path.relative_to(ROOT)),receipt_sha256=sha(receipt_path),
                            historical_binary_sha256=receipt['binary_sha256'],
                            inputs_sha256={str(p.relative_to(ROOT)):sha(p) for p in paths},
                            native_events=len(result['native_events']),stage_events=len(result['exclusive_stage_events']),
                            nested_events=len(result['nested_detail_events'])))
        outputs.append(dict(file=out.name,sha256=sha(out)))
    result=dict(status='PROFILE_REPLAY_PASS',historical_inputs=records,outputs=outputs,
                source_sha256={str((ROOT/'integration_evidence/native_cluster_3d_core_v1'/n).relative_to(ROOT)):sha(ROOT/'integration_evidence/native_cluster_3d_core_v1'/n) for n in ('profile_summary.py','analyze.py','prepare.py','check_package.py')},
                replay_sha256=sha(Path(__file__)),
                scope='Verified pinned historical PASS logs and summarized their measured rows only. Original audits and evidence untouched; current stricter cache schema is not retroactively applied to old logs. No build, MPI, CFD, scheduler, new metric/mesh qualification, current performance result or speedup claim.')
    (destination/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
if __name__=='__main__':main()
