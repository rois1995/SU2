"""Replay saved481 on the historically qualified 492/v3 target in a fresh root."""
from argparse import ArgumentParser
from pathlib import Path
from unittest.mock import patch

import run_shared as shared

SELF = Path(__file__).resolve()
RUNNER = Path(shared.__file__).resolve()
CUT = shared.cut
N6 = shared.SAVED_REFERENCE.parent
HISTORICAL_RUNNER = N6 / 'run_build578_safeguards.py'
SAVED_VALIDATOR = shared.R / 'check_saved_surface_corrections481.py'
TARGET_KEYS = ('SU2_NATIVE_TARGET_CHECKPOINT3D', 'SU2_NATIVE_FROZEN_SENSOR3D',
               'SU2_NATIVE_FROZEN_CORE3D', 'SU2_NATIVE_FROZEN_TANGENT_SCALE3D')
SOURCE = shared.other_source
PREREQUISITES = shared.prerequisites


def original_case():
    return Path(CUT.read(shared.SAVED_REFERENCE)['safeguards']['saved481']['case'])


def corrected_source(build):
    """Apply the exact N6 saved481 binding to the existing P3 worker."""
    historical = CUT.read(original_case() / 'command.json')['environment']
    target = {key: historical[key] for key in TARGET_KEYS}
    text = SOURCE(build)
    text = shared.replace(text, " if mode=='saved_current':\n  producer_receipt=",
                          " if mode in ('saved481','saved_current'):\n  producer_receipt=")
    rename = "  text=text.replace('curved496_saved_surface_corrections_','curved496_current_target_surface_corrections_')"
    text = shared.replace(text, rename, "  if mode=='saved_current':\n " + rename)
    anchor = "  for file in (producer_receipt,qualification,"
    return shared.replace(text, anchor, f"  assert current == {target!r}\n" + anchor)


def prerequisite_pins(build, closure):
    return [*PREREQUISITES(build, closure), RUNNER, SELF, HISTORICAL_RUNNER, SAVED_VALIDATOR,
            original_case() / 'command.json', original_case() / 'run.cfg']


def native_environment(command):
    ignored = ('SU2_NATIVE_FROZEN_CONFIG', shared.SWITCH, shared.TRACE)
    return {k: v for k, v in command['environment'].items()
            if k.startswith('SU2_NATIVE_') and k not in ignored}


def check_binding(case):
    reference = original_case()
    before = CUT.read(reference / 'command.json')
    after = CUT.read(case / 'command.json')
    assert before['ranks'] == after['ranks'] == 1
    assert before['argv'][-3:] == after['argv'][-3:]
    assert native_environment(before) == native_environment(after), after
    assert (reference / 'run.cfg').read_bytes() == (case / 'run.cfg').read_bytes(), case
    shared.command_check(after, 0, 1, after['timeout_seconds'])
    return {key: after['environment'][key] for key in TARGET_KEYS}


def repair(label):
    current = shared.verify_build(label)
    build = shared.HERE / f'build{label}'
    closure = shared.HERE / 'shared' / build.name / 'closure0'
    previous = closure / 'saved481/case_summary.json'
    original = CUT.read(previous)
    fingerprint = {key: current[key] for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    assert original['status'] == 'FAILED' and all(original[key] == value for key, value in fingerprint.items())
    shared.verified_pins(original)
    old_subrun = original['subruns']['saved481']
    old_case = Path(old_subrun['case'])
    old_log = Path(old_subrun['switch_receipt']['raw'])
    assert 'Mapped target checkpoint predates directional curvature sizing' in old_log.read_text()
    baseline = CUT.read(original_case() / 'command.json')['environment']
    old_binding = CUT.read(old_case / 'command.json')['environment']
    assert all(old_binding[key] != baseline[key] for key in TARGET_KEYS), old_binding
    root = closure / 'saved481_repaired'
    root.mkdir(exist_ok=True)
    summary = root / 'case_summary.json'
    if summary.exists():
        result = CUT.read(summary)
        assert all(result[key] == value for key, value in fingerprint.items())
        shared.verified_pins(result)
        return result
    record = shared.job(root / 'saved481', label, 0, 'saved481', 'saved481')
    selected = Path(record['case'])
    verified_binding = check_binding(selected)
    assert shared.verify_build(label) == current
    status = record['status']
    first = record['first_failure']
    if status == 'FAILED' and first == 'switch markers':
        case_status = CUT.read(selected / 'status.json')
        if case_status['exit_code'] not in (0, 124):
            first = {'field': 'test exit', 'returncode': case_status['exit_code'],
                     'query_entry': 'unestablished', 'raw_log': record['switch_receipt']['raw']}
    result = {**fingerprint, 'status': status, 'environment': shared.environment(0), 'closure_setting': '0',
              'first_difference': first, 'failure_kind': 'result' if status == 'FAILED' else None,
              'subruns': {'saved481': {**record, 'first_failure': first}}, 'pair': None,
              'replaces': {'receipt': str(previous),
                           'original_first_result': 'Mapped target checkpoint predates directional curvature sizing',
                           'classification': 'R3 launcher target binding', 'raw_log': str(old_log),
                           'query_entry': 'not reached on target-import failure', 'original_binding': old_binding[TARGET_KEYS[0]],
                           'qualified_binding': verified_binding},
              'pins': {**record['pins'], **shared.hashes((previous, old_log, *prerequisite_pins(build, 0)))}}
    CUT.save(summary, result)
    return result


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('closure', type=int, choices=(0,), nargs='?')
    parser.add_argument('group', choices=('saved481',), nargs='?')
    parser.add_argument('--worker', choices=('saved481',))
    parser.add_argument('--attempt-dir', type=Path)
    parser.add_argument('--seconds', type=int, choices=(600, 1800), default=600)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        text = corrected_source(shared.HERE / 'build673')
        compile(text, str(shared.REMAINING), 'exec')
        assert " if mode in ('saved481','saved_current'):\n  producer_receipt=" in text
        assert "  if mode=='saved_current':\n   text=text.replace(" in text
        print('saved481 qualified-target worker source verified. No native child launched.')
    else:
        assert args.build is not None
        with patch.multiple(shared, SELF=SELF, other_source=corrected_source, prerequisites=prerequisite_pins):
            if args.worker:
                assert args.closure == 0 and args.group == 'saved481' and args.attempt_dir is not None
                shared.other(args.attempt_dir, shared.HERE / f'build{args.build}', 'saved481', 0, args.seconds)
            else:
                outcome = repair(args.build)
                print('Corrected saved481', outcome['status'], outcome['first_difference'], flush=True)
                raise SystemExit({'passed': 0, 'FAILED': 1, 'OPEN': 75}[outcome['status']])
