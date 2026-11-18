"""Apply a reviewed phase-C draft and run its prescribed gates on the actual host."""
from argparse import ArgumentParser
from datetime import datetime
from pathlib import Path
import gzip
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time


PHASE = Path(__file__).resolve().parents[1]
NIGHT = PHASE.parents[1]
REPO = NIGHT.parents[4]
BASE = PHASE / 'end_B_preparation'
PREFLIGHT = BASE / 'primary_control_archive/end_b_actual_preflight.py'
SOURCE = REPO / 'Common/src/adaptation'
PYTHON = '/usr/bin/python3'
REPAIR_PREFIX_STEPS = {3, 4, 5, 6, 8, 10, 11}


def read(path):
    return json.loads(Path(path).read_text())


def save(path, value):
    with Path(path).open('x') as output:
        json.dump(value, output, indent=2)
        output.write('\n')


def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def actual_identity(pid):
    fields = (Path('/proc') / str(pid) / 'stat').read_text().rsplit(')', 1)[1].split()
    return {'pid': pid, 'start_ticks': int(fields[19])}


def terminal(identity):
    try:
        fields = (Path('/proc') / str(identity['pid']) / 'stat').read_text().rsplit(')', 1)[1].split()
        return fields[0] in ('Z', 'X') or int(fields[19]) != identity['start_ticks']
    except FileNotFoundError:
        return True


def run_gate(workspace, name, argv):
    """Run a single owned command after sampling the real host."""
    preflight = workspace / f'{name}_host_preflight.json'
    subprocess.run([PYTHON, str(PREFLIGHT), str(preflight)], cwd=REPO, check=True)
    print(f'{workspace.parent.name} {name}: starting', flush=True)
    with (workspace / f'{name}_outer.log').open('x') as logfile:
        child = subprocess.Popen(argv, cwd=REPO, stdout=logfile, stderr=subprocess.STDOUT)
        started = time.time()
        handle = actual_identity(child.pid)
        save(workspace / f'{name}_launch.json', {'actual_host_proc': True, 'controller': handle,
                                               'supervisor': actual_identity(os.getpid()), 'unix': started})
        code = child.wait()
    receipt = {'actual_host_proc': True, 'controller': handle, 'returncode': code,
               'elapsed_seconds': time.time() - started, 'terminal': terminal(handle)}
    save(workspace / f'{name}_wait.json', receipt)
    if code or not receipt['terminal']:
        raise RuntimeError(f'{name} exit {code}. See {workspace / (name + "_outer.log")}.')
    print(f'{workspace.parent.name} {name}: terminal in {receipt["elapsed_seconds"]:.3f} s', flush=True)
    return receipt


def rebind_validation(step, build, workspace):
    """Retarget the accepted N14 adapters without changing their comparisons or reference tables."""
    v2 = PHASE / 'C2/validation'
    for name in ('run_g1_560.py', 'run_g2_gq_prefix_560.py', 'validate_g2_gq.py'):
        text = (v2 / name).read_text().replace('C2/validation', f'C{step}/validation')
        (workspace / name.replace('560', str(build))).write_text(text.replace('560', str(build)))
    (workspace / 'run_g2_gq_prefix.py').symlink_to(f'run_g2_gq_prefix_{build}.py')
    for script in (f'run_g1_{build}.py', f'run_g2_gq_prefix_{build}.py'):
        subprocess.run([PYTHON, str(workspace / script), '--self-check'], cwd=REPO, check=True,
                       stdout=subprocess.DEVNULL)
    if step >= 8:
        for filename in ('run_g3_558.py', 'validate_end_b.py'):
            source = (BASE / filename).read_text().replace('PHASE = HERE.parent', 'PHASE = HERE.parents[1]')
            source = source.replace('558', str(build)).replace('CASE = "endb_g3"', f'CASE = "C{step}_G3"')
            if filename.startswith('run_g3_'):
                old = '        if target.exists():\n            raise FileExistsError(target)\n        target.write_text(json.dumps(value, indent=2) + "\\n")'
                new = '        if target.exists():\n            assert json.loads(target.read_text()) == value, target\n        else:\n            target.write_text(json.dumps(value, indent=2) + "\\n")'
                assert source.count(old) == 1
                source = source.replace(old, new, 1)
            (workspace / filename.replace('558', str(build))).write_text(source)
        launcher = (BASE / 'primary_control_archive/end_b_launch_g3.py').read_text()
        launcher = launcher.replace("v=p/'end_B_preparation'", f"v=p/'C{step}/validation'").replace('558', str(build))
        (workspace / 'launch_g3.py').write_text(launcher)
        subprocess.run([PYTHON, str(workspace / f'run_g3_{build}.py'), '--self-check'], cwd=REPO, check=True,
                       stdout=subprocess.DEVNULL)
    if step == 11:
        source = (BASE / 'run_g4_558.py').read_text().replace('N4 = HERE.parent.parent', 'N4 = HERE.parents[2]')
        source = source.replace('PHASE = HERE.parent', 'PHASE = HERE.parents[1]').replace('558', str(build))
        (workspace / f'run_g4_{build}.py').write_text(source)


