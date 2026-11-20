"""Run a fresh G5 OFF workflow and then the two ordered ON workflows."""

from argparse import ArgumentParser
from decimal import Decimal
from pathlib import Path
import runpy
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FP1 = HERE.parent
sys.path.insert(0, str(FP1))
import run_workflow as workflow
from run_g1 import cut, sha

OUTPUT = HERE / 'G5'
ORDER = FP1 / 'inputs/FP1_RESUME_5.md'
CHECKSUM = ORDER.with_suffix('.sha256')
ORDER_SHA = 'd758ed589117dfdac34dd75ed5848312665ea146c26782287ee5e96faf202fc2'
BUILD608 = FP1 / 'resume/G5/OFF/full608_run1'
COVERAGE = 'off_timer_coverage_comparison.json'


def coverage_rows(case):
    """Use the independently verified final remesh ledgers for each event and rank."""
    rows = {}
    for event in (1, 2):
        gate = cut.read(case / f'f3b_event{event}_timer_gate.json')
        assert gate['event'] == event and gate['passed']
        assert sorted(row['rank'] for row in gate['ranks']) == list(range(6))
        for row in gate['ranks']:
            ledger = row['ledger']
            summary = ledger['summary']
            rank = row['rank']
            assert ledger['passed'] and str(rank) == summary['rank']
            assert summary['root'] == 'remesh' and summary['boundary'] == 'final'
            assert summary['fixedstorage'] == '4120'
            coverage = Decimal(summary['coverage'])
            assert Decimal('.95') <= coverage <= 1
            rows[event, rank] = coverage
    return rows


def off_proof(output, build, binary_sha, source_sha, payloads, reference):
    work = output / 'OFF'
    case = work / f'full{build}_run1'
    receipt_file = work / 'run_receipt.json'
    receipt = cut.read(receipt_file)
    assert Path(receipt['case']) == case
    manifest = workflow.verify_helpers(work)
    assert manifest['build'] == build and manifest['config_sha256'] == workflow.OFF_CONFIG_SHA
    provenance = cut.read(case / 'provenance.json')
    assert provenance['source_pins_sha256'] == source_sha
    assert receipt['config_sha256'] == provenance['config_sha256'] == sha(case / 'run.cfg') == workflow.OFF_CONFIG_SHA
    status = cut.read(case / 'status.json')
    assert status['exit_code'] == 0 and status['full_workflow_accepted']
    assert status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
    for name in ('full', 'restart'):
        wait = cut.read(work / f'{name}_wait.json')
        assert wait['controller'] == cut.read(work / f'{name}_launch.json')['controller']
        assert wait['returncode'] == 0 and wait['terminal'] and cut.terminal(wait['controller'])
    assert receipt['hashes'] == workflow.workflow_payloads(case, binary_sha, payloads) == reference
    full = case / 'full_workflow_receipt.json'
    identity = cut.read(case / 'n8_payload_identity.json')
    assert identity['passed'] and identity['workflow_gates_passed'] and identity['hashes'] == reference
    assert identity['workflow_receipt_sha256'] == sha(full)
    logs = identity['rank_stdout_sha256']
    assert len(logs) == 6 and all(sha(case / path) == digest for path, digest in logs.items())
    restart = Path(receipt['restart'])
    isolated = cut.read(restart)
    assert isolated['disk_restart_verified'] and isolated['all_inputs_and_pins_unchanged']
    assert isolated['source_event_receipt_sha256'] == sha(full)
    assert all(sha(restart.parent / path) == digest for path, digest in isolated['artifact_sha256'].items())
    native = [{key: event[key] for key in ('event', 'native_seconds')} for event in receipt['events']]
    accepted = cut.read(full)
    assert accepted['workflow_accepted'] and not accepted['errors']
    assert native == [{'event': event['event'], 'native_seconds': event['whole_event_seconds_max_rank']}
                      for event in accepted['events']]
    assert [event['event'] for event in native] == [1, 2]
    previous, current = coverage_rows(BUILD608), coverage_rows(case)
    return {
        'passed': True, 'build': build, 'binary_sha256': binary_sha, 'source_pins_sha256': source_sha,
        'run_receipt_sha256': sha(receipt_file), 'restart_receipt_sha256': sha(restart),
        'workflow_receipt_sha256': sha(full), 'reference_off_case': str(BUILD608), 'case': str(case),
        'native_maxima': native,
        'coverage': [
            {'event': event, 'rank': rank, 'build608_percent': float(previous[event, rank] * 100),
             'resume5_percent': float(coverage * 100),
             'delta_percentage_points': float((coverage - previous[event, rank]) * 100)}
            for (event, rank), coverage in sorted(current.items())
        ],
    }


