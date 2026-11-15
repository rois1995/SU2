"""Seal the one completed F3 milestone: exclusive roots, physical gates, memory and payload identity."""
import hashlib
import importlib.util
import json
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
CASE = HERE / 'full_workflow'
PRIOR = HERE.parent / 'purpose_query_fix_20261114/p6c_20261115/full_workflow'


def sha(path):
    """Hash a retained artifact without reading it all into memory."""
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def root_receipts():
    """Require complete per-rank production roots and reconcile every exclusive ledger."""
    spec = importlib.util.spec_from_file_location('timer_gate', HERE / 'check_timer_tree.py')
    timer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(timer)
    timer.self_check()
    records = []
    for path in sorted(CASE.glob('rank_output/*/rank.*/stdout')):
        for item in timer.snapshots(path.read_text(errors='replace')):
            if item['summary']['boundary'] == 'final':
                records.append(dict(timer.assess(item), source=str(path)))
    assert records, 'Missing terminal timer rows'
    counts = defaultdict(lambda: defaultdict(int))
    for record in records:
        row = record['summary']
        counts[row['root']][int(row['rank'])] += 1
    for root, ranks in counts.items():
        assert set(ranks) == set(range(6)), (root, 'missing rank', dict(ranks))
        assert len(set(ranks.values())) == 1, (root, 'different event counts', dict(ranks))
    assert {'metric', 'remesh', 'replace', 'mesh_output'}.issubset(counts), counts
    assert all(value == 2 for value in counts['remesh'].values()), counts['remesh']
    old_measurements = [dict(root=record['summary']['root'], rank=record['summary']['rank'], **row)
                        for record in records for row in record['rows']
                        if row['operation'] == 'old_cavity_measurement']
    assert old_measurements, 'Required old-cavity measurement node was never recorded'
    return {'passed': all(record['passed'] for record in records), 'records': records,
            'root_counts_by_rank': dict(counts), 'old_cavity_measurements': old_measurements,
            'minimum_named_coverage': min(float(record['summary']['coverage']) for record in records),
            'scope': 'Separate nonoverlapping production lifecycle roots; inclusive calls and repeated snapshots are nonadditive.'}


def payload_identity():
    """Compare both mesh/reference/matching restart triplets against the accepted P6c workflow."""
    records = []
    for event in (1, 2):
        mesh = f'adapted_mesh_adap_{event:05d}.su2'
        for name in (mesh, mesh + '.native_ref', f'restart_flow_adap_{event:05d}.dat'):
            current = CASE / name
            prior = PRIOR / name
            assert current.is_file() and prior.is_file(), name
            current_hash, prior_hash = sha(current), sha(prior)
            records.append({'path': name, 'sha256': current_hash, 'P6c_sha256': prior_hash,
                            'byte_identical': current_hash == prior_hash})
    return {'passed': all(row['byte_identical'] for row in records), 'records': records}


def main():
    """Save a closed receipt only after terminal handles, unchanged pins and all independent gates."""
    status = json.loads((CASE / 'status.json').read_text())
    assert status['state'] == 'terminal' and status['all_owned_handles_terminal']
    assert status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
    physical = json.loads((CASE / 'independent_workflow_receipt.json').read_text())
    collapse = json.loads((CASE / 'final_source_first_collapse_gate.json').read_text())
    tree = root_receipts()
    payloads = payload_identity()
    result = {'passed': status['exit_code'] == 0 and physical['workflow_accepted'] and
              collapse['passed'] and tree['passed'] and payloads['passed'],
              'status_sha256': sha(CASE / 'status.json'), 'physical': physical,
              'first_collapse': collapse, 'timers': tree, 'payload_identity': payloads,
              'isolated_restart': 'Prior passing P6c restart covers byte-identical final inputs; no fresh restart claimed.',
              'curved_BL_RANS_goal_complete': False}
    (HERE / 'complete_milestone_receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'passed': result['passed'], 'minimum_named_coverage': tree['minimum_named_coverage'],
                      'root_counts_by_rank': tree['root_counts_by_rank'], 'payloads_equal_P6c': payloads['passed']}))
    return not result['passed']


if __name__ == '__main__':
    raise SystemExit(main())
