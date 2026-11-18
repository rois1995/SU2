"""Preserve the executed F gates and publish the accepted or reverted disposition."""
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from report_summary import BRANCH, BUILD, REPORTS, SCOPED, WORKSPACE, write
from run_de import N18, checkpoint, cut, git, progress


def identity_rows(value):
    if isinstance(value, dict):
        if isinstance(value.get('pid'), int) and isinstance(value.get('start_ticks'), int):
            yield {'pid': value['pid'], 'start_ticks': value['start_ticks']}
        for item in value.values():
            yield from identity_rows(item)
    elif isinstance(value, list):
        for item in value:
            yield from identity_rows(item)


def under_cases():
    """Enumerate scoped, full and safeguard results that actually ran."""
    yield from sorted((SCOPED / 'cases').glob('*'))
    g4 = SCOPED / 'g4_case_paths.json'
    if g4.exists():
        yield from (Path(row['case']) for row in cut.read(g4)['new_cases'])
    yield from sorted((BRANCH / 'safeguards/cases').glob('*'))
    yield from sorted(path for path in WORKSPACE.iterdir() if path.is_dir() and
                      path.name.startswith(('full', 'disk_restart_full')))


def archive(path):
    packed = cut.pack(path)
    with gzip.open(packed, 'rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    assert digest == cut.sha(path)
    return packed, {'path': str(path), 'sha256': digest, 'archive_sha256': cut.sha(packed)}


def evidence():
    cases = list(under_cases())
    locations = (SCOPED, BRANCH / 'safeguards', WORKSPACE, cut.PHASE / f'build{BUILD}', N18)
    files = [path for location in locations if location.is_dir()
             for path in location.iterdir() if path.is_file() and path.suffix in ('.json', '.py')]
    files += [cut.PHASE / f'build{BUILD}_aliases.py']
    for case in cases:
        files += list(case.glob('*.json'))
        files += [path for path in case.glob('*.cfg')]
    for folder, names in (
        ('drafts/recipes', ('workflow_f.py', 'check_without_launch.py', 'no_launch_check.log', 'RECIPE.md', 'SHA256SUMS')),
        ('drafts/reporting', ('report_m28.py', 'check_report_m28.py', 'self_check.log')),
        ('qualified_build579', ('manifest.json',)),
    ):
        files += [N18 / folder / name for name in names]
    raw = [*N18.glob('*campaign.log'), *SCOPED.glob('*outer.log'), *WORKSPACE.glob('*outer.log')]
    for case in cases:
        raw += [case / name for name in ('run.log', 'host_resources.jsonl', 'resources.jsonl',
                  'history.csv', 'continued508_history.csv') if (case / name).is_file()]
        raw += [path for path in case.glob('rank_output/**/rank.*/*') if path.name in ('stdout', 'stderr')]
    records = {}
    for log in sorted(set(raw)):
        packed, row = archive(log)
        records[str(packed)] = row
        files.append(packed)
    cut.save(N18 / 'F_log_archives.json', records)
    files.append(N18 / 'F_log_archives.json')
    return files


def audit(build, output):
    files = list(N18.glob('*_launch.json')) + list(N18.glob('*_wait.json'))
    files += list((cut.PHASE / f'build{build}').glob('*terminal*.json'))
    for parent in (cut.PHASE / 'N18DE/validation', SCOPED, WORKSPACE, BRANCH / 'safeguards'):
        if not parent.exists():
            continue
        files += list(parent.glob('*_launch.json')) + list(parent.glob('*_wait.json'))
        files += list(parent.glob('g4_case_paths.json'))
        files += list(parent.glob('*/**/*terminal*receipt.json'))
        files += list(parent.glob('*/**/safeguard_process_receipt.json'))
        files += list(parent.glob('*/**/post_terminal_handles.json'))
        files += list(parent.glob('*/**/status.json'))
    rows = {}
    for path in set(files):
        for row in identity_rows(cut.read(path)):
            rows[row['pid'], row['start_ticks']] = row
    assert rows and all(cut.terminal(row) for row in rows.values())
    source = cut.PHASE / f'build{build}/source_pins.json'
    pins = cut.read(source)
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    host = N18 / f'{output.stem}_host_preflight.json'
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(host)], cwd=cut.REPO, check=True)
    assert not cut.read(host)['active_OWNED_heavy_processes']
    result = {'build': build, 'actual_host_proc': True, 'source_pins_verified': len(pins),
              'all_owned_handles_terminal': True, 'identities': [dict(row, terminal=True) for row in rows.values()],
              'source_pins_sha256': cut.sha(source), 'host_preflight_sha256': cut.sha(host)}
    cut.save(output, result)
    return output, host


