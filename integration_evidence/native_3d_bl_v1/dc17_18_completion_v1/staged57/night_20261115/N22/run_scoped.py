"""Build and qualify the two N22 candidates using the established prefix runners."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import sys

from compare_prefix import compare_cases
from gq_baseline import CASE as N21_GQ, ITEM, MANIFEST, bind_identity, check_build, cut, git

sys.path.insert(0, str(ITEM.parent / 'N18'))
from run_de import bind_build, verify_g4
from seal_f import archive

sys.path.insert(0, str(ITEM.parent / 'N21'))
from run_p_scoped import fixture_assertions, scoped_helpers, test_command

BASELINE_G2 = cut.NIGHT / 'N14/section6_12/validation/cases/N14_G2_552'
LABELS = ('serial', 'mpi2', 'mpi4')
BUILDS = {'M': (589, 590), 'C': (590, 591)}
SOURCE = 'Common/src/adaptation/native_engine3d/'
PATHS = {'M': {'Common/include/adaptation/CNativeEngine3D.hpp', SOURCE + 'EngineOwnership.cpp',
               SOURCE + 'EngineOwnership.hpp'},
         'C': {'Common/include/adaptation/CNativeMetricTransport3D.hpp',
               'Common/src/adaptation/CNativeEngine3D.cpp',
               *(SOURCE + name for name in ('EngineExchange.hpp', 'EngineExchange.cpp', 'EngineAdapt.cpp',
                                             'EngineCensus.hpp', 'EngineCensus.cpp'))}}


def paths(part, build):
    return (ITEM / part, cut.PHASE / f'N22{part}/validation',
            cut.PHASE / f'build{build}', cut.PHASE / f'round_8_5u_{part}')


def progress(part, message):
    with (cut.NIGHT / 'progress.md').open('a') as out:
        out.write(f'\nN22 {part}. {message}\n')


def prepare(part, build, previous):
    local, work, target, _ = paths(part, build)
    local.mkdir(exist_ok=True)
    baseline = cut.read(MANIFEST)
    assert baseline['passed'] and baseline['build'] == 589 and Path(baseline['case']) == N21_GQ
    assert cut.terminal(cut.read(ITEM / 'gq_reference_controller_launch.json'))
    assert (ITEM / 'source_check.md').is_file()
    assert all(cut.sha(path) == digest for path, digest in baseline['rank_stdout_sha256'].items())
    if not target.exists():
        old = cut.PHASE / f'build{previous}'
        changed = [str(Path(path).relative_to(cut.REPO)) for path, digest in
                   cut.read(old / 'source_pins.json').items() if cut.sha(path) != digest]
        assert changed and all(path in PATHS[part] or path.startswith('UnitTests/') for path in changed), changed
        assert not git('diff', '--check', str(baseline['commit']), '--', *changed)
        assert bind_build(previous, build, changed, old / 'source_pins.json') == target
        assert bind_identity('N22' + part, build) == work
        cut.save(local / 'candidate_source.json', {'build': build, 'previous': previous, 'paths': sorted(changed)})
    if not (target / 'build_wait.json').exists():
        cut.run_gate(target, 'build', [cut.PYTHON, str(target / 'build.py')])
    status = check_build(target)
    if not (target / 'primary_terminal_receipt.json').exists():
        waited = cut.read(target / 'build_wait.json')
        handles = cut.read(target / 'terminal_handles.json')['identities'] + [waited['controller']]
        assert all(cut.terminal(row) for row in handles)
        cut.save(target / 'primary_terminal_receipt.json', {'actual_host_proc': True, 'passed': True,
                 'all_owned_handles_terminal': True, 'identities': handles, 'controller_wait': waited})
        cut.save(target / 'cut_build_plan.json', {'build_label': f'build{build}',
                 'source_pins_sha256': cut.sha(target / 'source_pins.json')})
    assert all(cut.sha(path) == digest for path, digest in cut.read(target / 'source_pins.json').items())
    assert all(cut.sha(status[name]) == status[f'{name}_sha256'] for name in ('binary', 'test_binary'))
    return status


def launch_prefix(part, build, gate):
    """Reuse the stop and resource checks, then compare the sealed streams separately."""
    _, work, _, _ = paths(part, build)
    script = work / f'run_g2_gq_prefix_{build}.py'
    spec = spec_from_file_location('n22_candidate_prefix', script)
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    module.publish_aliases()
    module.os.environ['OMPI_MCA_osc'] = 'pt2pt'
    env, _ = module.load_runner()['namespace']()
    env['verify_pins']()
    case = work / f'cases/N22{part}_{gate}_{build}'
    sys.argv = [str(env['N4_SCRIPT']), str(case), '--gate', gate]
    env['main']()


def run_fixtures(part, local, status):
    fixtures = {'M': (('M1', 1, '[NativeOwnershipClock3D]~[mpi]'),
                      ('M2', 2, '[NativeOwnershipClock3D][mpi]')),
                'C': (('C2', 2, '[NativeN22C2]'),)}[part]
    values = {}
    for name, ranks, selector in fixtures:
        note = local / f'{name}_r3_note.md'
        if name == 'M2' and note.is_file():
            values[name] = {'state': 'R3_NOTE', 'ranks': ranks, 'note': str(note)}
            continue
        argv = test_command(status['test_binary'], selector)
        if ranks > 1:
            argv[-2:-2] = ['mpirun', '--bind-to', 'none', '-n', str(ranks)]
        receipt_path = local / f'{name}_wait.json'
        if not receipt_path.is_file():
            cut.run_gate(local, name, argv)
        wait = cut.read(receipt_path)
        assert wait['terminal'] and wait['returncode'] == 0
        log = local / f'{name}_outer.log'
        values[name] = {'state': 'PASSED', 'ranks': ranks,
                        'assertions': fixture_assertions(log.read_text(errors='replace'), ranks, 1),
                        'elapsed_seconds': wait['elapsed_seconds'], 'stdout': str(log), 'sha256': cut.sha(log)}
    if not (local / 'fixtures.json').is_file():
        cut.save(local / 'fixtures.json', values)
    return values


def run_prefix(part, build, gate, reference):
    local, work, target, _ = paths(part, build)
    case = work / f'cases/N22{part}_{gate}_{build}'
    if not (work / f'{gate}_wait.json').is_file():
        cut.run_gate(work, gate, [cut.PYTHON, '-B', str(Path(__file__).resolve()),
                                 part, '--build', str(build), '--controller', gate])
    status = cut.read(case / 'status.json')
    assert status['gate_passed'] and status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
    assert status['all_owned_handles_terminal'] and all(cut.terminal(row) for row in status['owned_identities'])
    receipt = cut.read(case / f'{gate}_gate_receipt.json')
    if gate == 'GQ':
        assert receipt['quality_window_rows'] == dict.fromkeys(map(str, range(6)), 3)
    provenance, reference_provenance = (cut.read(location / 'provenance.json') for location in (case, reference))
    assert provenance['binary_sha256'] == cut.read(target / 'build_status.json')['binary_sha256']
    assert provenance['source_pins_sha256'] == cut.sha(target / 'source_pins.json')
    assert provenance['config_sha256'] == reference_provenance['config_sha256']
    for basename in ('m6.su2', 'm6.su2.native_ref', 'baseline_euler_seed.dat'):
        expected, actual = ({Path(path).name: value for path, value in record['input_hashes'].items()}
                            for record in (reference_provenance, provenance))
        assert expected[basename] == actual[basename], basename
    comparison = compare_cases(part, gate, reference, case)
    evidence = local / f'{gate}_comparison.json'
    if not evidence.is_file():
        cut.save(evidence, comparison)
    else:
        assert cut.read(evidence) == comparison, evidence
    assert comparison['passed'], comparison['first_difference']
    result = {'passed': True, 'case': str(case), 'comparison': str(evidence),
              'decisions_changed': comparison['decisions_changed'], 'same_decisions': comparison['same_decisions'],
              'decisions': comparison['decisions'], 'fixedstorage': 4120,
              'seconds_until_stop': status['seconds_until_stop'],
              'observed_prefix_rank_hwm_sum_bytes': receipt['sampled_peak_memory']['sum_rank_process_peak_hwm_bytes']}
    if gate == 'G2':
        counts = status['first_collapse_gate']
        result.update(work=counts['upper_work_sum'], accepted_collapses=[row['accepted'] for row in counts['per_rank']])
    return result


def run_g1(work, build):
    checks = {}
    for cohort, count in zip(LABELS, (375, 114, 228), strict=True):
        if not (work / f'G1_{cohort}_wait.json').is_file():
            cut.run_gate(work, f'G1_{cohort}', [cut.PYTHON, str(work / f'run_g1_{build}.py'), '--cohort', cohort])
        receipt = cut.read(work / f'G1_candidate_{cohort}_summary.json')
        assert receipt['identity_pass'] and receipt['changed_case_count'] == 0
        assert receipt['expected_cases_per_rank'] * receipt['ranks'] == count
        checks[cohort] = {'records': count, 'elapsed_seconds': receipt['elapsed_wall_seconds']}
    return checks


def run_saved(work, build):
    if not (work / 'G3_wait.json').is_file():
        cut.run_gate(work, 'G3', [cut.PYTHON, str(work / 'launch_g3.py')])
    if not (work / 'G4_wait.json').is_file():
        cut.run_gate(work, 'G4', [cut.PYTHON, str(work / f'run_g4_{build}.py'), '--run'])
    verify_g4(work, cut.PHASE / 'N21P/validation')
    g3, = (work / 'cases').glob('*_G3_*/g3_end_b_gate.json')
    g3_result = cut.read(g3)
    assert g3_result['passed']
    return g3_result, cut.read(work / 'g4_case_paths.json')


def case_artifacts(*directories, live_log=None):
    files = set()
    for directory in directories:
        for path in directory.iterdir():
            if path == live_log:
                continue
            if path.suffix in ('.json', '.cfg', '.md', '.py') and path.is_file():
                files.add(path)
            if (path.suffix in ('.log', '.jsonl') or path.name == 'history.csv') and path.is_file():
                files.add(archive(path)[0])
        for path in directory.glob('rank_output/*/rank.*/*'):
            if path.name in ('stdout', 'stderr') and path.is_file():
                files.add(archive(path)[0])
    return files


def seal_scoped(part, build, result):
    local, work, target, branch = paths(part, build)
    artifact_roots = [local, work, target, branch / 'safeguards',
                      *((branch / 'safeguards/cases').iterdir()), *((work / 'cases').iterdir())]
    for row in result['G4']['new_cases']:
        artifact_roots.append(Path(row['case']))
    files = case_artifacts(*artifact_roots, live_log=local / 'scoped_outer.log')
    files.update((cut.PHASE / f'build{build}_aliases.py', Path(__file__).resolve(),
                  ITEM / 'compare_prefix.py', cut.NIGHT / 'progress.md'))
    cut.save(local / 'scoped_acceptance.json', result)
    files.add(local / 'scoped_acceptance.json')
    cut.save(local / 'scoped_paths.json', sorted(str(path) for path in files))


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('part', choices=('M', 'C'))
    parser.add_argument('--build', type=int)
    parser.add_argument('--previous', type=int)
    parser.add_argument('--controller', choices=('G2', 'GQ'))
    parser.add_argument('--gq-reference', type=Path, default=N21_GQ)
    args = parser.parse_args()
    previous, build = BUILDS[args.part]
    build, previous = args.build or build, args.previous or previous
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    if args.controller:
        launch_prefix(args.part, build, args.controller)
        return
    part = args.part
    local, work, _, branch = paths(part, build)
    if part == 'C':
        assert cut.read(ITEM / 'M/scoped_acceptance.json')['passed']
    status = prepare(part, build, previous)
    progress(part, f'Build{build} compiled without warnings. Running the pinned fixtures and scoped gates.')
    fixtures = run_fixtures(part, local, status)
    fixture_states = ', '.join(f"{name} {result['state']}" for name, result in fixtures.items())
    progress(part, f'{fixture_states}. Starting G1.')
    g1 = run_g1(work, build)
    progress(part, 'G1 matched all 375 / 114 / 228 records. Starting G2.')
    g2 = run_prefix(part, build, 'G2', BASELINE_G2)
    progress(part, f"G2 matched {g2['work']:,} first-collapse work units and all six rank counts. Starting G3 and G4.")
    g3, g4 = run_saved(work, build)
    progress(part, f"G3 passed with {g3['accepted_splits']:,} splits and {g3['common_edit_count']:,} exact common edits. G4 passed. Starting GQ.")
    gq = run_prefix(part, build, 'GQ', args.gq_reference.resolve())
    decision_status = 'changed' if gq['decisions_changed'] else 'matched the reference'
    progress(part, f"GQ reached three windows per rank in {gq['seconds_until_stop']:.3f} s, and ownership outcomes {decision_status}. Starting standing safeguards.")
    safeguards = branch / 'safeguards/acceptance_summary.json'
    if not safeguards.is_file():
        scoped_helpers(build, work, branch)['safeguards']()
    assert cut.read(safeguards)['passed']
    result = {'passed': True, 'state': 'PASSED', 'build': build, 'binary_sha256': status['binary_sha256'],
              'fixtures': fixtures, 'G1': g1, 'G2': g2, 'G3': g3, 'G4': g4, 'GQ': gq,
              'safeguards': str(safeguards)}
    seal_scoped(part, build, result)
    progress(part, 'Scoped gates and all standing safeguards passed. Next is the full workflow.')
    print(local / 'scoped_acceptance.json', flush=True)


if __name__ == '__main__':
    main()