def prepare_candidate(step, build, workspace, build_dir):
    """Apply only the reviewed draft at its verified predecessor and seal the new build graph inputs."""
    previous = PHASE / f'C{step - 1}/validation/acceptance_summary.json'
    assert read(previous)['state'] == 'PASSED', previous
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO, text=True).strip()
    prior_pins = read(PHASE / f'build{build - 1}/source_pins.json')
    assert all(sha(path) == expected for path, expected in prior_pins.items())
    subprocess.run(['git', 'diff', '--quiet', '--', str(SOURCE)], cwd=REPO, check=True)
    subprocess.run(['git', 'diff', '--cached', '--quiet'], cwd=REPO, check=True)
    prep = PHASE / f'C{step}_preparation'
    draft_pins = read(PHASE / 'phase_C/reviewed_draft_pins.json')[str(step)]
    assert draft_pins and all(sha(PHASE / path) == digest for path, digest in draft_pins.items())
    workspace.mkdir(parents=True, exist_ok=False)
    build_dir.mkdir(exist_ok=False)
    subprocess.run([PYTHON, str(PREFLIGHT), str(workspace / 'pre_edit_host_preflight.json')], cwd=REPO, check=True)
    if step in (3, 4):
        subprocess.run(['sha256sum', '-c', str(prep / 'draft_base.sha256')], cwd=REPO, check=True)
        touched = [SOURCE / 'CNativeEngine3D.cpp', SOURCE / 'native_engine3d/EngineCommit.cpp']
        for path in touched:
            shutil.copyfile(prep / 'draft' / path.name, path)
    else:
        patch = prep / 'proposed.diff'
        subprocess.run(['git', 'apply', '--check', str(patch)], cwd=REPO, check=True)
        subprocess.run(['git', 'apply', str(patch)], cwd=REPO, check=True)
        touched = [REPO / line[6:] for line in patch.read_text().splitlines() if line.startswith('+++ b/')]
    assert all(path.is_relative_to(SOURCE) for path in touched)
    subprocess.run(['git', 'diff', '--check', '--', *map(str, touched)], cwd=REPO, check=True)
    pins = dict(prior_pins)
    pins.update((str(path), sha(path)) for path in touched)
    save(build_dir / 'source_pins.json', dict(sorted(pins.items())))
    build_script = (PHASE / 'build560/build.py').read_text().replace('560', str(build))
    (build_dir / 'build.py').write_text(build_script)
    alias_script = (PHASE / 'build560_aliases.py').read_text().replace('560', str(build))
    alias_script = alias_script.replace('B8/validation', f'C{step}/validation')
    (PHASE / f'build{build}_aliases.py').write_text(alias_script)
    rebind_validation(step, build, workspace)
    save(workspace / 'candidate_source.json', {'base_commit': head, 'build': build,
                                               'source_paths': list(map(str, touched))})
    question = {
        3: ('Does “join stays first” move the join ahead of ReportPrivateWork? The alternatives were '
            'the first post-slot action and the first post-slot collective. Kept the private work observation '
            'immediately before the unchanged first collective because C requires the original order and timer scope.'),
        9: ('Where should the immediate inputs to the legacy strip, apex and growth searches live? '
            'The alternatives were caller-local assembly and moving the E18–E24 block together. '
            'Moved E18–E24 together to preserve the original scopes and retain the three reservation policies.')
    }.get(step)
    if question:
        stamp = datetime.now().astimezone().isoformat(timespec='seconds')
        with (NIGHT / 'QUESTIONS_FOR_REVIEWER.md').open('a') as log:
            log.write(f"\n## {stamp} · N4 phase C · C{step}\n\n{question} C{step} base commit `{head[:10]}`.\n")
    return touched


