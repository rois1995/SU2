"""Short E0 calibration checks, including the revision-4 zero-noise regression."""
import copy
import contextlib
import io
import json
import pathlib
import struct
import subprocess
import sys
import tempfile
from unittest.mock import patch
from e0_verify import main as verify
from e0_eval import evaluate
from e0_cells import read as read_cells
from e0gates import calibrate, score, merge_masks, attribution, regional, achieved_loss
from b0gates import class2
import e0_g0
import summ_b0
import summ_full


def mesh(q=1.0, nv=1000, count=400):
    return dict(nv=nv, cells=[dict(ar=0, db=0, q=q, distance=0.5, face=0) for _ in range(count)])


def reporting_regressions(stats):
    ref = mesh(count=300)
    ref['cells'] += [dict(ar=0, db=1, q=.1, distance=1.5, face=0) for _ in range(1000)]
    smaller = copy.deepcopy(ref)
    smaller['cells'].pop(0)
    good = dict(final=stats(ref), serial=[stats(ref)] * 25, surface=0, status='COMPLETE',
                steps=[], finalC=0, finalD=0, dim=2, sched='sbase', P=4, L=2,
                tMMGtotal=1., critPathTotal=1., neIn=1300, tRun=1., partition=dict(tPlacement=0.), hash='123')
    tiny = mesh(count=299)
    cases = [('calibration_small', dict(good, serial=[stats(tiny)] * 25),
              dict(candidate=ref, serial=[tiny] * 25, faces=[dict(kind=0, step=0)]),
              'mask cannot meet the 300-cell minimum; more cells are required'),
             ('candidate_shrink', dict(good, final=stats(smaller)),
              dict(candidate=smaller, serial=[ref] * 25, faces=[dict(kind=0, step=0)]),
              'frozen region below minimum')]
    expected = class2(good)
    expected_score = score(ref, calibrate(ref, [ref] * 19))
    def same_fields(result, baseline):
        assert result.keys() == baseline.keys()
        for key in baseline:
            if key.startswith('Q') or key == 'tail':
                assert result[key].keys() == baseline[key].keys(), key
    with tempfile.TemporaryDirectory(prefix='e0_reporting_') as tmp:
        root = pathlib.Path(tmp)
        artifacts = []
        for case, run, artifact, reason in cases:
            gates = class2(run)
            same_fields(gates, expected)
            assert not gates['classII'] and not gates['G0Eligible']
            assert gates['reason'] == gates['Q2']['reason'] == reason
            assert gates['Q2']['worstRel'] is gates['worstLoss'] is gates['Q2']['losses'] is None
            for base, tag in [('n3', 'Pb_P4_L2'), ('n5', 'background')]:
                dest = root / base / case / tag / 'result.json'
                dest.parent.mkdir(parents=True)
                dest.write_text(json.dumps(run))
            path = root / (case + '_cells.json')
            path.write_text(json.dumps(artifact)); artifacts.append(path)
            evaluated = evaluate(path)
            same_fields(evaluated['candidate'], expected_score)
            for control in evaluated['controls']: same_fields(control, expected_score)
            assert evaluated['candidate']['reason'] == reason and evaluated['candidate']['worstLoss'] is None
            if case == 'calibration_small':
                assert evaluated['model'] is None and evaluated['damage'] == []
                assert evaluated['calibrationError'] == reason
                assert all(not c['Q2']['ok'] and c['reason'] == reason for c in evaluated['controls'])
            else:
                assert evaluated['model'] is not None and evaluated['calibrationError'] is None
                assert all(c['Q2']['ok'] for c in evaluated['controls'])
        dest = root / 'n4/healthy/Pb_P4_L2/result.json'
        dest.parent.mkdir(parents=True); dest.write_text(json.dumps(good))
        for run in [dict(good, serial=[]), dict(good, serial=good['serial'][:19])]:
            same_fields(class2(run), expected)
        with patch.object(summ_full, 'R', str(root)), patch.object(summ_b0, 'R', str(root)), patch.dict(summ_full.SERIAL_MMG, {}, clear=True):
            for index, report in enumerate([lambda: summ_full.main('n3'), lambda: summ_b0.full(('n3',)),
                                            lambda: summ_b0.full(('n3', 'n4')), summ_b0.n5, summ_b0.classifier]):
                output = io.StringIO()
                with contextlib.redirect_stdout(output): report()
                for _, _, _, reason in cases: assert reason in output.getvalue(), output.getvalue()
                assert 'unscorable' in output.getvalue()
                lines = output.getvalue().splitlines()
                if index == 0:
                    assert all(line.split('\t')[8] == 'N(-,-)' and line.split('\t')[12] == 'N' for line in lines[1:])
                elif index in (1, 2):
                    row = lines[1].split('\t')
                    assert row[6] == row[9] == ('0' if index == 1 else '1')
                    if index == 1:
                        assert row[12] == '-'
                    else:
                        assert all(float(v) == 0. for v in row[12].translate(str.maketrans('[],', '   ')).split()), row[12]
        # Exercise the actual multi-artifact CLI: one failure cannot abort the
        # summary or suppress a later artifact's diagnostic.
        output = subprocess.run([sys.executable, '-B', str(pathlib.Path(__file__).with_name('e0_eval.py')),
                                 *map(str, artifacts), '--out', str(root / 'evaluation.json')],
                                capture_output=True, text=True, check=True)
        assert len(json.loads((root / 'evaluation.json').read_text())) == 2
        for _, _, _, reason in cases: assert reason in output.stdout
        assert 'control 0 unscorable' in output.stdout
        # A missing calibration model must produce an unfilled, failed G0
        # report without attempting heavy perturbation jobs.
        mask = root / 'r3/box2d/mask_cells.json'
        mask.parent.mkdir(parents=True); mask.write_text(json.dumps(cases[0][2]))
        output = io.StringIO()
        with patch.object(e0_g0, 'CASES', ['box2d']), patch.object(e0_g0, 'run') as calls, \
                patch.object(sys, 'argv', ['e0_g0', '--root', str(root), '--bin', 'unused']), \
                contextlib.redirect_stdout(output):
            assert e0_g0.main() == 2
        assert calls.call_count == 1  # Mask job only; no perturbations.
        report = json.loads((root / 'r3/g0.json').read_text())
        assert not report['G0'] and len(report['power']) == 18
        assert all(p['unfilled'] and p['reason'] == cases[0][3] for p in report['power'])
        assert len(report['unscorable']) == 6 and cases[0][3] in output.getvalue()
        # A timed-out mask must leave its case unfilled and still try the next case.
        with patch.object(e0_g0, 'CASES', ['box2d', 'box3s']), patch.object(e0_g0, 'run', return_value=False) as calls, \
                patch.object(sys, 'argv', ['e0_g0', '--root', str(root), '--bin', 'unused']), \
                contextlib.redirect_stdout(io.StringIO()):
            assert e0_g0.main() == 2
        assert calls.call_count == 2
        report = json.loads((root / 'r3/g0.json').read_text())
        assert len(report['power']) == 36 and all(p['unfilled'] for p in report['power'])



