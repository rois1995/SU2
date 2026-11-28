"""Seal TOL1 standing evidence and report the final disposition."""

from argparse import ArgumentParser
import json
import os
from pathlib import Path

from build_fp3 import HERE, SOURCE_CHECK, cut, git, verify_build
from run_full import E, PARTS, path_pin, prior, reference, root, sha, tolerance_gate, workflow

LABEL = 'TOL1: native tolerance per marker; standing and equal-list checks passed'
NOT_EVALUATED = ('full OFF', 'full CORE')


def inputs(label):
    folder = HERE / f'build{label}'
    return {
        'build': folder / 'build_status.json',
        'scoped': HERE / 'controls' / folder.name / 'step1_scoped.json',
        'fixtures': folder / 'fixtures.json',
        **{part: root(label, part) / 'case_summary.json' for part in PARTS},
    }


def seal(label):
    paths = inputs(label)
    build = verify_build(label)
    gates = {name: E.require(path, build) for name, path in paths.items() if name != 'build'}
    assert all(gate['passed'] for gate in gates.values())
    assert len(gates['scoped']['parts']) == 12
    assert gates['fixtures']['cases'] and gates['fixtures']['fatal_cases']
    _, expected, _, _, _ = reference()
    for part in PARTS:
        summary = gates[part]
        assert summary['part'] == part and summary['probe'] == 0 and summary['comparison']['equal']
        assert set(summary['legacy_configs']) == {'full', 'isolated_restart'}
        for cfg in summary['legacy_configs'].values():
            assert bool(cfg['replacement']) == (part == 'equal'), cfg
        receipt = Path(summary['run']['path'])
        assert path_pin(receipt) == summary['run']
        data = cut.read(receipt)
        assert data['passed'] and data['hashes'] == expected
        assert cut.read(Path(data['restart']))['disk_restart_verified']
    tolerance_gate(gates['equal']['mobility'], gates['equal']['tolerance_rows'], gates['scalar']['mobility'])
    pointers = {name: path_pin(path) for name, path in paths.items()}
    record = {
        **{key: build[key] for key in E.FINGERPRINTS},
        'status': 'passed', 'passed': True, 'label': LABEL, 'source_commit': build['source_commit'],
        'build_status': pointers['build'], 'gates': pointers, 'reference': path_pin(prior.FULL_REF),
        'pins': {str(path): sha(path) for path in [*paths.values(), SOURCE_CHECK, Path(__file__).resolve()]},
        'not_evaluated': list(NOT_EVALUATED),
    }
    workflow.sealed_write(paths['build'].parent / 'section3.json', record)
    return record


def read_status(path):
    if not path.is_file():
        return {'status': 'not evaluated'}
    data = cut.read(path)
    fields = ('first_difference', 'legacy_configs', 'warnings', 'cases', 'fatal_cases', 'tolerance_rows', 'mobility')
    return {key: data[key] for key in fields if key in data} | {
        'status': data.get('status', 'passed' if data.get('passed') else 'FAILED'), 'receipt': path_pin(path)}


