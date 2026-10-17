#!/usr/bin/env python3
"""Exact saved-edge/original-donor replay. No CFD, normalization or mesh acceptance claim."""
import json, os, shlex, subprocess, sys, time
from pathlib import Path
import numpy as np
from run import HERE, ROOT, OLD, BUILD, BINARY, pins, sha


def main():
    label = sys.argv[1] if len(sys.argv) > 1 else 'first_layer_edge_v1'
    assert '/' not in label and label not in ('.', '..')
    folder = HERE / label
    folder.mkdir(exist_ok=True)
    original = HERE / 'construction_first_trial_v1'
    source = original / 'sensor_original.bin'
    mesh_path = original / 'Coarse.su2'
    fixture = OLD / 'cell329/replay.dat'
    saved = json.loads((original / 'provenance.json').read_text())['identity']
    for path in (source, mesh_path):
        assert sha(path) == saved[str(path.relative_to(ROOT))]
    sys.path.insert(0, str(ROOT / 'TestCases/adaptation/capability'))
    from capcheck import read_su2
    mesh = read_su2(mesh_path)
    dtype = np.dtype([('id', '<u8'), ('x', '<f8', (3,)), ('m', '<f8', (9,))])
    with source.open('rb') as f:
        count = int(np.fromfile(f, dtype='<u8', count=1)[0])
        records = np.fromfile(f, dtype=dtype)
    assert len(records) == count == len(mesh.P)
    assert np.array_equal(np.sort(records['id']), np.arange(count))
    assert np.array_equal(records['x'], mesh.P[records['id']])
    # Closed AABB intersection cannot omit a donor intersecting the closed segment.
    # Exact tetrahedron/segment filtering and trace coverage are checked in the native replay.
    a = [.619448337998914411173, 1.02515983674473298848, -.0153284912029800698963]
    b = [.619865041411222006396, 1.02046045941819452096, -.015806993408904391335]
    x = mesh.P[mesh.E]
    selected = np.flatnonzero(np.all(np.min(x, axis=1) <= np.maximum(a, b), axis=1) &
                              np.all(np.max(x, axis=1) >= np.minimum(a, b), axis=1))
    assert 0 < len(selected) <= 256, len(selected)
    focus = .544677734375523202087
    samples = np.unique(np.r_[np.linspace(0, 1, 2049),
                            [focus + sign * 2. ** -k for k in range(2, 49) for sign in (-1, 1)]])
    samples = np.unique(np.r_[samples, [2. ** -k for k in range(4, 61)],
                              [1. - 2. ** -k for k in range(4, 54)]])
    cut_file = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None
    reference_cuts = np.loadtxt(cut_file) if cut_file else np.array([0., .5, 1.])
    assert reference_cuts[0] == 0 and reference_cuts[-1] == 1 and np.all(np.diff(reference_cuts) > 0)
    gauss = []
    for order in (16, 32, 64):
        nodes, weights = np.polynomial.legendre.leggauss(order)
        gauss.append((order, np.concatenate([left + .5 * (right - left) * (1 + nodes)
                      for left, right in zip(reference_cuts[:-1], reference_cuts[1:])]),
                      np.concatenate([weights * .5 * (right - left)
                      for left, right in zip(reference_cuts[:-1], reference_cuts[1:])])))
    samples = np.unique(np.concatenate([samples] + [g[1] for g in gauss]))
    text = ['NATIVE_BL_EDGE_3D', ' '.join(format(t, '.17g') for t in a + b), str(len(selected))]
    text += [' '.join(map(str, [int(i)] + list(mesh.E[i]))) for i in selected]
    text += [str(len(samples))] + [format(t, '.17g') for t in samples]
    packet = folder / 'edge.dat'
    payload = '\n'.join(text) + '\n'
    if packet.exists():
        assert packet.read_text() == payload
    else:
        packet.write_text(payload)
    sources = ['Common/src/adaptation/CNativeField3D.cpp', 'Common/include/adaptation/CNativeField3D.hpp',
               'Common/src/adaptation/CNativePredicates3D.cpp', 'Common/include/adaptation/CNativeMesh3D.hpp',
               'Common/src/adaptation/CNativeRemesher3D.cpp', 'SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
               'TestCases/adaptation/capability/capcheck.py']
    identity = dict(pins(), **{p: sha(ROOT / p) for p in sources})
    identity.update({str(p.relative_to(ROOT)): sha(p) for p in (Path(__file__).resolve(), source, mesh_path, fixture, packet)})
    if cut_file:
        identity[str(cut_file.relative_to(ROOT))] = sha(cut_file)
    proof = folder / 'provenance.json'
    if proof.exists():
        assert json.loads(proof.read_text()) == identity, 'Changed provenance; preserve existing evidence.'
    else:
        proof.write_text(json.dumps(identity, indent=2) + '\n')
    result = folder / 'result.json'
    if result.exists():
        report = json.loads(result.read_text())
        assert report['provenance_sha256'] == sha(proof)
        assert report['outputs_sha256'] == {p: sha(folder / p) for p in report['outputs_sha256']}
        print('REUSE', json.dumps(report), flush=True)
        return
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
               TMPDIR=str(BUILD / 'compile-tmp'), SU2_NATIVE_BL_REPLAY=str(fixture),
               SU2_NATIVE_BL_EDGE=str(packet), SU2_NATIVE_BL_EDGE_SENSOR=str(source))
    command = ['taskset', '-c', '0', 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', 'resources.txt',
               'timeout', '--kill-after=5s', '180s', str(BINARY), '[NativeFacetedVolumeReplay3D]',
               '--abort', '--reporter', 'compact']
    start = time.monotonic()
    with (folder / 'run.log').open('w') as log:
        process = subprocess.run(command, cwd=folder, env=env, stdout=log, stderr=subprocess.STDOUT)
    outputs = ['run.log', 'resources.txt', 'edge_donors.csv', 'edge_cuts.txt', 'edge_samples.csv']
    report = {'exit_code': process.returncode, 'elapsed_seconds': time.monotonic() - start,
              'command': command, 'provenance_sha256': sha(proof), 'original_cells': len(mesh.E),
              'AABB_candidates': len(selected), 'CFD_steps': 0, 'mesh_constructed': False,
              'normalized_budget_accuracy_established': False,
              'outputs_sha256': {p: sha(folder / p) for p in outputs if (folder / p).exists()}}
    log_text = (folder / 'run.log').read_text()
    lines = [line for line in log_text.splitlines() if line.startswith('FROZEN_EDGE ')]
    if lines:
        report['edge'] = dict(token.split('=', 1) for token in shlex.split(lines[-1])[1:])
    if (folder / 'edge_samples.csv').exists():
        measured = np.genfromtxt(folder / 'edge_samples.csv', delimiter=',', names=True)
        report['fixed_gauss_references'] = []
        for order, nodes, weights in gauss:
            indices = np.searchsorted(measured['t'], nodes)
            assert np.array_equal(measured['t'][indices], nodes)
            value = float(np.sum(weights * measured['norm'][indices], dtype=np.longdouble))
            report['fixed_gauss_references'].append({'order_per_band': order, 'value': value})
        report['reference_is_proven_bound'] = False
        report['reference_scope'] = 'Fixed Gauss on declared bands; same original edge trace/native point law, independent of adaptive edge controller, not independent geometry or a proven bound.'
        report['reference_cuts'] = list(map(float, reference_cuts))
    result.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report), flush=True)
    assert process.returncode in (0, 1), 'Replay did not terminate normally; inspect saved failure.'
    assert 'FROZEN_EDGE ' in (folder / 'run.log').read_text(), 'No complete edge diagnostic.'


if __name__ == '__main__':
    main()
