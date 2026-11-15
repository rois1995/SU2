"""Run each connected unit check once using the existing pinned case runner."""
import json, os, sys, tempfile, time
from pathlib import Path
HERE = Path(__file__).resolve().parent
R = HERE.parent
sys.path.insert(0, str(R))
import run_cases as checks
mode = sys.argv[1]
assert mode in ('serial', 'mpi2')
status = json.loads((HERE / 'build_status.json').read_text())
assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
checks.BINARY = Path(status['test_binary'])
assert checks.sha(checks.BINARY) == status['test_binary_sha256']
pins = json.loads((HERE / 'source_pins.json').read_text())
assert all(checks.sha(path) == digest for path, digest in pins.items())
for name, value in (('build507_status.json', dict(status, completed_build=True)), ('build507_source_pins.json', pins),
                    ('build507_terminal_handles.json', {'actual_host_proc': True, 'all_owned_handles_terminal': True, 'identities': [], 'verified_no_heavy_process_unix': time.time()})):
    path = R / name
    if path.exists():
        prior = json.loads(path.read_text())
        if 'verified_no_heavy_process_unix' not in prior: assert prior == value
    else: path.write_text(json.dumps(value, indent=2) + '\n')
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'): del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1', OMPI_MCA_osc='pt2pt')
os.environ['OMPI_MCA_orte_tmpdir_base'] = tempfile.mkdtemp(prefix='p7p8-units-', dir=checks.REPO / 'integration_evidence/build-integrated-v2/compile-tmp')
filters = '[NativeJobMemory3D]'
if mode == 'serial': filters += ',[NativeGeometryBackend3D],[NativeOwnedFaceProof3D],[NativeGeometryStats3D],[NativeTimerTree3D]'
case = 'p7p8_r3_units_' + mode
checks.CASES[case] = (1 if mode == 'serial' else 2, filters)
original_save = checks.save
def save(path, value):
    if Path(path).name == 'pins.json': value[str(Path(__file__).resolve())] = checks.sha(__file__)
    original_save(path, value)
checks.save = save
sys.argv = ['run_cases.py', 'build507', case]
checks.main(R / 'runtime_20261113_474/loaded_runtime_pins.json')
