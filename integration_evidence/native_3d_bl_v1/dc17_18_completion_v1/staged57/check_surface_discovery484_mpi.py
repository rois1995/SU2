"""Qualify affected census/publication/empty-rank and constraint checks on MPI6.

Invoke only after build484 reaches its pinned terminal receipt. Same bounded focused tags; actual M6 follows separately.
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

BUILD = 'build484'
CASE = 'surface_discovery484_mpi6'
TAGS = ('NativeSurfacePreparationEngine3D','NativeCollapseCorrection3D','NativeFirstRowSurfaceFeedback3D','NativeBLDirectionInvalidation3D','NativeDeadline3D','NativeAtlasWorkStop3D')
FILTER = ','.join(f'[{tag}]' for tag in TAGS)
STATUS = R / 'build484_status.json'
HANDLES = R / 'build484_observed_handles.json'
SOURCE_PINS = R / 'build484_source_pins.json'
RUNTIME_PINS = R / 'runtime_20261113_474/loaded_runtime_pins.json'
RESULT = R / 'surface_discovery484_mpi6_assessment.json'


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
    checks.CASES[CASE] = (6, FILTER)
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
    original_save=checks.save
    def save_pinned(path,value):
        if Path(path).name=='pins.json':
            for extra in (Path(__file__).resolve(),R/'surface_discovery484_fidelity.json',R.parents[3]/'Handbook/ADAPTATION_SURFACE_DISCOVERY_484_20261113.md',R.parents[3]/'Handbook/ADAPTATION_GEOMETRY_MAGNITUDE_DECISION_20261113.md'):
                value[str(extra)]=sha(extra)
        original_save(path,value)
    checks.save=save_pinned
    try:
        checks.main(RUNTIME_PINS)
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
    (attempt / 'terminal_handles.json').write_text(json.dumps(terminal, indent=2) + '\n')
    status = json.loads((attempt / 'status.json').read_text())
    result = {
        'schema': 'surface_discovery484_mpi6_check_v1',
        'case': str(attempt), 'build': BUILD, 'exit_code': exit_code,
        'status': status, 'terminal_handles': terminal,
        'focused_tags': list(TAGS), 'filter': FILTER, 'ranks': 6,
        'timeout_seconds': 600, 'available_memory_at_launch_bytes': available,
        'build484_status_sha256': sha(STATUS),
        'build484_observed_handles_sha256': sha(HANDLES),
        'build484_source_pins_sha256': sha(SOURCE_PINS),
        'runtime_pins_sha256': sha(RUNTIME_PINS),
        'check_script_sha256': sha(Path(__file__).resolve()),
        'scope': 'Focused MPI6 ownership/publication and constraint checks; not actual M6 readiness or final mesh acceptance.'
    }
    RESULT.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'case': str(attempt), 'exit_code': exit_code,
                      'pins_verified': status.get('pins_verified'),
                      'all_owned_handles_terminal': terminal['all_owned_handles_terminal']}), flush=True)
    assert terminal['all_owned_handles_terminal'], live
    raise SystemExit(exit_code or int(status.get('exit_code') != 0 or not status.get('pins_verified')))


if __name__ == '__main__':
    main()
