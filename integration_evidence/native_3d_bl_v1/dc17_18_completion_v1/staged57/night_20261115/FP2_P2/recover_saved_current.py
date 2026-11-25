"""Finish the saved-current comparison from the authoritative historical hashes."""
from argparse import ArgumentParser
from pathlib import Path
import sys
from unittest.mock import patch

import run_p2 as p
import run_shared as shared


def recover(label, closure):
    build = shared.verify_build(label)
    root = p.HERE / 'shared' / f'build{label}' / f'closure{closure}' / 'saved-current'
    summary = root / 'case_summary.json'
    row = p.cut.read(summary)
    p.verify_pins(row['pins'])
    assert p.fingerprint(row) == p.fingerprint(build) and row['environment'] == shared.environment(closure)
    prior = row['subruns']['saved_current']
    if 'reference_recovery' in row:
        return row
    old = p.cut.read(shared.SAVED_REFERENCE)['safeguards']['saved_current']
    reference = Path(old['case'])
    missing = [name for name in old['payloads'] if not (reference / name).is_file()]
    assert missing and prior['first_failure'].startswith('missing evaluation: [Errno 2]'), prior
    assert any(str(reference / name) in prior['first_failure'] for name in missing)
    assert old['passed'] and old['exact'] and old['require_identity']
    historical = p.HERE.parent / 'FP2_P1/shared/build670/closure0/saved-current/case_summary.json'
    p.inherited.c3.c2.committed(historical, 'dec45f8a71')
    old_result = p.cut.read(historical)['subruns']['saved_current']
    assert old_result['status'] == 'passed'
    replay = Path(old_result['raw'])
    assert p.cut.sha(replay) == old_result['raw_sha256']
    assert p.cut.read(replay)['payloads'] == old['payloads']
    attempt = root / 'saved_current'
    case, = (attempt / 'cases').iterdir()
    wait = prior['wait']
    assert wait == p.cut.read(attempt / 'controller_wait.json')
    assert wait['terminal'] and p.cut.terminal(wait['controller'])
    owned = p.cut.read(case / 'safeguard_process_receipt.json')
    assert all(p.cut.terminal(handle) for handle in owned['identities'])

    sys.path.insert(0, str(shared.NIGHT / 'N16'))
    import run_c
    inputs = [historical, replay, case / 'run.log', Path(__file__).resolve()]

    def compare(mode, original, candidate):
        assert mode == 'saved_current' and original == reference and candidate == case
        payloads = {}
        for name, expected in old['payloads'].items():
            assert expected['reference'] == expected['candidate']
            if name not in missing:
                assert p.cut.sha(original / name) == expected['candidate']
                inputs.append(original / name)
            inputs.append(candidate / name)
            payloads[name] = {'reference': expected['candidate'], 'candidate': p.cut.sha(candidate / name)}
        private = bool(run_c.PRIVATE.search((case / 'run.log').read_text()))
        planning = [directory / 'actual_bl_cavity_planning.jsonl' for directory in (reference, case)]
        inputs.extend(planning)
        same_planning = run_c.planning_rows(planning[0]) == run_c.planning_rows(planning[1])
        exact = same_planning and all(item['reference'] == item['candidate'] for item in payloads.values())
        return {'private_work': private, 'require_identity': not private, 'payloads': payloads,
                'exact': exact, 'passed': private or exact}

    with patch.object(run_c, 'compare_safeguard', compare):
        raw, receipt, failed, missed = shared.evaluate(attempt, 'saved_current', closure, wait)
    archive = summary.with_name('case_summary_before_reference_recovery.json')
    if archive.is_file():
        assert archive.read_bytes() == summary.read_bytes()
    else:
        archive.write_bytes(summary.read_bytes())
    status = 'FAILED' if failed else 'OPEN' if missed else 'passed'
    row.update(status=status, reference_recovery={'missing_payloads': missing, 'reference': str(reference),
                                                   'authoritative_seal': str(shared.SAVED_REFERENCE)})
    prior.update(status=status, raw=str(raw), raw_sha256=p.cut.sha(raw), first_failure=failed,
                 missed_comparison=missed, switch_receipt=receipt)
    row['pins'].update(p.hashes((archive, raw, *inputs)))
    pending = summary.with_suffix('.tmp')
    p.cut.save(pending, row)
    pending.replace(summary)
    return row


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('closure', type=int, choices=(0, 1))
    args = parser.parse_args()
    result = recover(args.label, args.closure)
    print('saved-current', result['status'], result.get('reference_recovery'), flush=True)
    raise SystemExit(p.TERMINAL[result['status']])