def cell_format_regression():
    # Same logical artifact through both loaders: exact gate/attribution results,
    # boundary-quality doubles, 64-bit IDs, dimensions, edges and bad payloads.
    with tempfile.TemporaryDirectory(prefix='e0_cells_') as tmp:
        root = pathlib.Path(tmp)
        for dim in (2, 3):
            nedge = dim * (dim + 1) // 2
            candidate = mesh(.95)
            for c in candidate['cells']:
                c.update(**{'class': 0}, edges=[1.25] * nedge)
            ref = copy.deepcopy(candidate)
            for c in ref['cells']: c['q'] = 1.
            faces = [dict(step=2, kind=1, piece=str(2**63-1), beforeQ=.7, afterQ=.8,
                          finalQ=.9, retainedFace=True)]
            old = dict(hash=str(2**64-1), candidate=candidate, serial=[ref] * 25, faces=faces)
            legacy = root / 'legacy.json'; legacy.write_text(json.dumps(old))
            payload = bytearray()
            def pack(m):
                result = dict(nv=m['nv'], ne=len(m['cells']), offset=len(payload))
                for c in m['cells']:
                    payload.extend(struct.pack('<BBBddq' + str(nedge) + 'f', c['class'], c['ar'], c['db'],
                                               c['q'], c['distance'], c['face'], *c['edges']))
                return result
            header = dict(format='B0CELLS1', dim=dim, hash=old['hash'], candidate=pack(candidate),
                          serial=[pack(ref) for _ in range(25)])
            header['faces'] = dict(offset=len(payload), count=1)
            f = faces[0]
            payload.extend(struct.pack('<iiqdddB', f['step'], f['kind'], int(f['piece']),
                                       f['beforeQ'], f['afterQ'], f['finalQ'], f['retainedFace']))
            header['bytes'] = len(payload)
            path = root / 'packed.json'; path.write_text(json.dumps(header)); path.with_suffix('.bin').write_bytes(payload)
            decoded = read_cells(path)
            assert decoded['hash'] == old['hash']
            assert list(decoded['candidate']['cells']) == candidate['cells']
            assert list(decoded['faces']) == faces
            a, b = evaluate(legacy), evaluate(path); a.pop('file'); b.pop('file')
            assert a == b and b['candidate']['Q2']['ok']
            for bad in [dict(header, format='B0CELLS2'), dict(header, dim=4), dict(header, bytes=len(payload)-1),
                        dict(header, candidate=dict(header['candidate'], offset=1)),
                        dict(header, candidate=dict(header['candidate'], ne=-1))]:
                path.write_text(json.dumps(bad))
                try: read_cells(path)
                except ValueError: pass
                else: raise AssertionError('bad binary header accepted')
            path.write_text(json.dumps(header)); path.with_suffix('.bin').write_bytes(payload[:-1])
            try: read_cells(path)
            except ValueError: pass
            else: raise AssertionError('truncated payload accepted')


