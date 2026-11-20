"""Reuse the H6 OFF prefixes and build608 OFF workflow before two conditional ON runs."""

from argparse import ArgumentParser
from pathlib import Path
import runpy
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FP1 = HERE.parent
sys.path[:0] = [str(FP1), str(FP1.parent / 'N18')]
import run_resume_scoped as scoped
import run_workflow as workflow
from run_g1 import cut, sha, verify_build

OFF = FP1 / 'resume/G5/OFF/run_receipt.json'
OFF_TIMES = (1818.319, 1039.725)


def verified_off(payloads):
    report = cut.read(OFF)
    case = Path(report['case'])
    status = cut.read(case / 'status.json')
    complete = cut.read(case / 'full_workflow_receipt.json')
    assert status['full_workflow_accepted'] and status['all_owned_handles_terminal']
    assert all(cut.terminal(row) for row in status['owned_identities'])
    assert complete['workflow_accepted'] and not complete['errors']
    assert cut.read(Path(report['restart']))['disk_restart_verified']
    assert cut.read(case / 'n8_wall_timer_report.json')['all_fixedstorage_4120']
    assert report['hashes'] == payloads['payload_hashes'](case) == workflow.off_reference(payloads)
    assert report['config_sha256'] == sha(case / 'run.cfg') == workflow.OFF_CONFIG_SHA
    assert [event['event'] for event in report['events']] == [1, 2]
    assert tuple(round(event['native_seconds'], 3) for event in report['events']) == OFF_TIMES
    # The order gives the comparison clocks to millisecond precision.
    report['events'] = [dict(event, native_seconds=seconds) for event, seconds in zip(report['events'], OFF_TIMES)]
    return report


def guard_receipts(build):
    status = cut.read(build / 'build_status.json')
    _, pins = verify_build(status, build / 'source_pins.json')
    replay = cut.read(HERE / 'event1/replay_receipt.json')
    assert replay['build'] == status['build'] and replay['accepted']
    assert replay['pins'][str(build / 'source_pins.json')] == pins
    assert all(sha(Path(path)) == digest for path, digest in replay['pins'].items())
    gate = cut.read(HERE / 'event1/replay_comparison.json')['resume4_stop_rule']
    assert gate['proceed'] and gate['watch']['all_owned_terminal']
    assert all(cut.terminal(row) for row in gate['watch']['owned_processes'])
    scope = cut.read(HERE / 'scope.json')
    assert scope['cconfig_exact_against_option_commit']
    assert scope['nrr_outside_local_core_and_print_exact_against_option_commit']

    prefix = HERE / 'prefix'
    for name in ('G2', 'GQ'):
        assert cut.read(prefix / f'{name}_comparison.json')['passed']
        source = cut.read(prefix / 'cases' / name / 'provenance.json')
        assert source['binary_sha256'] == status['binary_sha256'] and source['source_pins_sha256'] == pins
    records = [375, 114, 228]
    for name, count in zip(('serial', 'mpi2', 'mpi4'), records, strict=True):
        case = HERE / f"G1_{status['build']}" / name
        result = cut.read(case / 'comparison.json')
        assert result['identity_pass'] and result['records'] == count
        assert cut.read(case / 'pins.json')['test_binary'] == status['test_binary_sha256']
    return status, records


def progress(line):
    with (FP1.parent / 'progress.md').open('a') as out:
        out.write(f'\nFP1 H6 post-replay. {line}\n')


def run_scoped(build):
    status, records = guard_receipts(build)
    label = status['build']
    root = HERE / 'scoped'
    root.mkdir(exist_ok=True)
    scoped.legacy_build_receipts(build, status)
    bl = scoped.g3(root / 'g3', build, label)
    progress(f"G3 passed with {bl['accepted_splits']} splits and {bl['common_edit_count']} identical shared edits.")
    two_d = scoped.g4(root / 'g4', build, label)
    progress('G4 2D cohorts passed.')
    saved = scoped.saved_safeguards(root / 'safeguards', build, label)
    scoped.seal(root / 'safeguards.json', {'passed': True, 'build': label, 'G3': bl, 'G4': two_d, 'safeguards': saved})
    scoped.seal(root / 'scoped.json', {'passed': True, 'build': label, 'binary_sha256': status['binary_sha256'],
                                      'G1': records, 'G2': str(HERE / 'prefix/G2_comparison.json'),
                                      'GQ': str(HERE / 'prefix/GQ_comparison.json'), 'G3': bl, 'G4': two_d})
    progress('Scoped gates and saved-state safeguards passed. The OFF workflow proof is build608.')


def on_campaign():
    # The existing workflow owns each launch and restart. Bind the approved prior OFF proof and H pass fields.
    original = Path(workflow.__file__).resolve()
    source = original.read_text()
    changes = [
        ('    outputs = {}\n    for mode in MODES:', "    outputs = {'OFF': verified_off(payloads)}\n    for mode in MODES[1:]:"),
        ("if key not in (*labels, 'rank'):", "if key not in (*labels, 'rank', 'round_changes', 'quality_changes'):"),
        ("float(value) if key == 'seconds' else int(value)", "float(value) if key.startswith('seconds') else int(value)"),
        ("sum_fields(rows, ('direction',))", "sum_fields(rows, ('stage', 'direction'))"),
        ("R / 'f3b_20261115/check_timer_tree.py', NIGHT / 'N16/event_prefix.py']",
         "R / 'f3b_20261115/check_timer_tree.py', NIGHT / 'N16/event_prefix.py', *H6_INPUTS]"),
    ]
    for before, after in changes:
        source = workflow.replace_once(source, before, after)
    env = {'__name__': 'fp1_h6_on_campaign', '__file__': str(original), 'verified_off': verified_off,
           'H6_INPUTS': [Path(__file__).resolve(), OFF, FP1 / 'inputs/FP1_RESUME_4.md',
                         FP1 / 'inputs/FP1_RESUME_3.md', HERE / 'event1/replay_comparison.json',
                         HERE / 'scoped/scoped.json', HERE / 'scoped/safeguards.json']}
    exec(compile(source, str(original), 'exec'), env)
    return env


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', nargs='?', type=Path)
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--only', choices=('scoped', 'on'))
    args = parser.parse_args()
    env = on_campaign()
    if args.self_check:
        prior = verified_off(runpy.run_path(str(workflow.PAYLOADS)))
        assert tuple(e['native_seconds'] for e in prior['events']) == OFF_TIMES
        env['self_check']()
        return
    assert args.build is not None and Path('/proc/1/comm').read_text().strip() == 'systemd'
    build = args.build.resolve()
    if args.only != 'on':
        run_scoped(build)
    if args.only != 'scoped':
        guard_receipts(build)
        root = HERE / 'scoped'
        env['campaign'](HERE / 'G5', build, root / 'scoped.json', root / 'safeguards.json')


if __name__ == '__main__':
    main()
