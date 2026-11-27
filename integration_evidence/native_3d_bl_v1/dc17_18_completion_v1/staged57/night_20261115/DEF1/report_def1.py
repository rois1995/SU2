"""Seal the DEF1 standing gates and render the capability disposition from saved receipts."""

from argparse import ArgumentParser
import json
import os
from pathlib import Path

from build_fp3 import HERE, AD_BUILD, cut, git, verify_build
from run_lab import DIAGNOSTICS

NIGHT = HERE.parent
REFERENCE = NIGHT / 'FP3_E/accepted_reference.json'
SCOPED = ('G1-serial', 'G1-mpi2', 'G1-mpi4', 'hidden', 'G2', 'GQ', 'first-collapse', 'exact-c492',
          'merit481', 'native2d', 'saved-current', 'G3', 'G4')
FINGERPRINTS = ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')
STANDING_LABEL = 'DEF1: defaults norm 2 and gradation 1.6; standing checks passed'
NOT_EVALUATED = ('full OFF workflow', 'full CORE workflow')


def pin(path):
    return {'path': str(path.resolve()), 'sha256': cut.sha(path)}


def checked(pointer):
    path = Path(pointer['path'])
    assert cut.sha(path) == pointer['sha256'], path
    return cut.read(path)


def receipts(label):
    build = HERE / f'build{label}'
    lab = HERE / 'lab' / build.name
    return {
        'normal build': build / 'build_status.json',
        'reverse AD build': build / 'build_ad_status.json',
        'normal lab': lab / 'normal/receipt.json',
        'reverse AD lab': lab / 'ad/receipt.json',
        'scoped standing checks': HERE / 'controls' / build.name / 'step1_scoped.json',
        'standing S workflow': HERE / 'G5' / build.name / 'S1/case_summary.json',
    }


def validate_lab(path, mode, binary_sha, pins):
    receipt = cut.read(path)
    assert receipt['passed'] is True and receipt['mode'] == mode, path
    assert receipt['binary_sha256'] == binary_sha == cut.sha(receipt['binary']), path
    runner = HERE / 'run_lab.py'
    assert cut.sha(runner) == receipt['runner_sha256'], path
    pins[str(runner)] = receipt['runner_sha256']
    expected = [mode, *DIAGNOSTICS] if mode == 'normal' else [mode]
    assert [row['case'] for row in receipt['cases']] == expected, path
    for row in receipt['cases']:
        assert row['passed'] is True and row['binary_sha256'] == binary_sha, row['case']
        folder = path.parent / row['case']
        for artifact, key in (('stdout', 'stdout'), ('stderr', 'stderr'), ('host_preflight.json', 'host_preflight')):
            pins[str(folder / artifact)] = row[key]
        if row['case'] in DIAGNOSTICS:
            assert row['expected_diagnostic'] == DIAGNOSTICS[row['case']] and row['returncode'] != 0, row['case']
        else:
            assert row['returncode'] == 0 and len(row['selected_tests']) == (4 if mode == 'normal' else 1), row


def merge_pins(target, additional):
    for path, digest in additional.items():
        assert target.setdefault(path, digest) == digest, path