def report(label, failure, open_check, revert, diagnoses):
    paths = inputs(label)
    build = cut.read(paths['build']) if paths['build'].is_file() else {}
    section = paths['build'].parent / 'section3.json'
    names = cut.read(paths['scoped']).get('parts', {}) if paths['scoped'].is_file() else {}
    checks = {name: read_status(path) for name, path in paths.items()}
    checks.update({name: read_status(paths['scoped'].parent / name / 'case_summary.json') for name in names})
    checks.update({name: {'status': 'not evaluated'} for name in NOT_EVALUATED})
    if section.is_file():
        standing = cut.read(section)
        assert standing['passed'] and standing['source_commit'] == build['source_commit']
        for name, pointer in standing['gates'].items():
            assert checks[name]['receipt'] == pointer

    cl3 = HERE.parent / 'CL3'
    runs = {}
    for name, directory in [('CL3', cl3), ('CL3 repeat', cl3 / 'repeat')]:
        candidates = list((directory / 'reports').glob('CL3_*.json'))
        assert len(candidates) <= 1
        for path in candidates:
            data = cut.read(path)
            assert data['label'].startswith('CL3:') and data['terminal']['all_owned_handles_terminal']
            runs[name] = {'label': data['label'], 'finding': data['finding'],
                          'wiring': data['tolerance_wiring'], 'repeat': data.get('runtime_repeat'),
                          'json': path_pin(path), 'report': path_pin(path.with_suffix('.md'))}
    if failure:
        assert revert
        state, title = 'FAILED', f'TOL1: FAILED at {failure}'
    elif open_check:
        state, title = 'OPEN', f'TOL1: OPEN at {open_check}'
    else:
        assert section.is_file() and not revert
        if repeat := runs.get('CL3 repeat'):
            assert repeat['repeat']['status'] == 'passed'
        elif first := runs.get('CL3'):
            assert not first['wiring']['wiring_defect']
        state, title = ('accepted' if runs else 'standing checks passed'), LABEL

    return {'label': title, 'status': state, 'build': label, 'source_commit': build.get('source_commit'),
            'binary_sha256': build.get('binary_sha256'), 'source_check': path_pin(SOURCE_CHECK),
            'reference': path_pin(prior.FULL_REF), 'touched_files': build.get('changed', []), 'checks': checks,
            'section3': path_pin(section) if section.is_file() else None, 'capabilities': runs,
            'reverted_candidate': build.get('source_commit') if revert else None,
            'revert': git('rev-parse', revert).decode().strip() if revert else None,
            'diagnoses': [path_pin(path) for path in diagnoses]}


def write_report(data):
    file = HERE / 'reports' / f'TOL1_{data["build"]}.md'

    def link(pointer, text=None):
        path = Path(pointer['path'])
        return f'[{text or path.name}]({os.path.relpath(path, file.parent)})'

    lines = [f'# TOL1 build {data["build"]}', '', f'**{data["label"]}**', '',
             f'Disposition **{data["status"]}**.', '',
             f'{link(data["source_check"], "R4 source check")} · {link(data["reference"], "FP3-E S reference")}', '']
    if data['source_commit']:
        lines.extend([f'Source {data["source_commit"]}. Solver SHA256 {data["binary_sha256"]}.', ''])
    if data['revert']:
        lines.extend([f'Restored the previous production behavior in {data["revert"]}.', ''])
    lines.extend(['## Standing and equal-list checks', '', '| Check | Status | Receipt |', '| --- | --- | --- |'])
    for name, row in data['checks'].items():
        receipt = link(row['receipt']) if row.get('receipt') else '—'
        lines.append(f'| {name} | {row["status"]} | {receipt} |')
    for name, row in data['checks'].items():
        if row.get('first_difference') is not None:
            lines.extend(['', f'First difference in {name}.', '', '    ' + json.dumps(row['first_difference'])])
    for part in PARTS:
        row = data['checks'][part]
        if not row.get('legacy_configs'):
            continue
        lines.extend(['', f'### {part.capitalize()} configurations', '', '| Configuration | Source SHA256 | Launch SHA256 |',
                      '| --- | --- | --- |'])
        for name, config in row['legacy_configs'].items():
            lines.append(f'| {name} | {config["sealed_config"]["sha256"]} | {config["derived_config"]["sha256"]} |')
    lines.extend(['', '## Capability', ''])
    if not data['capabilities']:
        lines.append('CL3 was unevaluated.')
    for row in data['capabilities'].values():
        lines.extend([f'- {row["label"]}. {link(row["report"])}', ''])
    lines.extend(['', '## Touched source and fixture files', '', *[f'- {path}' for path in data['touched_files']]])
    if data['diagnoses']:
        lines.extend(['', '## Diagnoses', '', *[f'- {link(pointer)}' for pointer in data['diagnoses']]])
    file.write_text('\n'.join(lines).rstrip() + '\n')
    workflow.sealed_write(file.with_suffix('.json'), data)
    return file


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--seal', action='store_true')
    group.add_argument('--failure', metavar='GATE')
    group.add_argument('--open', metavar='CHECK')
    parser.add_argument('--revert')
    parser.add_argument('--diagnosis', type=lambda value: Path(value).resolve(), action='append', default=[])
    args = parser.parse_args()
    if args.seal:
        data = seal(args.build)
    else:
        data = report(args.build, args.failure, args.open, args.revert, args.diagnosis)
        write_report(data)
    print(data['label'], flush=True)