def verified_off(output, build, binary_sha, source_sha, payloads, reference):
    proof = cut.read(output / 'OFF' / COVERAGE)
    assert proof == off_proof(output, build, binary_sha, source_sha, payloads, reference)
    return cut.read(output / 'OFF' / 'run_receipt.json')


def bind_campaign(only, extra_sources):
    # Bind the handoff and H6 report fields without editing the archived workflow or its gates.
    original = Path(workflow.__file__).resolve()
    source = original.read_text()
    off_args = 'output, build, binary_sha, source_sha, payloads, reference'
    anchor = "    differences = payloads['compare_hashes'](outputs['ON2']['hashes'], outputs['ON1']['hashes'])"
    changes = [
        ('    outputs = {}\n    for mode in MODES:',
         f"    outputs = {{'OFF': verified_off({off_args})}} if ONLY == 'on' else {{}}\n"
         "    for mode in (MODES[1:] if ONLY == 'on' else MODES[:1]):"),
        (anchor, f"    if ONLY == 'off':\n        sealed_write(output / 'OFF' / COVERAGE, off_proof({off_args}))\n"
         "        print('FP1 N G5 OFF PASSED.', flush=True)\n        return\n" + anchor),
        ("if key not in (*labels, 'rank'):", "if key not in (*labels, 'rank', 'round_changes', 'quality_changes'):"),
        ("float(value) if key == 'seconds' else int(value)", "float(value) if key.startswith('seconds') else int(value)"),
        ("sum_fields(rows, ('direction',))", "sum_fields(rows, ('stage', 'direction'))"),
        ("R / 'f3b_20261115/check_timer_tree.py', NIGHT / 'N16/event_prefix.py']",
         "R / 'f3b_20261115/check_timer_tree.py', NIGHT / 'N16/event_prefix.py', *EXTRA_SOURCES]"),
    ]
    for before, after in changes:
        source = workflow.replace_once(source, before, after)
    env = {'__name__': 'fp1_n_g5', '__file__': str(original), 'ONLY': only, 'EXTRA_SOURCES': extra_sources,
           'COVERAGE': COVERAGE, 'off_proof': off_proof, 'verified_off': verified_off}
    exec(compile(source, str(original), 'exec'), env)
    return env


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', nargs='?', type=Path)
    parser.add_argument('--only', choices=('off', 'on'))
    parser.add_argument('--scoped', type=Path)
    parser.add_argument('--safeguards', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    assert sha(ORDER) == ORDER_SHA and CHECKSUM.read_text().split() == [ORDER_SHA, ORDER.name]
    if args.self_check:
        prior = cut.read(BUILD608 / 'provenance.json')
        reference = cut.read(BUILD608.parent / 'run_receipt.json')['hashes']
        proof = off_proof(BUILD608.parents[1], 608, prior['binary_sha256'], prior['source_pins_sha256'],
                          runpy.run_path(str(workflow.PAYLOADS)), reference)
        assert len(proof['coverage']) == 12 and all(row['delta_percentage_points'] == 0 for row in proof['coverage'])
        assert bind_campaign('on', [])['ONLY'] == 'on'
        bind_campaign('off', [])['self_check']()
        return
    if None in (args.build, args.only, args.scoped, args.safeguards):
        parser.error('provide the sealed build, --only, --scoped and --safeguards')
    scoped, safeguards = args.scoped.resolve(), args.safeguards.resolve()
    sources = [Path(__file__).resolve(), ORDER, CHECKSUM, scoped, safeguards]
    if args.only == 'on':
        sources.extend(OUTPUT / 'OFF' / name for name in ('run_receipt.json', COVERAGE))
    bind_campaign(args.only, sources)['campaign'](OUTPUT, args.build.resolve(), scoped, safeguards)


if __name__ == '__main__':
    main()
