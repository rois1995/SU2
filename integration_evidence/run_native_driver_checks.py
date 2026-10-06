"""Sequential native SU2 driver checks; archive each run's executable and source provenance."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--label', required=True)
parser.add_argument('--ranks', nargs='+', type=int, default=[1, 2, 4])
parser.add_argument('--filter', default='[NativeRemesher]')
parser.add_argument('--binary', choices=['test_driver', 'test_memory'], default='test_driver')
parser.add_argument('--build', default='build-nommg')
parser.add_argument('--save-audit', action='store_true')
parser.add_argument('--airfoil-config', type=Path)
parser.add_argument('--timeout', type=int, default=240)
args = parser.parse_args()
if Path(args.build).name != args.build:
    parser.error('Use a plain build directory name under the integration workspace.')
if any(n not in (1, 2, 4) for n in args.ranks):
    parser.error('Use 1, 2 or 4 ranks, sequentially.')
if not 30 <= args.timeout <= 3600:
    parser.error('Use a timeout between 30 and 3600 seconds per MPI job.')
root = Path(__file__).resolve().parent
source = Path(__file__).resolve().parent.parent
build = root / args.build
binary = build / 'UnitTests' / args.binary
destination = root / args.label
destination.mkdir(exist_ok=False)

def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()

files = [source / p for p in [
    'meson.build', 'meson_options.txt', 'Common/include/adaptation/CMMGInterface.hpp',
    'Common/src/adaptation/CMMGInterface.cpp',
    'QuickStart/native_NACA0012.cfg',
    'Common/include/CConfig.hpp', 'Common/src/CConfig.cpp', 'Common/include/option_structure.hpp',
    'Common/include/adaptation/CRemesher.hpp', 'Common/include/adaptation/CNativeRemesher.hpp',
    'Common/include/adaptation/CReaderSlices.hpp', 'Common/src/adaptation/CReaderSlices.cpp',
    'Common/include/adaptation/CNativeReferenceIO.hpp',
    'Common/src/adaptation/CNativeRemesher.cpp', 'Common/src/adaptation/CNativePredicates2D.cpp',
    'Common/src/adaptation/meson.build', 'Common/src/meson.build', 'config_template.cfg',
    'SU2_CFD/include/drivers/CSinglezoneDriver.hpp', 'SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
    'SU2_CFD/src/adaptation/CConservativeTransfer.cpp',
    'SU2_CFD/src/adaptation/CTransferAdmissibility.cpp',
    'SU2_CFD/include/adaptation/CTransferAdmissibility.hpp',
    'UnitTests/SU2_CFD/adaptation/CConservativeTransfer_tests.cpp',
    'UnitTests/SU2_CFD/adaptation/AdaptationMPI_tests.cpp',
    'UnitTests/Common/adaptation/CNativeReferenceIO_tests.cpp',
    'UnitTests/SU2_CFD/adaptation/TransferTestCase.hpp',
    'UnitTests/meson.build', 'UnitTests/SU2_CFD/adaptation/CNativeRemesher_tests.cpp']]
files.extend((source / 'Common/include/adaptation').glob('CNative*2D.hpp'))
files.extend((source / 'UnitTests/Common/adaptation').glob('CNative*2D_tests.cpp'))
files.extend((source / 'UnitTests/SU2_CFD/adaptation').glob('CNative*2D_tests.cpp'))
files.extend((source / 'SU2_CFD/src/output/filewriter').glob('*.cpp'))
files.extend((source / 'SU2_CFD/include/output/filewriter').glob('*.hpp'))
files.extend(source / relative for relative in (
    'SU2_CFD/src/output/CMeshOutput.cpp', 'SU2_CFD/src/output/COutput.cpp',
    'SU2_CFD/include/output/COutput.hpp', 'SU2_CFD/src/drivers/CDiscAdjSinglezoneDriver.cpp',
    'SU2_CFD/include/drivers/CDiscAdjSinglezoneDriver.hpp', 'SU2_CFD/src/adaptation/CAdjointTransfer.cpp',
    'SU2_CFD/include/adaptation/CAdjointTransfer.hpp',
    'UnitTests/Common/adaptation/CNativeScaling2D_tests.cpp'))
manifest = {'filter': args.filter, 'sequential': True, 'maximum_ranks': 4,
            'environment': {'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1'},
            'build': str(build), 'binary_sha256': sha(binary),
            'executable_name': args.binary,
            'source_sha256': {str(p.relative_to(source)): sha(p) for p in sorted(set(files))},
            'note': 'Source hashes include drafts: check the recorded UnitTests/Meson registrations for compiled scope.',
            'runner_sha256': sha(Path(__file__)),
            'runner_pid': os.getpid(),
            'source_revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip(),
            'source_status': subprocess.check_output(['git', 'status', '--short'], cwd=source, text=True),
            'runs': []}
build_options = build / 'meson-info/intro-buildoptions.json'
manifest['build_options'] = {option['name']: option['value'] for option in json.loads(build_options.read_text())
                             if option['name'] in ('enable-mmg', 'enable-cgns', 'with-mpi', 'with-omp', 'buildtype',
                                                   'enable-normal', 'enable-autodiff', 'enable-directdiff', 'b_ndebug',
                                                   'mmg_root', 'mmg_scotch_root')}
manifest['timeout_seconds_per_job'] = args.timeout
source_archive = destination / 'sources'
shutil.copy2(Path(__file__), destination / 'runner_source.py')
for relative, expected_hash in manifest['source_sha256'].items():
    archived = source_archive / relative
    archived.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / relative, archived)
    if sha(archived) != expected_hash:
        raise RuntimeError(f'Source changed during runner preparation: {relative}')
manifest['archived_sources'] = str(source_archive)
report = destination / 'evidence.json'
report.write_text(json.dumps(manifest, indent=2) + '\n')
env = dict(os.environ, **manifest['environment'])
if args.airfoil_config:
    config = args.airfoil_config.resolve(strict=True)
    manifest['environment']['SU2_NATIVE_AIRFOIL_CONFIG'] = str(config)
    env['SU2_NATIVE_AIRFOIL_CONFIG'] = str(config)
    manifest['airfoil_config_sha256'] = sha(config)
    shutil.copy2(config, destination / 'airfoil_input.cfg')
    for line in config.read_text().splitlines():
        if line.strip().startswith('MESH_FILENAME='):
            mesh = Path(line.split('=', 1)[1].split('%', 1)[0].strip()).resolve(strict=True)
            manifest['airfoil_mesh'] = {'path': str(mesh), 'sha256': sha(mesh)}
            shutil.copy2(mesh, destination / 'airfoil_input.su2')
if args.save_audit:
    manifest['environment']['SU2_NATIVE_SAVE_AUDIT'] = '1'
    env['SU2_NATIVE_SAVE_AUDIT'] = '1'
all_ok = True
for ranks in args.ranks:
    command = ['mpiexec', '-n', str(ranks), str(binary), args.filter, '--use-colour', 'no']
    log = destination / f'np{ranks}.log'
    start = time.monotonic()
    with log.open('w') as output:
        process = subprocess.Popen(command, cwd=destination, env=env, stdout=output,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        manifest['current_run'] = {'ranks': ranks, 'pid': process.pid, 'command': command,
                                   'started_unix_seconds': time.time()}
        report.write_text(json.dumps(manifest, indent=2) + '\n')
        try:
            code = process.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            code = 124
    text = log.read_text()
    if args.save_audit:
        # Later rank counts reuse the test's filenames. Keep each rank's artifacts
        # before starting the next MPI job, rather than retaining only the last run.
        audit_directory = destination / f'audit_np{ranks}'
        audit_directory.mkdir()
        for artifact in sorted(destination.glob('native_*')):
            if artifact.is_file() and artifact.suffix in ('.su2', '.cgns', '.native_ref', '.csv', '.cfg', '.dat', '.meta'):
                shutil.copy2(artifact, audit_directory / artifact.name)
    ok = code == 0 and text.count('All tests passed') == ranks
    manifest['runs'].append({'ranks': ranks, 'command': command, 'exit_code': code,
                             'elapsed_seconds': time.monotonic() - start,
                             'verified': ok, 'log': str(log)})
    manifest.pop('current_run', None)
    report.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'np{ranks}: {"PASS" if ok else "FAIL"} ({log})', flush=True)
    if not ok:
        all_ok = False
        break
# Copy only after MPI ends. No later relink may change the executable associated with this evidence.
archive = destination / args.binary
shutil.copy2(binary, archive)
manifest['archived_binary'] = str(archive)
assert sha(archive) == manifest['binary_sha256']
report.write_text(json.dumps(manifest, indent=2) + '\n')
raise SystemExit(0 if all_ok else 1)
