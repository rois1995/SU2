"""Execute DEF1's hidden test cohorts and isolated fatal options on a frozen build."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET


HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
PREFLIGHT = NIGHT / 'N4/section6_3_after_N11/end_B_preparation/primary_control_archive/end_b_actual_preflight.py'
DIAGNOSTICS = {
    'empty_middle': 'ADAP_HAUSD: empty field in marker list.',
    'empty_first': 'ADAP_HAUSD: empty field in marker list.',
    'empty_last': 'ADAP_HAUSD: empty field in marker list.',
    'odd': 'ADAP_HAUSD: marker list must contain name/value pairs.',
    'junk': 'ADAP_HAUSD: invalid number for marker WALL.',
    'zero': 'ADAP_HAUSD: value for marker WALL must be finite and > 0.',
    'negative': 'ADAP_HAUSD: value for marker WALL must be finite and > 0.',
    'nonfinite': 'ADAP_HAUSD: value for marker WALL must be finite and > 0.',
    'duplicate': 'ADAP_HAUSD: duplicate marker WALL.',
    'native': ('ADAP_HAUSD per marker is available only with ADAP_REMESHER= MMG; '
               'the native remesher uses one value.'),
    'absent': 'ADAP_HAUSD marker MISSING is absent from the gathered mesh.',
    'absent_override': 'ADAP_HAUSD marker MISSING is absent from the gathered mesh.',
}


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def execute(mode, binary, output):
    binary, output = binary.resolve(strict=True), output.resolve()
    digest = sha(binary)
    output.mkdir(parents=True, exist_ok=False)
    settings = {name: '1' for name in ('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS')}
    settings.update(SU2_NATIVE_FP3_PROBE='0', SU2_NATIVE_GEOMETRY_WORK_TRACE='0')
    environment = {key: value for key, value in os.environ.items() if not key.startswith('SU2_NATIVE_')}
    environment.update(settings)

    def invoke(case, selector, reporter, fatal=None):
        path = output / case
        path.mkdir()
        env = environment.copy()
        if fatal:
            env['SU2_DEF1_FATAL_CASE'] = fatal
        preflight = path / 'host_preflight.json'
        subprocess.run([sys.executable, '-B', str(PREFLIGHT), str(preflight)], check=True)
        command = ['nice', '-n', '19', str(binary), selector, '-r', reporter]
        stdout, stderr = path / 'stdout', path / 'stderr'
        with stdout.open('wb') as out, stderr.open('wb') as err:
            completed = subprocess.run(command, cwd=path, env=env, stdout=out, stderr=err, timeout=5400)
        receipt = {'case': case, 'argv': command, 'returncode': completed.returncode,
                   'binary_sha256': digest, 'settings': {**settings, **({'SU2_DEF1_FATAL_CASE': fatal} if fatal else {})},
                   'stdout': sha(stdout), 'stderr': sha(stderr), 'host_preflight': sha(preflight)}
        return path, receipt

    selector, count = ('[def1_normal]', 4) if mode == 'normal' else ('[def1_ad]', 1)
    path, ordinary = invoke(mode, selector, 'xml')
    try:
        tree = ET.parse(path / 'stdout')
        tests = tree.findall('.//TestCase')
        names = [test.get('name') for test in tests]
    except ET.ParseError:
        names = []
    ordinary.update(passed=ordinary['returncode'] == 0 and len(names) == count, selected_tests=names,
                    expected_test_count=count)
    rows = [ordinary]

    if mode == 'normal':
        for name, diagnostic in DIAGNOSTICS.items():
            path, row = invoke(name, '[def1_fatal]', 'console', fatal=name)
            text = (path / 'stdout').read_text(errors='replace') + (path / 'stderr').read_text(errors='replace')
            row.update(passed=row['returncode'] != 0 and diagnostic in text, expected_diagnostic=diagnostic)
            rows.append(row)

    assert sha(binary) == digest
    passed = all(row['passed'] for row in rows)
    receipt = {'passed': passed, 'mode': mode, 'binary': str(binary), 'binary_sha256': digest,
               'runner_sha256': sha(Path(__file__).resolve()), 'cases': rows}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'DEF1 {mode} hidden fixtures: {sum(row["passed"] for row in rows)}/{len(rows)} suites passed.', flush=True)
    return 0 if passed else 1


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('mode', choices=('normal', 'ad'))
    cli.add_argument('binary', type=Path)
    cli.add_argument('output', type=Path)
    args = cli.parse_args()
    raise SystemExit(execute(args.mode, args.binary, args.output))
