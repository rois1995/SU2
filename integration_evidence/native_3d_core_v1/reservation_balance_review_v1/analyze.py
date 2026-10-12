"""Diagnose active partition versus reservation work from pinned saved envelope logs."""
from collections import defaultdict
from pathlib import Path
import hashlib
import json
import sys

HERE = Path(__file__).resolve().parent
BASE = HERE.parent
ROOT = BASE.parents[1]
sys.path[:0] = [str(BASE),str(ROOT/'integration_evidence/native_cluster_3d_core_v1')]
from audit_lifecycle import parse
from profile_summary import profiles
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
CASES = ('box_filter_n4_n4_m2_v1','box_filter_a2_direction_n4_n4_m2_v1',
         'box_filter_thin_h20_n4_n4_m2_v1')


def main():
    results = []
    for name in CASES:
        folder = BASE/name
        receipt = json.loads((folder/'validation.json').read_text())
        audit = json.loads((folder/'independent_audit.json').read_text())
        assert receipt['status'] == audit['status'] == 'PASS'
        for artifact,digest in receipt['artifact_sha256'].items():
            assert sha(folder/artifact) == digest, (name,artifact)
        parameters = audit['parameters']
        assert (parameters['ranks'],parameters['workers']) == (4,2)
        native,stages,details = [],[],[]
        sequence = defaultdict(list)
        envelope = defaultdict(list)
        for line in (folder/'case.stdout').read_text().splitlines():
            if line.startswith('NATIVE3D '): native.append(parse(line))
            elif line.startswith('NATIVE3D_STAGE '): stages.append(parse(line))
            elif line.startswith('NATIVE3D_DETAIL '):
                row = parse(line);details.append(row)
                if row['stage'] in ('partition','refine','coarsen'):
                    sequence[int(row['rank'])].append(row['stage'])
            elif line.startswith('ENVELOPE '):
                row = parse(line);envelope[int(row['rank'])].append(int(row['cycle']))
        for rank in range(4):
            assert sequence[rank] == ['partition','refine','coarsen']*3, (name,rank)
            assert envelope[rank] == [0,1,2], (name,rank)
        summaries = profiles(native,stages,details)
        grouped = {(r['stage'],r['event_index']):r['profile'] for r in summaries['nested_detail_events']}
        records = []
        for cycle in range(3):
            partition = grouped['partition',cycle]
            assert sum(v['value'] for v in partition['measures']['cells_before']['rank_values']) == sum(v['value'] for v in partition['measures']['cells_after']['rank_values'])
            assert partition['measures']['work_before']['rank_sum'] == partition['measures']['work_after']['rank_sum']
            active = partition['partition_active_load']
            for operation in ('refine','coarsen'):
                p = grouped[operation,cycle]
                assert p['measured_ranks'] == [0,1]
                counts = {key:p['measures'][key]['rank_values'] for key in (
                    'rounds','attempted','accepted','rejected','conflicts','requests','donor_candidates',
                    'donor_filtered','donor_box_filtered','donor_payloads','edge_evaluations','edge_reused')}
                records.append(dict(cycle=cycle,operation=operation,
                    initial_partition_active_load=active,proposal_outcomes=p['proposal_outcomes'],
                    rank_work_counters=counts,
                    work_imbalance={key:p['measures'][key]['maximum_over_mean'] for key in counts}))
        results.append(dict(case=name,parameters=parameters,validation_sha256=sha(folder/'validation.json'),
            independent_audit_sha256=sha(folder/'independent_audit.json'),source_log_sha256=sha(folder/'case.stdout'),
            binary_sha256=receipt['binary_sha256'],events=records))
    build = json.loads((BASE/'box_filter_build_v2/validation.json').read_text())
    sources = ('Common/src/adaptation/CNativePartition3D.cpp','Common/src/adaptation/CNativeDistributed3D.cpp',
               'Common/src/adaptation/CNativeDonor3D.cpp','Common/src/adaptation/CNativeRemesher3D.cpp')
    assert all(sha(ROOT/name) == build['source_sha256'][name] for name in sources)
    assert all(r['binary_sha256'] == build['binary_sha256'] for r in results)
    result = dict(status='PASS',cases=results,source_sha256={name:sha(ROOT/name) for name in sources},
        analysis_tools_sha256={str(p.relative_to(ROOT)):sha(p) for p in [Path(__file__).resolve(),
            ROOT/'integration_evidence/native_cluster_3d_core_v1/profile_summary.py',BASE/'audit_lifecycle.py']},
        scope='Reparse three pinned successful historical current-prefilter envelope logs using additive reusable active-load/reservation summaries. '
        'Envelope per-rank stage order and explicit cycle labels establish this specialized ordinal association; generic profiles do not assume shared cycle IDs. '
        'Counters are rank work, not elapsed/speed/wait evidence. Owner ADT candidate work and requester payload/private attempts are distinct. '
        'No new SU2/MPI/performance run, partition-weight calibration or reservation-policy change.')
    (HERE/'analysis.json').write_text(json.dumps(result,indent=2)+'\n')
    event = results[1]['events'][3]
    print('PASS:three source-pinned cases;18operation events; active partition and reservation outcomes')
    print('Aspect2 cycle1 coarsen weights',event['initial_partition_active_load']['work_after']['rank_values'])
    print('Attempts/conflicts',event['rank_work_counters']['attempted'],event['rank_work_counters']['conflicts'])
    print('Donor owner candidates',event['rank_work_counters']['donor_candidates'])
    print('Requester payloads',event['rank_work_counters']['donor_payloads'])


if __name__ == '__main__':
    main()
