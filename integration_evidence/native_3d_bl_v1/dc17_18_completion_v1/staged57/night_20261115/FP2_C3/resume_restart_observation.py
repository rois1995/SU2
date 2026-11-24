"""Repeat only the build664 S restart after a transient startup-sample EIO."""

from pathlib import Path
import runpy
import sys

launcher = runpy.run_path(str(Path(__file__).with_name('run_fp2_b_g5.py')))
workflow, cut, sha = (launcher[key] for key in ('workflow', 'cut', 'sha'))
HERE = Path(__file__).resolve().parent
ARCHIVE = 'restart_attempt1_observation'


def corrected_helper(work, label):
    old = f'run_restart{label}.py'
    new = f'run_restart{label}_observation.py'
    target = work / new
    source = (work / old).read_text().replace(old, new)
    anchor = '            compile(source, str(BASE), "exec")\n'
    inherited = '''            except (OSError,RuntimeError,ValueError) as exc:
                # Preserve the failed observation while continuing to own/reap the bounded CFD job.
                runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})'''
    corrected = '''            except OSError as exc:
                if exc.errno == __import__("errno").EIO:
                    # A failed read is an incomplete sample. All six ranks must still verify together.
                    with (FOLDER / "runtime_sample_eio.jsonl").open("a") as observations:
                        observations.write(json.dumps({"time": time.time(), "error": str(exc)}) + "\\n")
                else:
                    runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})
            except (RuntimeError,ValueError) as exc:
                runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})'''
    source = workflow.replace_once(source, anchor,
        f'            old_observation = {inherited!r}\n'
        f'            new_observation = {corrected!r}\n'
        '            assert source.count(old_observation) == 1, old_observation\n'
        '            source = source.replace(old_observation, new_observation, 1)\n' + anchor)
    compile(source, str(target), 'exec')
    if target.exists():
        assert target.read_text() == source, target
    else:
        with target.open('x') as handle:
            handle.write(source)
    return target


def archive_attempt(work, restart):
    archive = work / ARCHIVE
    manifest = archive / 'manifest.json'
    if not manifest.exists():
        launcher['terminal_wait'](work, 'restart', 1)
        status = cut.read(restart / 'status.json')
        assert status['state'] == 'terminal' and status['exit_code'] == 0
        assert status['pins_verified'] and status['resource_gate']['passed']
        assert not status['runtime_receipt_written']
        assert status['runtime_observation_error'] == '[Errno 5] Input/output error'
        post = cut.read(restart / 'post_terminal_handles.json')
        assert post['actual_host_proc'] and post['all_owned_handles_terminal']
        handles = [post['controller_identity'], post['controller_wrapper_identity']]
        handles.extend(row['identity'] for row in post['owned_identity_checks'])
        assert all(cut.terminal(row) for row in handles)
        failure = cut.read(work / 'failed_attempt.json')
        assert failure['stage'] == 'isolated restart' and not failure['passed']
        for name, digest in failure['artifacts'].items():
            assert sha(work / name) == digest, name
        paths = [restart, work / f'{restart.name}_controller_launch.json',
                 *(work / name for name in ('restart_host_preflight.json', 'restart_launch.json',
                                            'restart_wait.json', 'restart_outer.log', 'failed_attempt.json'))]
        files = [file for path in paths for file in (path.rglob('*') if path.is_dir() else [path])
                 if file.is_file()]
        assert not any(path.is_symlink() for path in (*paths, *files))
        archive.mkdir(exist_ok=True)
        cut.save(manifest, {'moved_paths': [path.name for path in paths],
                            'files': {str(path.relative_to(work)): sha(path) for path in sorted(files)}})
    record = cut.read(manifest)
    for name in record['moved_paths']:
        if not (archive / name).exists():
            (work / name).rename(archive / name)
    for name, digest in record['files'].items():
        assert sha(archive / name) == digest, name
    return manifest


def resume(build_dir):
    launcher['verify_orders']()
    prior = launcher['prior']
    build, _, _ = prior.lab.prepare(build_dir)
    label = build['build']
    work = launcher['work_path'](label)
    case = work / f'full{label}_run1'
    restart_case = work / f'disk_restart_{case.name}_event2'
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    launcher['reference_sources'](payloads)
    launcher['prerequisite_sources'](build)
    config = prior.config_for('S')
    with launcher['bound']():
        launcher['terminal_wait'](work, 'full', 0)
        workflow.verify_helpers(work)
        prior.BASE['status_ok'](case, build, config)
        launcher['require_clean_contact_logs'](work, case)
        hashes = workflow.seal_run(case, build['binary_sha256'], config, payloads, True)
        events = workflow.event_collector()(case)
        target = corrected_helper(work, label)
        import subprocess
        subprocess.run([cut.PYTHON, '-B', str(target), '--self-check'], cwd=cut.REPO, check=True)
        manifest = archive_attempt(work, restart_case)
        workflow.run_stage(work, 'restart', [cut.PYTHON, '-B', str(target), '--source-case', str(case)])
        verify = runpy.run_path(str(workflow.F_WORKFLOW))['verify_restart']
        verify.__globals__['WORKSPACE'] = work
        restart = verify(label, case)
        workflow.verify_helpers(work)
        artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                               'restart_launch.json', 'restart_wait.json')]
        artifacts += [target, manifest, restart, HERE / 'reports/FP2_C3_S_restart_diagnosis_664.md',
                      Path(__file__).resolve(), work / 'restart_host_preflight.json']
        artifacts += [case / name for name in ('provenance.json', 'status.json', 'full_workflow_receipt.json',
                                               'n8_payload_identity.json')]
        artifacts += sorted(case.glob('p7p8_event*_gate.json')) + sorted(case.glob('f3b_event*_timer_gate.json'))
        artifacts += sorted(case.glob('rank_output/*/rank.*/stdout')) + sorted(case.glob('rank_output/*/rank.*/stderr'))
        artifacts += sorted(restart_case.glob('runtime_sample_eio.jsonl'))
        reference = launcher['s0_receipt']()
        record = {**{key: build[key] for key in prior.lab.FINGERPRINTS}, 'passed': True, 'mode': 'S',
                  'case': str(case), 'hashes': hashes, 'restart': str(restart), 'events': events,
                  'config_sha256': sha(config), 'restart_observation_retry': str(manifest),
                  'report_reference': {'commit': launcher['S0'], 'receipt': str(reference), 'sha256': sha(reference)},
                  'artifacts': {str(path.relative_to(work) if path.is_relative_to(work) else path): sha(path)
                                for path in artifacts}}
        workflow.sealed_write(work / 'run_receipt.json', record)
        prior.verify_run(work, 'S', build, payloads)
    print('Isolated restart verified. The original S observation is sealed.', flush=True)


if __name__ == '__main__':
    resume(Path(sys.argv[1]).resolve())
