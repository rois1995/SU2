"""Recover the equal-list isolated restart after its prelaunch scalar config assertion."""

from argparse import ArgumentParser
import ast
from pathlib import Path
import runpy
import subprocess
from unittest.mock import patch

import run_full as full
from build_fp3 import cut, verify_build

E, workflow = full.E, full.workflow
ARCHIVE = 'restart_attempt1_measurement'


def archive_failure(folder, label):
    work = folder / 'trial1'
    restart_name = f'disk_restart_full{label}_run1_event2'
    archive = work / ARCHIVE
    manifest = archive / 'manifest.json'
    if not manifest.is_file():
        result = cut.read(folder / 'case_summary.json')
        assert result['failure_kind'] == 'measurement' and not result['passed']
        E.check_pins(result['pins'])
        E.c3.terminal_wait(work, 'full', 0)
        E.c3.terminal_wait(work, 'restart', 1)
        failure = cut.read(work / 'failed_attempt.json')
        assert failure['stage'] == 'isolated restart'
        restart = work / restart_name
        assert not (restart / 'terminal_handles.json').exists() and not (restart / 'run.cfg').exists()
        log = (work / 'restart_outer.log').read_text()
        assert 'AssertionError' in log and 'line 344, in <module>' in log
        paths = [
            folder / 'case_summary.json', restart,
            *(work / name for name in (f'{restart_name}_controller_launch.json', 'failed_attempt.json',
                                       'restart_host_preflight.json', 'restart_launch.json', 'restart_wait.json',
                                       'restart_outer.log')),
        ]
        files = sorted(file for path in paths for file in (path.rglob('*') if path.is_dir() else (path,))
                       if file.is_file())
        archive.mkdir()
        workflow.sealed_write(manifest, {
            'moved_paths': {str(path): path.name for path in paths},
            'files': {str(archive / (file.name if file.parent != restart else f'{restart_name}/{file.name}')):
                      full.sha(file) for file in files},
            'observation': 'The restart config already carried the equal list. The scalar-only byte assertion stopped '
                           'before writing the config or starting the solver.',
        })
    row = cut.read(manifest)
    for old, name in row['moved_paths'].items():
        destination = archive / name
        if not destination.exists():
            Path(old).rename(destination)
    E.check_pins(row['files'])
    return {str(manifest): full.sha(manifest), **row['files']}


def corrected_helper(work, label, expected):
    original = work / f'run_restart{label}.py'
    target = work / f'run_restart{label}_equal_recovery.py'
    source = original.read_text().replace(original.name, target.name)
    lines = source.splitlines(keepends=True)
    i, = (index for index, line in enumerate(lines) if line.startswith('            legacy_guard = '))
    prefix = '            legacy_guard = '
    guard = ast.literal_eval(lines[i][len(prefix):].strip())
    scalar_comparison = 'assert config.encode() == '
    equal_projection = f'assert config.replace({full.EQUAL.decode()!r}, {full.SCALAR.decode()!r}, 1).encode() == '
    lines[i] = prefix + repr(workflow.replace_once(guard, scalar_comparison, equal_projection)) + '\n'
    source = ''.join(lines)
    compile(source, str(target), 'exec')
    if target.exists():
        assert target.read_text() == source
    else:
        target.write_text(source)
    helper = runpy.run_path(str(target))
    worker = helper['runner_context']().load_base().retargeted_source()
    at = worker.index('from pathlib import Path\nexpected_restart')
    guard = worker[at:worker.index('(FOLDER / "run.cfg").write_text(config)', at)]
    check = {'config': expected.decode()}
    exec(compile(guard, '<equal restart config check>', 'exec'), check)
    assert check['config'].encode() == expected
    return target


def resume(label):
    build = verify_build(label)
    folder = full.root(label, 'equal')
    work = folder / 'trial1'
    case = work / f'full{label}_run1'
    payloads, expected, _, _, settings = full.reference()
    config = full.full_config(label, 'equal')
    restart_config, _ = full.derive(settings[0].read_bytes(), 'equal')
    E.c3.terminal_wait(work, 'full', 0)
    workflow.verify_helpers(work)
    E.base['status_ok'](case, build, config)
    hashes = workflow.workflow_payloads(case, build['binary_sha256'], payloads)
    assert hashes == expected
    target = corrected_helper(work, label, restart_config)
    archived = archive_failure(folder, label)
    support = [target, Path(__file__).resolve(), full.HERE / 'reports/equal_restart_recovery.md']

    def restart_only(trial_build, part, runtime_build, sources, runtime_payloads):
        assert (trial_build, part, runtime_build, runtime_payloads.keys()) == (label, 'equal', build, payloads.keys())
        sealed = workflow.seal_run(case, build['binary_sha256'], config, payloads, True)
        assert sealed == hashes
        events = workflow.event_collector()(case)
        collected, record = E.collect(work, case, part, events=events)
        assert record['status'] == 'passed', record
        subprocess.run([cut.PYTHON, '-B', str(target), '--self-check'], cwd=cut.REPO, check=True)
        try:
            workflow.run_stage(work, 'restart', [cut.PYTHON, '-B', str(target), '--source-case', str(case)])
            verify = runpy.run_path(str(workflow.F_WORKFLOW))['verify_restart']
            verify.__globals__['WORKSPACE'] = work
            restart = verify(label, case)
        except (OSError, RuntimeError, AssertionError, subprocess.CalledProcessError) as error:
            kind, first = E.restart_problem(work, case, error)
            raise E.FailedCheck(kind, first) from error
        E.c3.require_clean_contact_logs(work, restart.parent)
        return E.envelope(work, label, part, build, events, hashes, restart, collected)

    prior_pins = E.full_pins
    with patch.object(E, 'run_trial', restart_only), patch.object(
        E, 'full_pins', lambda path, row: prior_pins(path, row) | archived |
                                        {str(file): full.sha(file) for file in support}
    ):
        return full.run(label, 'equal')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    raise SystemExit({'passed': 0, 'FAILED': 1, 'OPEN': 75}[resume(parser.parse_args().build)])
