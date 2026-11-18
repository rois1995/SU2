"""Run Part C with the same workflow gates and its own six-file repeat reference."""
import json
from pathlib import Path
import runpy
import shutil
import subprocess
import sys

from event_prefix import compare
from report_85r import purpose, rank_ledgers, write_csv
from run_scoped import N16, cut

BASE = cut.PHASE / 'phase_D/G5'
WORKSPACE = cut.PHASE / 'round_8_5r_C/G5'
RUN1 = WORKSPACE / 'full575_run1'
RUN2 = WORKSPACE / 'full575_run2'
T0 = cut.PHASE / 'round_8_5r_B/G5/full574'


def substitute(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def bind_full(source):
    # The inherited runner still performs the full physical, transfer and resource validation.
    old = ('    reference = json.loads(module.PAYLOAD_REFERENCE.read_text())\n'
           '    actual = comparator.payload_hashes(CASE)\n'
           '    mismatches = comparator.compare_hashes(actual, reference["hashes"])')
    new = '''    actual = comparator.payload_hashes(CASE)
    first_case = CASE.parent / "full575_run1"
    reference_path = first_case / "accepted_reference_payloads.json"
    if CASE == first_case:
        with reference_path.open("x") as file:
            json.dump({"accepted": False, "hashes": actual, "state": "run-one standing gates passed",
                       "workflow_receipt_sha256": sha(CASE / "full_workflow_receipt.json")}, file, indent=2)
            file.write("\\n")
    reference = json.loads(reference_path.read_text())
    assert comparator.payload_hashes(first_case) == reference["hashes"]
    mismatches = comparator.compare_hashes(actual, reference["hashes"])'''
    anchor = '    return source\n\n\ndef postvalidate'
    replacements = (
        (old, new),
        ('"reference": str(module.PAYLOAD_REFERENCE)', '"reference": str(reference_path)'),
        ('"reference_sha256": sha(module.PAYLOAD_REFERENCE)', '"reference_sha256": sha(reference_path)'),
        ('six payloads differ from accepted references', 'six payloads differ from Part C run one'),
    )
    lines = ['    assert source.count(' + repr(a) + ') == 1\n'
             '    source = source.replace(' + repr(a) + ', ' + repr(b) + ', 1)\n'
             for a, b in replacements]
    return substitute(source, anchor, ''.join(lines) + anchor)


def bind_restart(source, anchor):
    helper = '''    reference = source_case / "accepted_reference_payloads.json"
    payloads = json.loads(reference.read_text())
    assert len(payloads["hashes"]) == 6
    assert source_case.name == "full575_run1"
    # Bind only the accepted validator's old reference path and expected hashes.
    helper.verify_n8_identity.__globals__.update(SECTION1=source_case, REFERENCE_HASHES=payloads["hashes"])
'''
    return substitute(source, anchor, helper + anchor)


def prepare():
    WORKSPACE.mkdir(parents=True)
    for name in ('check_connected_events.py', 'check_complete_events.py', 'check_timer_tree.py',
                 'unchanged_build499_source_pins.json'):
        shutil.copyfile(BASE / name, WORKSPACE / name)
    for stem in ('run_full', 'run_restart', 'validate_restart'):
        source = (BASE / f'{stem}572.py').read_text().replace('572', '575')
        if stem == 'run_full':
            source = bind_full(source)
        elif stem == 'run_restart':
            source = bind_restart(source, '    helper.read_source_receipts(source_case)\n')
        else:
            source = bind_restart(source, '    helper.source_receipts(source_case)\n')
        target = WORKSPACE / f'{stem}575.py'
        compile(source, str(target), 'exec')
        target.write_text(source)
    cut.save(WORKSPACE / 'helper_pins.json', {str(path): cut.sha(path) for path in WORKSPACE.iterdir()})


def check_and_call(name, runner, *args):
    script = WORKSPACE / f'{runner}575.py'
    return cut.run_gate(WORKSPACE, name, [cut.PYTHON, str(script), *map(str, args)])


def restart_validation():
    case = WORKSPACE / f'disk_restart_{RUN1.name}_event2'
    receipt = cut.read(case / 'post_terminal_handles.json')
    assert receipt['actual_host_proc'] and receipt['all_owned_handles_terminal']
    handles = [receipt['controller_identity'], receipt['controller_wrapper_identity']]
    handles.extend(row['identity'] for row in receipt['owned_identity_checks'])
    assert all(cut.terminal(handle) for handle in handles)
    validator = WORKSPACE / 'validate_restart575.py'
    with (WORKSPACE / 'restart_post_only.log').open('x') as output:
        subprocess.run([cut.PYTHON, str(validator), '--source-case', str(RUN1), '--case-dir', str(case)],
                       stdout=output, stderr=subprocess.STDOUT, check=True)
    assert cut.read(case / 'disk_restart_validation.json')['disk_restart_verified']


def run():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert cut.read(T0.parent / 'acceptance_summary.json')['passed']
    assert cut.terminal(cut.read(N16 / 'T0_campaign_launch.json'))
    assert all(cut.sha(path) == digest for path, digest in cut.read(WORKSPACE / 'helper_pins.json').items())
    for stem in ('run_full', 'run_restart', 'validate_restart'):
        path = WORKSPACE / f'{stem}575.py'
        with (WORKSPACE / f'{stem}_selfcheck.json').open('x') as output:
            subprocess.run([cut.PYTHON, str(path), '--self-check'], stdout=output, check=True)
    check_and_call('C_run1', 'run_full', '--case', RUN1)
    reference = cut.read(T0 / 'n8_payload_identity.json')
    assert reference['passed'] and all(cut.sha(T0 / path) == digest for path, digest in reference['rank_stdout_sha256'].items())
    identity = compare(T0, RUN1)
    cut.save(RUN1 / 'event_one_construction_identity.json', identity)
    assert identity['passed'], 'See event_one_construction_identity.json for the first difference.'
    check_and_call('C_restart', 'run_restart', '--source-case', RUN1)
    restart_validation()
    check_and_call('C_run2', 'run_full', '--case', RUN2)
    first, second = [cut.read(case / 'n8_payload_identity.json') for case in (RUN1, RUN2)]
    assert first['passed'] and second['passed'] and first['hashes'] == second['hashes']
    resumed_workflow = (cut.PHASE / 'phase_C/collect_g5_summary.py')
    collect_events = runpy.run_path(str(resumed_workflow))['collect_events']
    events = [collect_events(case) for case in (T0, RUN1, RUN2)]
    t0, one, two = [rank_ledgers(case, name) for case, name in ((T0, 'T0'), (RUN1, 'run1'), (RUN2, 'run2'))]
    gate = purpose(t0, one)
    cut.save(WORKSPACE / 'purpose_rule.json', gate)
    reports = N16 / 'reports'
    reports.mkdir(exist_ok=True)
    write_csv(reports / 'STEP_8_5R_575.csv', t0 + one + two)
    cut.save(WORKSPACE / 'acceptance_summary.json', {'passed': gate['passed'], 'standing_gates_passed': True,
             'build': 575, 'repeat_payloads_exact': True, 'events': events, 'purpose': gate,
             'repeat_payloads': second['hashes']})


if __name__ == '__main__':
    {'--prepare': prepare, '--run': run}[sys.argv[1]]()
