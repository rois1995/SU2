"""Scope the inherited switch receipt's rank-option check to mpirun."""

from argparse import ArgumentParser
import inspect
from pathlib import Path
import runpy
from unittest.mock import patch

import run_full as full
from checkpoint import require, seal

SELF = Path(__file__).resolve()


def corrected_switch_receipt():
    source = inspect.getsource(full.switch_receipt)
    old = "argv[argv.index('-n') + 1]"
    assert source.count(old) == 1
    source = source.replace(old, "argv[argv.index('-n', argv.index('mpirun') + 1) + 1]")
    namespace = vars(full).copy()
    exec(compile(source, str(SELF), 'exec'), namespace)
    return namespace['switch_receipt']


def recover_off(label):
    part = 'B2b'
    require(label, 'B0', 'B1', 'B2a')
    build = full.verify_build(label)
    root = full.root(label, part)
    work = root / 'trial1'
    case = work / f'full{label}_run1'
    restart = work / f'disk_restart_{case.name}_event2' / 'disk_restart_validation.json'
    config = full.config_for('OFF')
    payloads = runpy.run_path(str(full.workflow.PAYLOADS))
    for stage in ('full', 'restart'):
        full.C3.terminal_wait(work, stage, 0)
    full.BASE['status_ok'](case, build, config)
    full.C3.require_clean_contact_logs(work, case)
    full.C3.require_clean_contact_logs(work, restart.parent)
    full.workflow.verify_helpers(work)
    restart_row = full.cut.read(restart)
    assert restart_row['disk_restart_verified'] and restart_row['all_inputs_and_pins_unchanged']
    assert restart_row['source_event_receipt_sha256'] == full.sha(case/'full_workflow_receipt.json')
    assert all(full.sha(restart.parent/name)==digest for name,digest in restart_row['artifact_sha256'].items())
    identity = full.cut.read(case/'n8_payload_identity.json')
    hashes = full.workflow.workflow_payloads(case, build['binary_sha256'], payloads)
    assert identity['passed'] and identity['workflow_gates_passed'] and identity['hashes'] == hashes
    events = full.workflow.event_collector()(case)
    for path in (work/'failed_attempt.json', root/'case_summary.json',
                 full.HERE/f'build{label}/B2b.json'):
        archived = path.with_name(f'{path.stem}_before_rank_option_repair.json')
        if not archived.exists():
            row = full.cut.read(path)
            assert (row.get('stage') == 'sealing provenance' or row.get('failure_kind') == 'measurement'
                    or row.get('status') == 'FAILED'), path
            path.rename(archived)
    full.envelope(work, label, part, build, config, events, payloads, hashes, restart)
    full.authenticate_run(work, label, part, build, payloads)


def run(label, part, recover):
    prepare = full.campaign
    with patch.object(full, 'switch_receipt', corrected_switch_receipt()), patch.object(
            full, 'campaign', lambda mode, pins, retry=False: prepare(mode, [*pins, SELF], retry)):
        if recover:
            recover_off(label)
        return full.run(label, part)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('part', choices=full.MODES)
    parser.add_argument('--recover-off', action='store_true')
    args = parser.parse_args()
    assert not args.recover_off or args.part == 'B2b'
    raise SystemExit(run(args.build, args.part, args.recover_off))