def section3(label):
    paths = receipts(label)
    direct = verify_build(label)
    assert direct['source_commit'] == git('rev-parse', direct['source_commit']).decode().strip()
    paths_pin = {name: pin(path) for name, path in paths.items()}
    pins = {item['path']: item['sha256'] for item in paths_pin.values()}
    reverse = cut.read(paths['reverse AD build'])
    assert reverse['passed'] is True and reverse['build'] == label, paths['reverse AD build']
    assert all(reverse[key] == direct[key] for key in ('source_commit', 'source_pins_sha256'))
    assert reverse['wait']['terminal'] and reverse['wait']['returncode'] == 0
    assert cut.terminal(reverse['wait']['controller'])
    assert cut.sha(reverse['binary']) == reverse['binary_sha256']
    assert all(cut.sha(AD_BUILD / name) == digest for name, digest in reverse['graph_after'].items())

    validate_lab(paths['normal lab'], 'normal', direct['test_binary_sha256'], pins)
    validate_lab(paths['reverse AD lab'], 'ad', reverse['binary_sha256'], pins)
    for name in ('scoped standing checks', 'standing S workflow'):
        gate = cut.read(paths[name])
        assert gate['status'] == 'passed', paths[name]
        assert all(gate[key] == direct[key] for key in FINGERPRINTS), paths[name]
        merge_pins(pins, gate['pins'])
    scoped = cut.read(paths['scoped standing checks'])
    assert scoped['passed'] is True and set(scoped['parts']) == set(SCOPED)
    for part in scoped['parts'].values():
        child = checked(part)
        assert part['status'] == child['status'] == 'passed' and child['passed'] is True, part['path']
        assert all(child[key] == direct[key] for key in FINGERPRINTS), part['path']

    full = cut.read(paths['standing S workflow'])
    assert full['comparison']['equal'] is True and full['comparison']['differences'] == []
    assert set(full['legacy_configs']) == {'full', 'isolated_restart'}
    run = checked(full['run'])
    assert run['passed'] is True and run['mode'] == 'S' and run['probe'] == 0
    assert set(run['switches']) == {'full', 'restart'}
    assert all(run[key] == direct[key] for key in FINGERPRINTS)
    restart = cut.read(run['restart'])
    assert restart['disk_restart_verified'] and restart['all_inputs_and_pins_unchanged']
    assert restart['artifact_sha256']['run.cfg'] == full['legacy_configs']['isolated_restart']['derived_sha256']
    assert run['config_sha256'] == full['legacy_configs']['full']['derived_sha256']
    assert REFERENCE.read_bytes() == git('show', f'efbab5351e:{REFERENCE.relative_to(cut.REPO)}')
    historical = cut.read(REFERENCE)
    assert historical['accepted'] and historical['build'] == 680
    historical_runs = historical['workflows']['S']['runs']
    assert len(historical_runs) == 2 and len(run['hashes']) == 6
    assert all(item['payload_hashes'] == run['hashes'] for item in historical_runs)
    for path in (REFERENCE, Path(run['restart']), Path(full['run']['path'])):
        item = pin(path)
        merge_pins(pins, {item['path']: item['sha256']})

    for path, digest in pins.items():
        assert cut.sha(path) == digest, path
    record = {
        'status': 'passed', 'passed': True, 'label': STANDING_LABEL, 'build': label,
        'source_commit': direct['source_commit'], 'binary_sha256': direct['binary_sha256'],
        'build_status': paths_pin['normal build'], 'gates': paths_pin, 'pins': pins,
        'reference': pin(REFERENCE), 'not_evaluated': list(NOT_EVALUATED),
    }
    destination = paths['normal build'].parent / 'section3.json'
    if destination.is_file():
        assert cut.read(destination) == record, destination
    else:
        cut.save(destination, record)
    return record


def as_status(data):
    if 'status' in data:
        return data['status']
    return 'passed' if data.get('passed') else 'FAILED'


def capabilities(label):
    result = {}
    for mode in ('CL2', 'MMG2'):
        base = NIGHT / mode / 'reports' / f'{mode}_{label}'
        raw, markdown = base.with_suffix('.json'), base.with_suffix('.md')
        if not raw.is_file() and not markdown.is_file():
            result[mode] = {'status': 'not evaluated'}
            continue
        data = cut.read(raw)
        assert markdown.is_file() and data['launch']['build'] == label
        assert data['label'].startswith(mode + ':'), data['label']
        result[mode] = {'status': data['label'], 'first_finding': data['finding'],
                        'runtime_defaults': data['runtime_defaults'], 'terminal': data['terminal'],
                        'binary_sha256': data['launch']['binary_sha256'],
                        'report': pin(markdown), 'json_report': pin(raw)}
    return result