def qualify(step, build, workspace, build_dir):
    """Build once and run the authorized identity and fixture gates in isolation."""
    build_wait = run_gate(build_dir, 'build', [PYTHON, str(build_dir / 'build.py')])
    build_status = read(build_dir / 'build_status.json')
    handles = read(build_dir / 'terminal_handles.json')
    assert build_status['pins_verified'] and handles['all_owned_handles_terminal']
    identities = handles['identities'] + [dict(build_wait['controller'], alive=False)]
    assert all(terminal(row) for row in identities)
    save(build_dir / 'primary_terminal_receipt.json', {'actual_host_proc': True, 'passed': True, 'identities': identities,
                                                    'all_owned_handles_terminal': True, 'controller_wait': build_wait})
    save(build_dir / 'cut_build_plan.json', {'build_label': f'build{build}',
                                            'source_pins_sha256': sha(build_dir / 'source_pins.json')})
    for cohort in ('serial', 'mpi2', 'mpi4'):
        run_gate(workspace, f'G1_{cohort}', [PYTHON, str(workspace / f'run_g1_{build}.py'), '--cohort', cohort])
        result = read(workspace / f'G1_candidate_{cohort}_summary.json')
        assert result['identity_pass'] and result['changed_case_count'] == 0
    run_gate(workspace, 'G2', [PYTHON, str(workspace / f'run_g2_gq_prefix_{build}.py'), '--gate', 'G2',
                              '--case-name', f'C{step}_G2_{build}'])
    if step >= 8:
        run_gate(workspace, 'G3', [PYTHON, str(workspace / 'launch_g3.py')])
    if step == 11:
        run_gate(workspace, 'G4', [PYTHON, str(workspace / f'run_g4_{build}.py'), '--run'])
    if step in REPAIR_PREFIX_STEPS:
        run_gate(workspace, 'GQ', [PYTHON, str(workspace / f'run_g2_gq_prefix_{build}.py'), '--gate', 'GQ',
                                  '--case-name', f'C{step}_GQ_{build}'])


def record_progress(step, build, action):
    """Maintain one current status alongside the accumulated checkpoint receipts."""
    progress = NIGHT / 'progress.md'
    text = progress.read_text()
    cut = '\n## Phase C checkpoint progress\n'
    if cut not in text:
        text = text[:text.index('C1 was pushed')] + cut
    first, history = text.split(cut, 1)
    history = '\n'.join(line for line in history.splitlines() if not line.startswith('Current action'))
    text = first + cut + history.rstrip() + f'\n\nCurrent action — C{step}, build{build}: {action}\n'
    progress.write_text(text)


def pack(path):
    """Preserve the raw evidence and add a compact archive for the checkpoint."""
    archive = path.with_suffix(path.suffix + '.gz')
    if not archive.exists():
        with path.open('rb') as source, gzip.GzipFile(archive, mode='wb', mtime=0) as sink:
            shutil.copyfileobj(source, sink)
    return archive