def main():
    cell_format_regression()
    ref = mesh()
    model = calibrate(ref, [copy.deepcopy(ref) for _ in range(19)])
    assert model['k'] == model['kt'] == 0
    assert model['q2Boundary'] == [0.05]
    assert model['vertexBoundary'] == 0.02
    assert model['tailBoundary'] == [1e-4]
    assert score(mesh(0.95), model)['Q2']['ok']
    assert not score(mesh(0.949), model)['Q2']['ok']
    assert score(mesh(nv=1020), model)['Q4b']['ok']
    assert not score(mesh(nv=1021), model)['Q4b']['ok']
    bad = mesh(); bad['cells'][0]['q'] = 0.4
    assert not score(bad, model)['tail']['ok']
    ref_tail = mesh()
    for c in ref_tail['cells'][:2]: c['q'] = 0.4
    mt = calibrate(ref_tail, [copy.deepcopy(ref_tail) for _ in range(19)])
    assert mt['tailBoundary'] == [1.25 * 0.005 + 1e-4]
    null = [mesh(0.90 if i % 2 else 1.0) for i in range(19)]
    noisy = calibrate(ref, null)
    assert noisy['k'] >= 0 and all(b >= 0.05 for b in noisy['q2Boundary'])
    try:
        calibrate(ref, null[:18])
        assert False
    except ValueError:
        pass
    small = mesh(count=350)
    for c in small['cells'][:50]: c['db'] = 1
    assert merge_masks([small] * 20) == [list(range(24))]
    tiny = mesh(count=20)
    try:
        merge_masks([tiny] * 20)
        assert False
    except ValueError:
        pass
    art = dict(candidate=ref, serial=[ref] * 25, faces=[dict(kind=0, step=0)])
    assert attribution(art, model)[0]['excess'] == 0
    far = copy.deepcopy(art)
    for c in far['candidate']['cells']: c['distance'] = 9
    assert any(r['kind'] == 'unattributed' for r in attribution(far, model))
    quiet_ref = mesh()
    quiet_ref['cells'] += [dict(ar=1, db=0, q=1., distance=.5, face=0) for _ in range(400)]
    varied = [copy.deepcopy(quiet_ref) for _ in range(19)]
    for i, m in enumerate(varied):
        for c in m['cells'][400:]: c['q'] = .9 if i % 2 else 1.
    regional_model = calibrate(quiet_ref, varied)
    assert regional_model['q2Boundary'][0] < regional_model['q2Boundary'][1]
    def stats(m):
        regions = [dict(ar=a, db=d, nCell=0, q1=0., bad=0., nEdge=0, edgeIn=-1., quality=[]) for a in range(4) for d in range(6)]
        for c in m['cells']:
            r = regions[6 * c['ar'] + c['db']]; r['quality'].append(c['q']); r['nCell'] += 1
        for r in regions:
            if r['quality']: r['q1'] = sorted(r['quality'])[int(.01 * (len(r['quality']) - 1))]
        return dict(nv=m['nv'], ne=len(m['cells']), regions=regions, edgeIn=1., edgeInNear=1., nEdgeNear=1, badFrac=0., nBad=0,
                    complexityTarget=1., complexityCarried=1., devMean=[], devMax=[])
    reporting_regressions(stats)
    run = dict(final=stats(ref), serial=[stats(ref) for _ in range(25)], surface=0, status='COMPLETE')
    gates = class2(run)
    assert gates['G0Eligible'] and gates['classII']
    for old_run in [dict(run, serial=run['serial'][:19]), copy.deepcopy(run)]:
        if len(old_run['serial']) == 25:
            for r in old_run['final']['regions']: r.pop('quality')
        gates = class2(old_run)
        assert not gates['G0Eligible'] and not gates['classII']
        assert all(k in gates for k in ['Q1', 'Q1near', 'Q2', 'Q3', 'Q4', 'Q5'])
    run['final'] = stats(mesh(.949))
    assert not class2(run)['Q2']['ok']
    # Regression: a candidate must not merge away a failing 300-cell near region
    # by reducing its count to 299 beside 1,000 low-quality neighbouring cells.
    regional_ref = mesh(count=300)
    regional_ref['cells'] += [dict(ar=0, db=1, q=.1, distance=1.5, face=0) for _ in range(1000)]
    serial = [copy.deepcopy(regional_ref) for _ in range(25)]
    for held_back in serial[20:]:
        held_back['cells'].pop(0)
    frozen = calibrate(serial[0], serial[1:20])
    assert frozen['masks'] == [[0] + list(range(6, 24)), list(range(1, 6))]
    with tempfile.TemporaryDirectory(prefix='e0_frozen_masks_') as tmp:
        path = pathlib.Path(tmp) / 'cells.json'
        for count, quality in [(300, 1.), (300, .6), (299, .6), (299, 1.)]:
            candidate = mesh(quality, count=count)
            candidate['cells'] += copy.deepcopy(regional_ref['cells'][300:])
            run = dict(final=stats(candidate), serial=[stats(m) for m in serial], surface=0, status='COMPLETE')
            gates = class2(run)
            path.write_text(json.dumps(dict(candidate=candidate, serial=serial, faces=[dict(kind=0, step=0)])))
            evaluated = evaluate(path)
            assert gates['calibration'] == evaluated['model'] == frozen
            assert gates['Q2']['ok'] == evaluated['candidate']['Q2']['ok'] == (count == 300 and quality == 1.)
            assert gates['classII'] == (count == 300 and quality == 1.)
            assert all(not control['Q2']['ok'] for control in evaluated['controls'])
            if count == 299:
                assert gates['Q2']['reason'] == 'frozen region below minimum'
                assert not gates['tail']['ok'] and not gates['Q4b']['ok']
        # Calibration counts still determine merges; held-back/candidate counts do not.
        calibration = copy.deepcopy(serial[1:20])
        calibration[0]['cells'].pop(0)
        assert calibrate(serial[0], calibration)['masks'] == [list(range(24))]
    # Every bin empty in ALL reference/calibration meshes belongs to a frozen
    # region. Moving a candidate OR held-back population there cannot hide loss.
    empty_ref = mesh(count=1000)
    for c in empty_ref['cells']: c['db'] = 1
    empty_serial = [copy.deepcopy(empty_ref) for _ in range(25)]
    empty_model = calibrate(empty_serial[0], empty_serial[1:20])
    assert sorted(b for g in empty_model['masks'] for b in g) == list(range(24))
    with tempfile.TemporaryDirectory(prefix='e0_empty_bins_') as tmp:
        path = pathlib.Path(tmp) / 'cells.json'
        for b in range(24):
            candidate = copy.deepcopy(empty_ref)
            for c in candidate['cells'][:400]:
                c.update(ar=b // 6, db=b % 6, q=.6)
            controls = copy.deepcopy(empty_serial)
            controls[20:25] = [copy.deepcopy(candidate) for _ in range(5)]
            gates = class2(dict(final=stats(candidate), serial=[stats(m) for m in controls], surface=0, status='COMPLETE'))
            path.write_text(json.dumps(dict(candidate=candidate, serial=controls, faces=[dict(kind=0, step=0)])))
            evaluated = evaluate(path)
            assert gates['calibration'] == evaluated['model'] == empty_model
            assert not gates['classII'] and not gates['Q2']['ok']
            assert not evaluated['candidate']['Q2']['ok']
            assert all(not control['Q2']['ok'] for control in evaluated['controls'])
            assert abs(evaluated['candidate']['worstLoss'] - .4) < 1e-12
            assert sum(len(q) for q in regional(candidate, empty_model['masks'])) == 1000
            assert sum(row['n'] for row in evaluated['damage']) == 1000
            # Healthy cells in the same formerly empty bin still pass.
            for c in candidate['cells']: c['q'] = 1.
            assert score(candidate, empty_model)['Q2']['ok']
        # NaN can vanish under sorting/comparisons, and invalid db values can
        # alias valid AR bins. Even ONE invalid cell must fail, including in a
        # population large enough for a zero-quality cell to fit the tail floor.
        large = mesh(count=10001)
        invalid_cells = [dict(q=q) for q in [float('nan'), float('inf'), -float('inf'), 0., -.1]]
        invalid_cells += [dict(ar=a, db=d) for a, d in [(-1, 0), (4, 0), (0, -1), (0, 6), (-1, 6), (4, -1), (0., 0), (0, 0.5)]]
        invalid_cells += [dict(**{'class': c}) for c in [-1, 8, 1]]
        for change in invalid_cells:
            candidate = copy.deepcopy(large)
            candidate['cells'][-1].update(change)
            rejected = score(candidate, empty_model)
            assert all(not rejected[g]['ok'] for g in ['Q2', 'tail', 'Q4b']), change
            assert rejected['worstLoss'] is None and achieved_loss(candidate, empty_model) is None
            controls = copy.deepcopy(empty_serial)
            controls[20] = copy.deepcopy(candidate)
            path.write_text(json.dumps(dict(candidate=candidate, serial=controls, faces=[dict(kind=0, step=0)])))
            evaluated = evaluate(path)
            assert not evaluated['candidate']['Q2']['ok'] and not evaluated['controls'][0]['Q2']['ok']
            assert sum(row['n'] for row in evaluated['damage']) == len(candidate['cells'])
            try:
                calibrate(empty_ref, [candidate] + empty_serial[1:19])
            except ValueError:
                pass
            else:
                raise AssertionError('invalid calibration population accepted')
        # Supplied masks cannot omit or duplicate bins, even when those bins
        # happen to be empty in the candidate and calibration.
        for masks in [[[0]], [list(range(24)), [0]], [[], list(range(24))], [[i] for i in range(24)]]:
            broken_model = dict(empty_model, masks=masks)
            assert not score(empty_ref, broken_model)['Q2']['ok']
            try:
                calibrate(empty_ref, empty_serial[1:20], masks=masks)
            except ValueError:
                pass
            else:
                raise AssertionError('incomplete/duplicate/undersized frozen partition accepted')
        # Both statistics and artifacts account for the stated cell population.
        candidate = dict(empty_ref, ne=1001)
        assert not score(candidate, empty_model)['Q2']['ok']
        clean_stats = stats(empty_ref)
        bad_stats = []
        for mutate in [lambda s: s['regions'][1]['quality'].pop(),
                       lambda s: s.update(ne=1001),
                       lambda s: s['regions'].pop(),
                       lambda s: s['regions'].__setitem__(0, copy.deepcopy(s['regions'][1])),
                       lambda s: s['regions'][0].update(ar=-1, db=6),
                       lambda s: s['regions'][1]['quality'].__setitem__(-1, float('nan')),
                       lambda s: s['regions'][1]['quality'].__setitem__(-1, 0.)]:
            broken = copy.deepcopy(clean_stats); mutate(broken); bad_stats.append(broken)
        for broken in bad_stats:
            for final, serial_stats in [(broken, [clean_stats] * 25), (clean_stats, [clean_stats, broken] + [clean_stats] * 23)]:
                gates = class2(dict(final=final, serial=serial_stats, surface=0, status='COMPLETE'))
                assert not gates['classII'] and not gates['G0Eligible'] and not gates['Q2']['ok']
    # Audit decisions use synthetic artifacts; this does not exercise real MPI.
    with tempfile.TemporaryDirectory(prefix='e0_verify_unit_') as tmp:
        root = pathlib.Path(tmp)
        base = dict(hash=str(2**63 + 17), finalValid=True, planMismatches=0,
                    steps=[dict(step=0, committed=True, nRolled=0, failures=[])])
        def write(path, data):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(data))
        for case in ['box3s', 'box3d', 'naca1', 'box2d']:
            for sched in ['sbase', 'sbatch']:
                for ranks in [1, 2]: write(root / 'r1' / f'r{ranks}' / case / sched / 'result.json', base)
        for ranks in [1, 2]:
            for sched in ['sbase', 'sbatch']:
                for stage in ['planning', 'allocation', 'migration', 'mmg', 'interpolation', 'validation', 'splice', 'commit']:
                    for step in [0, 1]:
                        for piece in ['first', 'largest']:
                            wd = root / 'inject' / f'r{ranks}' / f'{sched}_{stage}_{piece}_s{step}'
                            r = dict(base, status='INCOMPLETE_COVERAGE' if stage == 'commit' else 'COMPLETE', injectionsObserved=1,
                                     steps=[dict(step=step, committed=stage != 'commit', nRolled=1, failures=[])])
                            write(wd / 'result.json', r); (wd / 'state_final.b0state').touch()
        for stage in ['validation', 'interpolation']:
            wd = root / 'late_mpi' / f'{stage}_first_s1'
            write(wd / 'result.json', dict(base, status='INCOMPLETE_COVERAGE', injectionsObserved=1,
                                          steps=[dict(step=1, committed=True, nRolled=1, failures=[])]))
            (wd / 'state_final.b0state').touch()
        with contextlib.redirect_stdout(io.StringIO()):
            assert verify(root) == 0
            bad = root / 'r1/r2/box3s/sbase/result.json'
            write(bad, dict(base, hash='123'))
            assert verify(root) == 2
            write(bad, base)
            missing = root / 'inject/r1/sbase_mmg_first_s0/result.json'
            r = json.loads(missing.read_text()); r['injectionsObserved'] = 0; write(missing, r)
            assert verify(root) == 2
    print('E0 gates: PASS (binary/legacy cells, exact boundary gates/attribution, malformed/truncated payloads, zero noise, margins, complete frozen partition, candidate/held-back empty bins, invalid cells/counts, frozen 300-vs-299 masks, unscorable calibration/candidate reporting and G0 failure, tail, attribution, G-R audit decisions)')


if __name__ == '__main__':
    main()
