"""Run hidden TOL1 and DEF1 fixtures on the sealed normal test driver."""

from argparse import ArgumentParser
import os
from pathlib import Path
import runpy
import signal
import subprocess
import sys
import xml.etree.ElementTree as ET

from build_fp3 import DEF1, HERE, cut, from_base, verify_build

LEGACY = DEF1 / 'run_lab.py'
DEFS = runpy.run_path(str(from_base(LEGACY)))
PREFLIGHT = DEFS['PREFLIGHT']
LEGACY_FATAL = {
    **DEFS['DIAGNOSTICS'],
    'native': 'ADAP_HAUSD: NOT_DECLARED is not a boundary marker',
    'native_above': 'ADAP_HAUSD: x_minus value is above 1e100.',
    'native_bl': 'ADAP_HAUSD per marker is not available with the native boundary layer yet.',
}
NATIVE_FATAL = {
    'missing': ('ADAP_HAUSD: with ADAP_REMESHER= NATIVE_CAVITY every boundary marker needs a value; missing:', 1),
    'unmapped': ('ADAP_HAUSD per marker needs a mapped curved native reference.', 1),
    'unit_floor': ('ADAP_HAUSD: the smallest value is below the native floor', 1),
    'tiny_floor': ('ADAP_HAUSD: the smallest value is below the native floor', 1),
    'native2d': ('ADAP_HAUSD per marker is available for the 3D native remesher only.', 1),
    'mpi_value': ('ADAP_HAUSD differs between ranks.', 2),
    'mpi_form': ('ADAP_HAUSD differs between ranks.', 2),
}


def execute(label):
    build = verify_build(label)
    binary = Path(build['test_binary'])
    root = HERE / f'build{label}'
    output = root / 'fixtures' / str(os.getpid())
    output.mkdir(parents=True)
    pin_paths = [Path(__file__).resolve(), LEGACY, PREFLIGHT, root / 'build_status.json']
    settings = {key: '1' for key in ('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS')}
    settings.update(OMPI_MCA_osc='pt2pt', TMPDIR=str(HERE.parents[4] / 'build-integrated-v2/compile-tmp'))
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith(('SU2_NATIVE_', 'SU2_DEF1_FATAL_', 'SU2_TOL1_FATAL_'))}

    def invoke(name, selector, reporter, diagnostic=None, flag=None, ranks=1):
        case = output / name
        case.mkdir()
        preflight = case / 'host_preflight.json'
        subprocess.run([sys.executable, '-B', str(PREFLIGHT), str(preflight)], check=True)
        options = {**settings, **({flag: name} if flag else {})}
        mpi = ['mpirun', '-np', str(ranks)] if ranks > 1 else []
        argv = ['nice', '-n', '19', *mpi, str(binary), selector, '-r', reporter]
        stdout, stderr = case / 'stdout', case / 'stderr'
        files = [stdout, stderr, preflight]
        with stdout.open('wb') as out, stderr.open('wb') as err:
            process = subprocess.Popen(argv, cwd=case, env={**environment, **options},
                                       stdout=out, stderr=err, start_new_session=True)
            timed_out = False
            try:
                process.wait(timeout=90 if ranks > 1 else 5400)
            except subprocess.TimeoutExpired:
                timed_out = True
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        record = dict(name=name, argv=argv, settings=options, returncode=process.returncode,
                      stdout=str(stdout), stderr=str(stderr), host_preflight=str(preflight), timed_out=timed_out)
        if diagnostic:
            log = stdout.read_text(errors='replace') + stderr.read_text(errors='replace')
            record.update(expected_diagnostic=diagnostic,
                          passed=not timed_out and process.returncode != 0 and diagnostic in log)
        else:
            try:
                tree = ET.parse(stdout)
                names = [test.get('name') for test in tree.findall('.//TestCase')]
            except ET.ParseError:
                names = []
            expected = {'TOL1': 9, 'DEF1': 4}[name]
            record.update(selected_tests=names, expected=expected,
                          passed=not timed_out and process.returncode == 0 and len(names) == expected)
        result = case / 'receipt.json'
        cut.save(result, record)
        pin_paths.extend([*files, result])
        print(f'{name}: {"passed" if record["passed"] else "FAILED"}', flush=True)
        return record

    cases = [invoke('TOL1', '[TOL1]', 'xml'), invoke('DEF1', '[def1_normal]', 'xml')]
    fatal_cases = [
        *[invoke(name, '[def1_fatal]', 'console', error, 'SU2_DEF1_FATAL_CASE')
          for name, error in LEGACY_FATAL.items()],
        *[invoke(name, '[tol1_fatal]', 'console', error, 'SU2_TOL1_FATAL_CASE', ranks)
          for name, (error, ranks) in NATIVE_FATAL.items()],
    ]
    passed = all(row['passed'] for row in cases + fatal_cases)
    assert verify_build(label) == build
    cut.save(root / 'fixtures.json', {
        **{key: build[key] for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')},
        'status': 'passed' if passed else 'failed', 'passed': passed,
        'cases': cases, 'fatal_cases': fatal_cases, 'pins': {str(path): cut.sha(path) for path in pin_paths},
    })
    return passed


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    raise SystemExit(not execute(parser.parse_args().build))