def seal_prefix(workspace, step, build, gate):
    """Keep the full reference comparison and terminal process/resource state for a prefix."""
    case = workspace / f'cases/C{step}_{gate}_{build}'
    status = read(case / 'status.json')
    compared = read(case / f'{gate.lower()}_build{build}_comparison.json')
    assert status['state'] == 'terminal' and status['gate_passed'] and status['all_owned_handles_terminal']
    assert status['source_and_binary_pins_unchanged'] and status['inputs_unchanged']
    assert all(terminal(row) for row in status['owned_identities'])
    assert compared['passed'] and compared['strict_old_integer_rows'] and compared['fixedstorage_required'] == 4120
    assert compared['census_adjustment']['passed']
    samples = [json.loads(line) for line in (case / 'host_resources.jsonl').read_text().splitlines()]
    rank_count = max(sample['rank_process_count'] for sample in samples)
    peak = max(sample['sum_rank_process_peak_hwm_bytes'] for sample in samples)
    assert rank_count == 6 and peak <= 19_999_490_048
    paths = list(case.glob('*.json')) + [case / 'run.cfg'] + [
        pack(case / name) for name in ('run.log', 'history.csv', 'host_resources.jsonl')]
    result = {'comparison': str(case / f'{gate.lower()}_build{build}_comparison.json'),
              'passed': True, 'seconds_until_stop': status['seconds_until_stop'],
              'seconds_total': status['seconds_total'], 'maximum_rank_count': rank_count,
              'observed_prefix_rank_hwm_sum_bytes': peak, 'fixedstorage': 4120, 'census_delta': 0}
    if gate == 'G2':
        actual = status['first_collapse_gate']
        counts = [row['accepted'] for row in actual['per_rank']]
        assert actual['upper_work_sum'] == 90_809_869 and counts == [480, 473, 346, 263, 469, 464]
        result.update(work=actual['upper_work_sum'], accepted_collapses=counts)
    return result, paths


