"""Run the bounded serial geometry-merit focused tags against verified build473.

Preparation only in this turn. Invoke explicitly after build473 reaches its pinned terminal receipt.
"""
import hashlib
import json
import os
import sys
import tempfile
import time
from pathlib import Path

R = Path(__file__).resolve().parent
sys.path.insert(0, str(R))
import run_cases as checks

BUILD = 'build473'
CASE = 'geometry_merit473_serial'
TAGS = (
    'NativeGeometryBackend3D',
    'NativeGuidanceReuse3D',
    'NativeSurfaceMemo3D',
    'NativeFirstLayer3D',
    'NativeWallReconstruction3D',
    'NativeJointGeometry3D',
    'NativeHeightOptimization3D',
    'NativeCurvedFirstChild3D',
    'NativeBLDirectionInvalidation3D',
    'NativeFirstRowSurfaceFeedback3D',
    'NativeCurvedTransaction3D',
)
FILTER = ','.join(f'[{tag}]' for tag in TAGS)
STATUS = R / 'build473_status.json'
HANDLES = R / 'build473_observed_handles.json'
SOURCE_PINS = R / 'build473_source_pins.json'
RUNTIME_PINS = R / 'runtime_20261106_v2/loaded_runtime_pins.json'
RESULT = R / 'geometry_merit473_serial_assessment.json'


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def proc_identity(pid):
    path = Path('/proc') / str(pid) / 'stat'
    try:
        fields = path.read_text().rsplit(')', 1)[1].split()
        return {'present': True, 'start_ticks': int(fields[19])}
    except (FileNotFoundError, ProcessLookupError):
        return {'present': False, 'start_ticks': None}


def main():
    assert not RESULT.exists(), 'Preserve prior validation evidence; choose a new result name to rerun.'
    build_status = json.loads(STATUS.read_text())
    assert build_status.get('state') == 'terminal' and build_status.get('exit_code') == 0
    assert build_status.get('pins_verified') and build_status.get('completed_build')
    handle_receipt = json.loads(HANDLES.read_text())
    assert handle_receipt.get('actual_host_proc')
    previous_identities = list(handle_receipt.get('identities', []))
    if handle_receipt.get('wrapper_excluded'):
        previous_identities.append(handle_receipt['wrapper_excluded'])
    for identity in previous_identities:
        current = proc_identity(identity['pid'])
        assert not (current['present'] and current['start_ticks'] == identity['start_ticks']), identity

    available = int(next(line.split()[1] for line in Path('/proc/meminfo').read_text().splitlines()
                         if line.startswith('MemAvailable:'))) * 1024
    assert available > (8 << 30), f'Insufficient available RAM for focused validation: {available}'
    for path in (SOURCE_PINS, RUNTIME_PINS):
        assert path.is_file()
    # run_cases independently verifies build, complete source and runtime pins before execution.
    checks.CASES[CASE] = (1, FILTER)
    prior = set((R / 'cases').glob(CASE + '_*'))
    scratch = R.parents[3] / 'integration_evidence/build-integrated-v2/compile-tmp'
    assert scratch.is_dir()
    os.environ.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
                      OMPI_MCA_osc='pt2pt',
                      OMPI_MCA_orte_tmpdir_base=tempfile.mkdtemp(prefix=CASE + '-', dir=scratch))

    seen = {}
    original_sample = checks.sample_tree

    def sample(pid):
        rows = original_sample(pid)
        for row in rows:
            try:
                fields = Path('/proc', str(row['pid']), 'stat').read_text().rsplit(')', 1)[1].split()
                identity = {'pid': row['pid'], 'start_ticks': int(fields[19])}
                seen[(identity['pid'], identity['start_ticks'])] = identity
            except (FileNotFoundError, ProcessLookupError, PermissionError):
                pass
        return rows

    checks.sample_tree = sample
    wrapper = {'pid': os.getpid(),
               'start_ticks': int(Path('/proc/self/stat').read_text().rsplit(')', 1)[1].split()[19])}
    started = time.time()
    sys.argv = ['run_cases.py', BUILD, CASE]
    exit_code = 1
    try:
        checks.main()
    except SystemExit as exc:
        exit_code = exc.code or 0

    attempts = sorted((set((R / 'cases').glob(CASE + '_*')) - prior), key=lambda p: p.stat().st_mtime)
    assert len(attempts) == 1, attempts
    attempt = attempts[0]
    live = []
    for identity in seen.values():
        current = proc_identity(identity['pid'])
        if current['present'] and current['start_ticks'] == identity['start_ticks']:
            live.append(identity)
    terminal = {'actual_host_proc': True, 'all_owned_handles_terminal': not live,
                'sampled_identities': list(seen.values()), 'live_owned_handles': live,
                'wrapper_excluded': wrapper, 'checked_unix': time.time()}
    (attempt / 'terminal_handles_actual.json').write_text(json.dumps(terminal, indent=2) + '\n')
    status = json.loads((attempt / 'status.json').read_text())
    result = {
        'schema': 'geometry_merit473_serial_check_v1',
        'case': str(attempt), 'build': BUILD, 'exit_code': exit_code,
        'status': status, 'terminal_handles': terminal,
        'focused_tags': list(TAGS), 'filter': FILTER, 'ranks': 1,
        'timeout_seconds': 600, 'available_memory_at_launch_bytes': available,
        'build473_status_sha256': sha(STATUS),
        'build473_observed_handles_sha256': sha(HANDLES),
        'build473_source_pins_sha256': sha(SOURCE_PINS),
        'runtime_pins_sha256': sha(RUNTIME_PINS),
        'check_script_sha256': sha(Path(__file__).resolve()),
        'scope': 'Focused serial regressions only; no complete M6 capability, publication, mesh acceptance, or production qualification is claimed.'
    }
    RESULT.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'case': str(attempt), 'exit_code': exit_code,
                      'pins_verified': status.get('pins_verified'),
                      'all_owned_handles_terminal': terminal['all_owned_handles_terminal']}), flush=True)
    assert terminal['all_owned_handles_terminal'], live
    raise SystemExit(exit_code or int(status.get('exit_code') != 0 or not status.get('pins_verified')))


if __name__ == '__main__':
    main()
