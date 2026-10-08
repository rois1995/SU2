"""One matched RAE2822 native adaptation and resumed-flow cycle, sequentially."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def check_original_reference(seed):
    from airfoil_reference_audit import original_wall
    from audit_native_bl import mesh
    from audit_native_composite_rae import check_math
    def wall_edges(points, edges):
        return {tuple(sorted(tuple(map(float, points[i])) for i in edge)) for edge in edges}
    points, edges, paths = original_wall(seed / 'input.su2')
    original, _, markers = mesh(seed / 'input.su2')
    assert len(paths) == 1
    assert wall_edges(points, edges) == wall_edges(original, markers['AIRFOIL'])
    retained, retained_edges, paths = original_wall(seed / 'mesh_adap_00002.su2')
    assert len(paths) == 2
    assert wall_edges(retained, retained_edges) == wall_edges(points, edges)
    check_math()


def main():
    repo = Path(__file__).resolve().parent.parent
    physical = Path('/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt')
    archives = physical / 'integration_evidence/metric_robustness'
    root = archives / 'adaptation_cycle_pair_v1'
    seed = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v21')
    check_original_reference(seed)
    if '--self-check' in sys.argv:
        print('Original reference and geometric composition checks PASS')
        return
    versions = {}
    receipts = {}
    for label, receipt_name, archive in (
        ('baseline', 'qr_symmetry_v1_validation.json', 'qr_symmetry_v1'),
        ('improved', 'recovery_geometry_wall_bias_v1_validation.json', 'recovery_geometry_wall_bias_v1'),
    ):
        receipt = repo / 'integration_evidence' / receipt_name
        evidence = json.loads(receipt.read_text())
        receipts[label] = evidence
        binary = archives / archive / 'bin/SU2_CFD'
        expected = evidence['binary_sha256']
        if isinstance(expected, dict):
            expected = expected['SU2_CFD']
        assert sha(binary) == expected, binary
        versions[label] = dict(source_commit=evidence['source_commit'], binary=str(binary),
                               binary_sha256=expected, receipt_sha256=sha(receipt))
    # Pin identical remesher/BL/transfer code, independently of the two Git heads.
    shared = {}
    for name in subprocess.check_output(['git', 'ls-files', 'Common/include/adaptation',
                                        'Common/src/adaptation', 'SU2_CFD/src/adaptation',
                                        'SU2_CFD/include/adaptation', 'SU2_CFD/src/drivers'], cwd=repo, text=True).splitlines():
        if name == 'SU2_CFD/src/adaptation/CAdapSensors.cpp':
            continue
        baseline_hash = receipts['baseline']['production_source_sha256'].get(name)
        if baseline_hash is None:
            baseline_hash = hashlib.sha256(subprocess.check_output(
                ['git', 'show', versions['baseline']['source_commit'] + ':' + name], cwd=repo)).hexdigest()
        b = archives / 'recovery_geometry_wall_bias_v1/source' / name
        assert baseline_hash == sha(b) == receipts['improved']['production_source_sha256'][name], name
        shared[name] = baseline_hash
    cfg = (seed / 'run.cfg').read_text()
    settings = dict(RESTART_SOL='YES', SOLUTION_FILENAME='seed', ITER='1',
                    NUM_METHOD_HESS='GREEN_GAUSS', ADAP_SIZES='( 12000 )',
                    ADAP_FLOW_ITER='( 2000 )',
                    VOLUME_OUTPUT='( COORDINATES, SOLUTION, PRIMITIVE, ADAP_SENSOR, HESSIAN, METRIC )')
    for key, value in settings.items():
        cfg, count = re.subn(r'^' + key + r'\s*=.*$', key + '= ' + value, cfg, flags=re.M)
        assert count == 1, key
    cfg += '\nADAP_HESSIAN_NOISE= 0\n'
    record = dict(versions=versions, shared_remesher_bl_transfer_driver_sha256=shared,
                  seed=str(seed), config=cfg, ranks=4, runs=[],
                  scope='Single matched GG recovery cycle; common integrated remesher base 2abbd117. Not the newer native-unsteady-performance head, and no grid-convergence certificate.')
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1')
    def save():
        (root / 'campaign.json').write_text(json.dumps(record, indent=2) + '\n')
    root.mkdir()  # Preserve prior campaigns: never overwrite.
    shutil.copy2(__file__, root / 'runner_source.py')
    save()
    for label, version in versions.items():
        wd = root / label
        wd.mkdir()
        for source, name in ((seed / 'mesh_adap_00002.su2', 'input.su2'),
                             (seed / 'mesh_adap_00002.su2.native_ref', 'input.su2.native_ref'),
                             (seed / 'solution_adap_00002.dat', 'seed.dat')):
            shutil.copy2(source, wd / name)
        (wd / 'run.cfg').write_text(cfg)
        inputs = {name: sha(wd / name) for name in ('input.su2', 'input.su2.native_ref', 'seed.dat', 'run.cfg')}
        if record['runs']:
            assert inputs == record['runs'][0]['inputs_sha256']
        while True:
            busy = [line for line in subprocess.check_output(['ps', '-eo', 'stat,comm'], text=True).splitlines()
                    if 'Z' not in line.split()[0] and (line.split()[-1].startswith('SU2_CFD')
                        or line.split()[-1] in ('ninja', 'cc1plus', 'test_driver', 'test_driver_AD'))]
            record.update(phase='waiting', case=label, busy=busy)
            save()
            if not busy:
                break
            time.sleep(5)
        command = ['nice', '-n', '10', 'timeout', '--kill-after=10s', '1200s',
                   'mpirun', '--host', 'localhost:4', '-np', '4', version['binary'], 'run.cfg']
        row = dict(case=label, command=command, inputs_sha256=inputs, phase='running')
        record['runs'].append(row)
        record['phase'] = 'running'
        save()
        start = time.monotonic()
        with (wd / 'solver.log').open('x') as log:
            result = subprocess.run(command, cwd=wd, env=env, stdout=log, stderr=subprocess.STDOUT)
        row.update(phase='terminal', solver_exit=result.returncode, elapsed_seconds=time.monotonic()-start)
        assert inputs == {name: sha(wd / name) for name in inputs}
        runtime = dict(row, binary=version['binary'], binary_sha256=version['binary_sha256'],
                       source_revision=version['source_commit'], ranks=4, working_directory=str(wd))
        (wd / 'run_evidence.json').write_text(json.dumps(runtime, indent=2) + '\n')
        save()
        print(label, result.returncode, row['elapsed_seconds'], flush=True)
    record['phase'] = 'terminal'
    save()


if __name__ == '__main__':
    main()