def finalize(step, build, workspace, build_dir, touched):
    """Record each passed identity gate, commit its evidence, then push the checkpoint."""
    assert all(sha(path) == expected for path, expected in read(build_dir / 'source_pins.json').items())
    assert read(build_dir / 'build_status.json')['pins_verified']
    summary = {'state': 'PASSED', 'build': build, 'source_pins': len(read(build_dir / 'source_pins.json')),
               'base_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO, text=True).strip()}
    summary['G1'] = {}
    paths = list(build_dir.glob('*.json')) + [build_dir / 'build.py', PHASE / f'build{build}_aliases.py']
    for cohort in ('serial', 'mpi2', 'mpi4'):
        result = read(workspace / f'G1_candidate_{cohort}_summary.json')
        attempt = Path(result['candidate_attempt'])
        receipt = read(attempt / 'n11_terminal_receipt.json')
        assert result['identity_pass'] and result['changed_case_count'] == 0
        assert receipt['all_owned_handles_terminal'] and all(terminal(row) for row in receipt['owned_processes'])
        summary['G1'][cohort] = {'case_records': result['expected_cases_per_rank'] * result['ranks'],
                                'elapsed_seconds': result['elapsed_wall_seconds'], 'changed_cases': 0}
        paths += [attempt / file for file in ('status.json', 'command.json', 'n11_terminal_receipt.json')]
        paths += [pack(attempt / file) for file in ('run.log', 'resources.jsonl')]
    for gate in ('G2', 'GQ') if step in REPAIR_PREFIX_STEPS else ('G2',):
        summary[gate], artifacts = seal_prefix(workspace, step, build, gate)
        paths += artifacts
    if step >= 8:
        case, = (workspace / 'cases').glob(f'C{step}_G3_*')
        summary['G3'] = read(case / 'g3_end_b_gate.json')
        assert summary['G3']['passed']
        paths += list(case.glob('*.json')) + [case / 'run.cfg'] + [pack(case / file) for file in (
            'run.log', 'resources.jsonl')]
    if step == 11:
        summary['G4'] = read(workspace / 'g4_case_paths.json')
        assert summary['G4']['passed']
        for record in summary['G4']['new_cases']:
            case = Path(record['case'])
            paths += list(case.glob('*.json')) + [pack(case / file) for file in ('run.log', 'resources.jsonl')]
    report = workspace / 'acceptance_summary.json'
    if report.exists():
        assert read(report) == summary
    else:
        save(report, summary)
    counts = [row['case_records'] for row in summary['G1'].values()]
    record_progress(step, build, f'passed G1 {counts}, G2 {summary["G2"]["seconds_until_stop"]:.3f} s; see scoped gate receipts.')
    paths += list(workspace.glob('*.json')) + list(workspace.glob('*.py'))
    paths += [NIGHT / 'progress.md', NIGHT / 'QUESTIONS_FOR_REVIEWER.md',
              PHASE / 'phase_C/run_cut_step.py', PHASE / 'phase_C/reviewed_draft_pins.json', workspace / 'run_g2_gq_prefix.py']
    subprocess.run(['git', 'diff', '--cached', '--quiet'], cwd=REPO, check=True)
    subprocess.run(['git', 'add', '-f', '--', *map(str, sorted(set(paths + touched)))], cwd=REPO, check=True)
    subprocess.run(['git', 'diff', '--cached', '--check', '--', str(SOURCE)], cwd=REPO, check=True)
    subprocess.run(['git', 'commit', '-q', '-m', f'Qualify phase C sub-step C{step} on build{build}'],
                   cwd=REPO, check=True)
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO, text=True).strip()
    subprocess.run(['git', 'push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl'], cwd=REPO, check=True)
    progress = NIGHT / 'progress.md'
    text = progress.read_text()
    current = text.index('Current action —')
    text = text[:current] + f'- C{step} PASSED at `{head[:10]}`, build{build}, G1 {counts} exact, G2 work 90,809,869 and rank counts exact.'
    for gate in ('G3', 'G4', 'GQ'):
        if gate in summary:
            text += f' {gate} PASSED.'
    text += f' Receipts in `N4/section6_3_after_N11/C{step}/validation`.\n'
    next_action = f'C{step + 1}' if step < 11 else 'check the authorized M27 handoff before phase C G5'
    progress.write_text(text + f'\nCurrent action — {next_action}.\n')
    print(f'C{step} checkpoint pushed: {head}', flush=True)


def main():
    """Run one step or a sequence whose drafts already have the required source review."""
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('first', type=int, choices=range(2, 12))
    parser.add_argument('--last', type=int, choices=range(2, 12))
    parser.add_argument('--seal-c2', action='store_true')
    args = parser.parse_args()
    assert Path.cwd() == REPO and REPO.name == 'SU2_NativeIntegrated'
    assert (Path('/proc/1/comm').read_text().strip() == 'systemd'), 'requires actual-host /proc'
    last = args.first if args.last is None else args.last
    assert args.first <= last and (args.first != 2 or args.seal_c2)
    for step in range(args.first, last + 1):
        build = 558 + step
        workspace = PHASE / f'C{step}/validation'
        build_dir = PHASE / f'build{build}'
        try:
            if step == 2:
                touched = [SOURCE / 'CNativeEngine3D.cpp'] + [
                    SOURCE / 'native_engine3d' / name for name in ('EngineQueues.hpp', 'EngineQueues.cpp', 'EngineExchange.cpp')]
            else:
                record_progress(step, build, 'verifying predecessor and applying the reviewed extraction.')
                touched = prepare_candidate(step, build, workspace, build_dir)
                record_progress(step, build, 'candidate applied; running the scoped identity gates.')
                qualify(step, build, workspace, build_dir)
            finalize(step, build, workspace, build_dir, touched)
        except Exception as error:
            record_progress(step, build, f'controller stopped at {error}. Preserve the case and verify handles before diagnosis.')
            raise



if __name__ == '__main__':
    main()