def restore_binaries():
    records = cut.read(N18 / 'qualified_build579/manifest.json')['binaries']
    for row in records.values():
        source, shared = Path(row['preserved_copy']), Path(row['shared_path'])
        assert cut.sha(source) == row['sha256']
        pending = shared.with_name(shared.name + '.restore579')
        assert not pending.exists()
        shutil.copyfile(source, pending)
        pending.chmod(shared.stat().st_mode & 0o777)
        assert cut.sha(pending) == row['sha256']
        os.replace(pending, shared)
        assert cut.sha(shared) == row['sha256']


def purpose_failure(result):
    gate = result['purpose']
    if not gate['combined_passed']:
        difference = (f"sum of event minima {gate['smaller_native_max_sum_seconds']} s exceeds "
                      f"{gate['combined_limit_seconds']} s")
    else:
        row = next(row for row in gate['events'] if not row['passed'])
        difference = (f"event {row['event']} minimum {row['smaller_native_max_seconds']} s exceeds "
                      f"{row['T0_native_limit_seconds']} s")
    return {'first_difference': difference,
            'cause': 'The two standing-gate passes reached the confirmed purpose comparison and missed its runtime limit.',
            'evidence': str(WORKSPACE / 'purpose_rule.json')}


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert cut.terminal(cut.read(N18 / 'F_campaign_launch.json'))
    base = cut.read(N18 / 'DE_checkpoint.json')
    assert base['passed'] and git('rev-parse', 'HEAD') == base['commit']
    change = cut.read(N18 / 'F_source_paths.json')
    assert change['base_commit'] == base['commit']
    paths = [cut.REPO / path for path in change['source_sha256']]
    assert all(cut.sha(cut.REPO / path) == digest for path, digest in change['source_sha256'].items())
    result_path = WORKSPACE / 'acceptance_summary.json'
    result = cut.read(result_path) if result_path.exists() else None
    passed = bool(result and result['passed'])
    if result:
        assert result['standing_gates_passed'] and result['repeat_payloads_exact']
        assert result['event_one_prefix_identity'] and result['purpose']['passed'] == passed
    certificate = N18 / 'F_terminal_audit.json'
    audit_paths = audit(BUILD, certificate)
    artifacts = [*evidence(), *audit_paths, *paths, cut.NIGHT / 'progress.md', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md']
    if passed:
        assert (WORKSPACE / 'qualified_reference_payloads.json').is_file()
        reports = write('PASSED')
        progress('F passed both full runs, the isolated restart, six repeat-identical files and the confirmed purpose rule.')
        head = checkpoint('Qualify N18 corrected larger quality rounds and M28', artifacts + reports)
        disposition = {'passed': True, 'commit': head, 'active_build': BUILD, 'F_commit': head}
    else:
        diagnosis_file = N18 / 'failure_diagnosis.json'
        if result:
            outcome = 'FAILED its purpose rule'
            diagnosis = purpose_failure(result)
            cut.save(diagnosis_file, diagnosis)
        else:
            outcome = 'FAILED'
            diagnosis = cut.read(diagnosis_file)
            artifacts.append(diagnosis_file)
        progress(f"F {outcome}. First difference: {diagnosis['first_difference']}. Preserving the candidate evidence.")
        candidate = checkpoint(f'Record N18 F candidate that {outcome}', artifacts)
        diagnosis['candidate_commit'] = candidate
        diagnosis_file.write_text(json.dumps(diagnosis, indent=2) + '\n')
        reports = write(outcome, diagnosis)
        patch = N18 / 'drafts/F/wide.patch'
        assert cut.sha(patch) == change['patch_sha256']
        git('apply', '--reverse', '--check', patch)
        git('apply', '--reverse', patch)
        restore_binaries()
        restored = audit(579, N18 / 'restored579_terminal_audit.json')
        head = checkpoint('Revert N18 F and retain the M28 failure evidence',
                          [*paths, *reports, *restored, diagnosis_file, cut.NIGHT / 'progress.md'])
        disposition = {'passed': False, 'commit': head, 'active_build': 579, 'F_commit': candidate,
                       'first_difference': diagnosis['first_difference'], 'failure': outcome}
    disposition['report'] = str(REPORTS / f'STEP_8_5R2_{BUILD}.md')
    cut.save(N18 / 'F_checkpoint.json', disposition)
    progress(f"N18 source disposition pushed as `{head[:10]}`. The final N7 report follows.")
    print(disposition, flush=True)


if __name__ == '__main__':
    main()
