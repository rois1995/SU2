#!/usr/bin/env python3
"""Run one accepted-base G1 cohort; preserve Catch XML even when assertions fail."""
import argparse
import hashlib
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
BUILD = HERE / 'reference_build528'
HOUSE = STAGED / 'run_cases.py'
RUNTIME = STAGED / 'runtime_20261113_474/loaded_runtime_pins.json'


def sha(path):
    """Hash a frozen reference input."""
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def transformed_source():
    """Change only the helper timeout and output format; keep test filters and checks."""
    source = HOUSE.read_text()
    replacements = {
        'timeout_seconds = 2400 if case == "m6_stage" else 600': 'timeout_seconds = 5400',
        'argv += [str(BINARY), filter_expr, "--durations", "yes"]':
            'argv += [str(BINARY), filter_expr, "--durations", "yes", "--reporter", "xml"]',
        '["mpirun", "--bind-to", "none", "-n", str(ranks)]':
            '["mpirun", "--bind-to", "none", "-n", str(ranks), "--output-filename", str(attempt / "rank_output")]',
    }
    for before, after in replacements.items():
        assert source.count(before) == 1, before
        source = source.replace(before, after, 1)
    compile(source, str(HOUSE), 'exec')
    return source


def expose(name, value):
    """Reuse a reference alias only when every recorded field is unchanged."""
    path = HERE / name
    payload = json.dumps(value, indent=2) + '\n'
    if path.exists():
        assert path.read_text() == payload, path
    else:
        path.write_text(payload)


def main():
    """Run one cohort; a Catch assertion exit is reference evidence, not acceptance."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cohort', choices=('serial', 'mpi2', 'mpi4'))
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    source = transformed_source()
    if args.self_check:
        assert 'timeout_seconds = 5400' in source and '"--reporter", "xml"' in source
        print(json.dumps({'passed': True, 'launched': False}))
        return
    status = json.loads((BUILD / 'build_status.json').read_text())
    assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
    assert sha(status['test_binary']) == status['test_binary_sha256']
    pins = json.loads((BUILD / 'source_pins.json').read_text())
    assert all(sha(path) == digest for path, digest in pins.items())
    for path in (Path(__file__).resolve(), HERE / 'G1_actual_selection.json'):
        pins[str(path)] = sha(path)
    expose('build528_status.json', dict(status, completed_build=True))
    expose('build528_source_pins.json', pins)
    selected = json.loads((HERE / 'G1_actual_selection.json').read_text())
    case = 'N10_G1_reference_' + args.cohort
    assert not list((HERE / 'cases').glob(case + '_*')), 'refusing an unnoticed repeat'
    ranks = 1 if args.cohort == 'serial' else int(args.cohort[-1])
    for key in list(os.environ):
        if key.startswith('SU2_NATIVE_'):
            del os.environ[key]
    os.environ.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
                      OMPI_MCA_osc='pt2pt', TMPDIR=str(STAGED.parents[3] /
                      'integration_evidence/build-integrated-v2/compile-tmp'))
    namespace = {'__name__': 'N10_G1_house', '__file__': str(HOUSE)}
    exec(compile(source, str(HOUSE), 'exec'), namespace)
    namespace.update(ROOT=HERE, BINARY=Path(status['test_binary']))
    namespace['CASES'][case] = (ranks, selected['serial_filter' if ranks == 1 else 'mpi_filter'])
    sys.argv = [str(HOUSE), 'build528', case]
    namespace['main'](RUNTIME)


if __name__ == '__main__':
    main()
