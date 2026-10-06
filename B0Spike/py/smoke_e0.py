"""Bounded serial E0 smoke checks. No mpirun; no long experiment series."""
import json
import os
import pathlib
import struct
import subprocess
import sys
import tempfile
import time

import numpy as np
from b0io import read, full
from e0_cells import read as read_cells

ROOT = pathlib.Path(__file__).resolve().parents[2]
RUN = ROOT.parent / 'runs/b0/e0_smoke'
BIN = ROOT.parent / 'runs/b0/bin/b0spike'
ENV = dict(os.environ, B0_SERIAL_ONLY='1', OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1')


def call(*args, expected=0, timeout=45):
    while os.getloadavg()[0] >= 7:
        print('load gate 7: waiting 5 seconds', flush=True)
        time.sleep(5)
    cmd = ['timeout', '--kill-after=5s', f'{timeout}s', str(BIN)] + [str(a) for a in args]
    result = subprocess.run(cmd, cwd=RUN, env=ENV, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(' '.join(str(a) for a in args), 'exit', result.returncode, flush=True)
    (RUN / 'test.log').open('a').write(' '.join(cmd) + '\n' + result.stdout)
    if result.returncode != expected:
        raise AssertionError(result.stdout)
    return result.stdout


def main():
    RUN.mkdir(parents=True, exist_ok=True)
    (RUN / 'test.log').write_text('')
    call('selftest', 'unused')
    for kind, dim, n in [('box2d', 2, 4), ('box3s', 3, 2), ('box3c', 3, 2), ('box2r', 2, 4)]:
        file = RUN / (kind + '.b0in')
        call('gen', kind, file, n)
        data = bytearray(file.read_bytes())
        _, nv, _, _ = struct.unpack_from('4Q', data, 8)
        offset = 88 + nv * dim * 8
        tensor = [25., 0., 25.] if dim == 2 else [16., 0., 0., 16., 0., 16.]
        struct.pack_into(f'{nv * len(tensor)}d', data, offset, *(tensor * nv))
        file.write_bytes(data)
    for kind in ['box2d', 'box3s']:
        for sched in ['sbase', 'sbatch', 'sb4']:
            prefix = RUN / (kind + '_' + sched)
            call('run', kind + '.b0in', '--P', 4, '--sched', sched, '--levels', 2,
                 '--budgetC', 3, '--budgetD', 2, '--out', str(prefix) + '.json', '--artifacts', prefix)
            result = json.loads(pathlib.Path(str(prefix) + '.json').read_text())
            assert isinstance(result['hash'], str) and result['planMismatches'] == 0 and result['finalValid']
            artifact = read_cells(str(prefix) + '_cells.json')
            assert artifact['format'] == 'B0CELLS1' and artifact['hash'] == result['hash']
            assert len(artifact['candidate']['cells']) == result['final']['ne']
            assert all(len(c['edges']) == (3 if kind == 'box2d' else 6) for c in artifact['candidate']['cells'])
            assert all(s['committed'] for s in result['steps']), result['steps']
            for snapshot in sorted(RUN.glob(prefix.name + '_step*.b0state')):
                call('statecheck', snapshot, 'roundtrip.b0state')
                assert snapshot.read_bytes() == (RUN / 'roundtrip.b0state').read_bytes()
                step = int(snapshot.stem.rsplit('_step', 1)[1])
                if result['steps'][step]['pieceIds']:  # SB4 can record an empty sheet step.
                    call('replay', snapshot, 'all', 'replayed')
                    assert snapshot.with_suffix('.posthash').read_text() == (RUN / 'replayed.posthash').read_text()
            call('statecheck', str(prefix) + '_final.b0state', 'roundtrip.b0state')
            call('distcheck', str(prefix) + '_final.b0state', 50)
    call('run', 'box2d.b0in', '--P', 4, '--sched', 'level0', '--level0stats', 1,
         '--out', 'level0stats.json')
    assert json.loads((RUN / 'level0stats.json').read_text())['finalValid']
    base = ['run', 'box2d.b0in', '--P', 4, '--sched', 'sbatch', '--budgetC', 2, '--budgetD', 1]
    snapshot = next(RUN.glob('box2d_sbase_step*.b0state'))
    # Exercise both open and write/flush failures before transaction execution.
    with tempfile.TemporaryDirectory(prefix='snapshot_failure_', dir=RUN) as tmp:
        wd = pathlib.Path(tmp)
        blocker = wd / 'not_a_directory'; blocker.write_text('block snapshot directory')
        for name, prefix, reasons in [('open', blocker / 'state', ['cannot write snapshot']),
                                      ('flush', wd / 'full', ['truncated or unwritable E0 snapshot',
                                                             'snapshot vector IO failed', 'snapshot string IO failed',
                                                             'snapshot flush failed'])]:
            if name == 'flush':
                pathlib.Path(str(prefix) + '_step0.b0state').symlink_to('/dev/full')
            output = wd / (name + '.json')
            log = call(*base, '--out', output, '--artifacts', prefix, expected=1)
            assert 'rank 0: error: snapshot save failed on rank 0:' in log
            assert any(reason in log for reason in reasons), log
            assert not output.exists() and not pathlib.Path(str(prefix) + '_step0.posthash').exists()
        # Statecheck uses the same collective writer guard, including close/flush errors.
        for dest in [blocker / 'roundtrip.b0state', pathlib.Path('/dev/full')]:
            log = call('statecheck', snapshot, dest, expected=1)
            assert 'rank 0: error: snapshot save failed on rank 0:' in log, log
        prefix = wd / 'posthash'
        pathlib.Path(str(prefix) + '_step0.posthash').symlink_to('/dev/full')
        output = wd / 'posthash.json'
        log = call(*base, '--out', output, '--artifacts', prefix, expected=1)
        assert 'failed on rank 0: cannot write ' in log and '_step0.posthash' in log, log
        assert not output.exists()
        # Payload/header failures cannot publish a completed artifact header.
        for suffix, reason in [('_cells.bin.tmp', 'cannot write cell artifact payload'),
                               ('_cells.json.tmp', 'cannot write ')]:
            prefix = wd / ('cell_failure_' + suffix.split('.')[1])
            pathlib.Path(str(prefix) + suffix).symlink_to('/dev/full')
            log = call('measure', snapshot, prefix, expected=1)
            assert reason in log and not pathlib.Path(str(prefix) + '_cells.json').exists(), log
        log = call(*base, '--out', blocker / 'result.json', expected=1)
        assert 'run output failed on rank 0: cannot write ' in log, log
    log = call('run', 'missing-input.b0in', expected=1)
    assert 'run setup failed on rank 0: cannot open ' in log, log
    log = call(*base, '--inject', 'planning:nonsense:0', expected=1)
    assert 'injection selection failed on rank 0:' in log, log
    call('statecheck', snapshot, expected=1)  # Missing output is rejected before IO.
    for stage in ['planning', 'allocation', 'migration', 'mmg', 'interpolation', 'validation', 'splice', 'commit']:
        call(*base, '--inject', stage + ':largest:0', '--out', 'injected.json')
        result = json.loads((RUN / 'injected.json').read_text())
        assert result['injectionsObserved'] == 1 and result['finalValid'], result
        assert result['steps'][0]['nRolled'] > 0 or not result['steps'][0]['committed']
    call(*base, '--inject', 'mmg:first:999', '--out', 'missed.json', expected=2)
    assert json.loads((RUN / 'missed.json').read_text())['status'] == 'INJECTION_MISSED'
    snapshot = next(RUN.glob('box2d_sbase_step*.b0state'))
    data = bytearray(snapshot.read_bytes()); data[0] ^= 1
    (RUN / 'bad.b0state').write_bytes(data)
    call('statecheck', 'bad.b0state', 'bad-out.b0state', expected=1)
    # Recompute the trailer after changing immutable input data: its content hash
    # must still reject the snapshot (independent of the transport checksum).
    data = bytearray(snapshot.read_bytes()); data[98] ^= 1
    checksum = 1469598103934665603
    for b in data[:-8]: checksum = ((checksum ^ b) * 1099511628211) & ((1 << 64) - 1)
    struct.pack_into('Q', data, len(data) - 8, checksum)
    (RUN / 'bad-input.b0state').write_bytes(data)
    output = call('statecheck', 'bad-input.b0state', 'bad-out.b0state', expected=1)
    assert 'immutable input/background hash mismatch' in output
    (RUN / 'truncated.b0state').write_bytes(snapshot.read_bytes()[:100])
    call('statecheck', 'truncated.b0state', 'bad-out.b0state', expected=1)
    ENV['B0_TWICE'] = '1'
    call('serial', 'box3s.b0in', 'history', 1)
    ENV.pop('B0_TWICE')
    h0, h1 = [json.loads((RUN / f'history_s{i}.json').read_text())['hash'] for i in [0, 1]]
    assert h0 == h1, (h0, h1)
    # A fresh prefix prevents stale artifacts from hiding an aborted serial loop.
    tmp = tempfile.mkdtemp(prefix='serial_ridges_', dir=RUN)
    prefix = pathlib.Path(tmp) / 'ref'
    ridge_input = prefix.parent / 'input.b0in'
    call('gen', 'box3s', ridge_input, 2)
    # Keep the oblique anisotropy but bound MMG work. Unlike the isotropic
    # smoke field, this produces cells with nonpositive carried-metric quality.
    data = bytearray(ridge_input.read_bytes())
    _, nv, _, _ = struct.unpack_from('4Q', data, 8)
    offset = 88 + nv * 3 * 8
    tensors = struct.unpack_from(f'{nv * 6}d', data, offset)
    struct.pack_into(f'{nv * 6}d', data, offset, *(v * .003 for v in tensors))
    ridge_input.write_bytes(data)
    input_bytes = ridge_input.read_bytes()
    call('serial', ridge_input, prefix, 24, timeout=90)  # s0 + 19 calibration + 5 controls
    meshes = [pathlib.Path(f'{prefix}_s{i}.b0in') for i in range(25)]
    assert len(list(prefix.parent.glob('ref_s*.b0in'))) == 25
    assert len(list(prefix.parent.glob('ref_s*.json'))) == 25
    output = call('classify', ridge_input, *meshes)
    classes = json.loads(next(line for line in output.splitlines() if line.startswith('{"meshes":')))['meshes']
    ridge_vertices = ridge_cells = 0
    for i, (file, classified) in enumerate(zip(meshes, classes)):
        m = read(file)
        tensors = full(m['M'], 3)
        ridge_vertices += int(np.count_nonzero(np.linalg.eigvalsh(tensors)[:, 0] <= 0))
        ridge_cells += int(np.count_nonzero(np.linalg.det(tensors[m['T']].mean(axis=1)) <= 0))
        vertices = m['X'][m['T']]
        volumes = np.linalg.det(vertices[:, 1:] - vertices[:, :1]) / 6
        assert np.all(np.isfinite(volumes)) and np.all(volumes > 0)
        result = json.loads(file.with_suffix('.json').read_text())
        assert result['renum'] == i and pathlib.Path(result['file']) == file
        assert isinstance(result['hash'], str) and result['status'] in [0, 1]
        assert result['stats']['nv'] == len(m['X']) and result['stats']['ne'] == len(m['T'])
        assert result['stats']['qmin'] > 0 and np.isfinite(result['stats']['qmin'])
        assert sum(result['classCells']) == len(m['T'])
        assert result['classCells'] == classified['classCells']
        assert result['classDensity'] == classified['classDensity']
    assert len(classes) == 25 and ridge_vertices > 0 and ridge_cells > 0
    assert ridge_input.read_bytes() == input_bytes
    print(f'3D serial ridge regression PASS: 25 meshes + 25 JSON, {ridge_vertices} non-SPD vertices, '
          f'{ridge_cells} nonpositive carried-metric cell determinants', flush=True)
    refs = ','.join([str(RUN / 'history_s0.b0in')] * 25)
    call('mask', 'box3s.b0in', '--P', 8, '--serial', refs, '--artifacts', 'mask')
    call('statecheck', 'mask_final.b0state', 'roundtrip.b0state')
    call('perturb', 'mask_final.b0state', 20, .05, 1701, 'perturbed')
    sys.path.insert(0, str(ROOT / 'B0Spike/py'))
    from e0_eval import evaluate
    from e0gates import score
    measured = evaluate(RUN / 'mask_cells.json')
    assert measured['candidate']['Q2']['ok']
    artifact = read_cells(RUN / 'mask_cells.json')
    legacy = dict(artifact, candidate=dict(artifact['candidate'], cells=list(artifact['candidate']['cells'])),
                  serial=[dict(m, cells=list(m['cells'])) for m in artifact['serial']], faces=list(artifact['faces']))
    legacy.pop('format')
    (RUN / 'mask_legacy_cells.json').write_text(json.dumps(legacy))
    old = evaluate(RUN / 'mask_legacy_cells.json'); old.pop('file')
    new = dict(measured); new.pop('file')
    assert old == new
    perturbed = read_cells(RUN / 'perturbed_cells.json')['candidate']
    assert score(perturbed, measured['model'])['worstLoss'] is not None
    result = json.loads((RUN / 'box2d_sbase.json').read_text())
    call('replay', 'box2d_sbase_step0.b0state', result['steps'][0]['pieceIds'][0], 'piece')
    assert json.loads((RUN / 'piece.json').read_text())['status'] == 0
    print('E0 serial smoke PASS: 2D/3D runs, collective/count selftests, snapshots, snapshot/statecheck/posthash/result write failures, setup/selector/argument failures, full-transaction replay, distances, 8 injections, missed injection, corrupt/truncated snapshots, immutable-input verification, call-history reset, 25 serial ridge references and JSON, mask/perturb/scoring, piece replay')


if __name__ == '__main__':
    main()