def report_data(label, failure=None, open_check=None, revert=None, diagnosis=()):
    paths = receipts(label)
    build_dir = paths['normal build'].parent
    section = build_dir / 'section3.json'
    scope = paths['scoped standing checks'].parent
    gates = {}
    for title, path in {**paths, **{name: scope / name / 'case_summary.json' for name in SCOPED}}.items():
        if path.is_file():
            data = cut.read(path)
            gates[title] = {'status': as_status(data), 'receipt': pin(path)}
            if first := data.get('first_difference'):
                gates[title]['first_difference'] = first
            if data.get('cases'):
                gates[title]['cases'] = data['cases']
            if data.get('legacy_configs'):
                gates[title]['legacy_configs'] = data['legacy_configs']
            if data.get('warnings'):
                gates[title]['warnings'] = data['warnings']
        else:
            gates[title] = {'status': 'not evaluated'}

    runtime = capabilities(label)
    unvisited = any(run['status'] == 'not evaluated' for run in runtime.values())
    wrong_default = any(run.get('runtime_defaults', {}).get('wiring_defect') for run in runtime.values())
    if failure:
        assert revert, 'A FAILED report needs the restoration commit.'
        title = f'DEF1: FAILED at {failure}'
        state = 'FAILED'
    elif open_check:
        title = f'DEF1: OPEN at {open_check}'
        state = 'OPEN'
    else:
        assert section.is_file() and not revert, 'Standing checks must be sealed before reporting success.'
        assert not wrong_default, 'Use the R5 branches for a proven runtime wiring defect.'
        title = STANDING_LABEL
        state = 'standing checks passed' if unvisited else 'accepted'

    sources = HERE / 'reports'
    direct = cut.read(paths['normal build']) if paths['normal build'].is_file() else {}
    if section.is_file():
        sealed = cut.read(section)
        assert sealed['passed'] and sealed['status'] == 'passed'
        assert sealed['build'] == label and sealed['source_commit'] == direct['source_commit']
        for name, pointer in sealed['gates'].items():
            assert checked(pointer) and gates[name]['receipt'] == pointer
    for run in runtime.values():
        if run['status'] != 'not evaluated':
            assert run['binary_sha256'] == direct['binary_sha256']
    return {
        'label': title, 'status': state, 'build': label, 'source_commit': direct.get('source_commit'),
        'binary_sha256': direct.get('binary_sha256'), 'section3': pin(section) if section.is_file() else None,
        'revert': git('rev-parse', revert).decode().strip() if revert else None,
        'reverted_candidate': direct.get('source_commit') if revert else None,
        'diagnoses': [pin(path) for path in diagnosis],
        'source_check': pin(sources / 'source_check.md'),
        'touched_files_receipt': pin(sources / 'touched_files.txt'),
        'touched_files': (sources / 'touched_files.txt').read_text().splitlines(),
        'gates': gates, 'capabilities': runtime, 'not_evaluated': list(NOT_EVALUATED),
        'reference': pin(REFERENCE), 'reference_changes': [],
    }


def relative(pointer, report):
    return os.path.relpath(pointer['path'], report.parent)


def report_md(data, destination):
    lines = [
        f'# DEF1 build {data["build"]}', '', f'**{data["label"]}**', '',
        f'Solver source commit `{data["source_commit"]}`. Solver SHA256 `{data["binary_sha256"]}`.',
        'The existing mode S reference stays in force. '
        f'[FP3-E accepted S reference]({relative(data["reference"], destination)})', '',
    ]
    if data['revert']:
        lines.extend([f'Restored the previous production behavior in `{data["revert"]}`. '
                      f'The candidate source commit was `{data["reverted_candidate"]}`.', ''])
    lines.extend(['## Whole-item source check', '',
                  f'[Source-check receipt]({relative(data["source_check"], destination)})', ''])
    source = Path(data['source_check']['path']).read_text().splitlines()
    lines.extend('#' + line if line.startswith('## ') else line for line in source[2:])
    lines.extend(['', '## Files changed for production and legacy fixtures', '',
                  f'[File inventory]({relative(data["touched_files_receipt"], destination)})', '',
                  '<details><summary>Changed paths</summary>', '', '```text', *data['touched_files'],
                  '```', '', '</details>', '', '## Section 3 standing gates and default fixtures', '',
                  '| Check | Status | Evidence |', '| --- | --- | --- |'])
    for name, gate in data['gates'].items():
        evidence = f'[Receipt]({relative(gate["receipt"], destination)})' if gate.get('receipt') else '—'
        lines.append(f'| {name} | {gate["status"]} | {evidence} |')
    for name in data['not_evaluated']:
        lines.append(f'| {name} | not evaluated | — |')
    lines.append('')
    if data['section3']:
        lines.extend([f'[Authenticated section 3 seal]({relative(data["section3"], destination)})', ''])
    for name, gate in data['gates'].items():
        if difference := gate.get('first_difference'):
            lines.extend([f'First difference in {name}', '', '```json',
                          json.dumps(difference, indent=2, ensure_ascii=False), '```', ''])
    if configs := data['gates']['standing S workflow'].get('legacy_configs'):
        lines.extend(['### Standing S effective configuration', '',
                      '| Launch | Sealed SHA256 | Lines inserted | Derived SHA256 |', '| --- | --- | --- | --- |'])
        for name, cfg in configs.items():
            inserted = ', '.join(cfg['inserted_lines']) or 'none'
            lines.append(f'| {name} | `{cfg["sealed_sha256"]}` | {inserted} | `{cfg["derived_sha256"]}` |')
        lines.append('')

    lines.extend(['## Default and marker fixtures', '',
                  'The normal lab exercises the pure GOAL rule, mesh-loop scalar and per-level defaults, '
                  'both Hausdorff forms and their inheritance, and the actual 2D/3D setter rows. The AD lab '
                  'exercises the GOAL scalar and per-level choices through CConfig, including its warning.', '',
                  '| Cohort | Case | Outcome |', '| --- | --- | --- |'])
    for name in ('normal lab', 'reverse AD lab'):
        for case in data['gates'][name].get('cases', []):
            outcome = 'passed' if case['passed'] else 'FAILED'
            lines.append(f'| {name} | {case["case"]} | {outcome} |')
    lines.extend(['', '## Capability observations', ''])
    for run, capability in data['capabilities'].items():
        if capability.get('report'):
            outputs = (f'[{run} report]({relative(capability["report"], destination)}) · '
                       f'[JSON]({relative(capability["json_report"], destination)})')
            lines.extend([f'- {capability["status"]}. {outputs}'])
        else:
            lines.append(f'- {run} was not evaluated.')
    if data['diagnoses']:
        lines.extend(['', '## Diagnoses', ''])
        for pointer in data['diagnoses']:
            lines.append(f'- [{Path(pointer["path"]).name}]({relative(pointer, destination)})')
    return '\n'.join(lines).rstrip() + '\n'


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--seal', action='store_true')
    group.add_argument('--failure', metavar='GATE')
    group.add_argument('--open', metavar='CHECK')
    parser.add_argument('--revert', metavar='COMMIT')
    parser.add_argument('--diagnosis', type=Path, action='append', default=[])
    args = parser.parse_args()
    if args.seal:
        result = section3(args.build)
    else:
        result = report_data(args.build, failure=args.failure, open_check=args.open,
                             revert=args.revert, diagnosis=args.diagnosis)
        stem = HERE / 'reports' / f'DEF1_{args.build}'
        markdown = stem.with_suffix('.md')
        markdown.write_text(report_md(result, markdown))
        stem.with_suffix('.json').write_text(json.dumps(result, indent=2, allow_nan=False) + '\n')
    print(result['label'])
