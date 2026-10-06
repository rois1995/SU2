"""R3 G0 pilot: frozen masks, run-clustered serial controls, Q2-only power windows.

Run only after R0 and R2. This is the long series runner for Claude, not a smoke test.
No scheduler/condition choice is made here. Out-of-window perturbations are recorded
as discarded. An unfilled allocation makes G0 fail rather than silently reducing n.
"""
import argparse
import json
import os
import pathlib
import statistics
import subprocess
import time
from e0gates import score, bootstrap, achieved_loss, unscorable
from e0_eval import evaluate
from e0_cells import read

CASES = ['naca0', 'naca1', 'cyl0', 'box2d', 'box2i', 'box2x', 'box3s', 'box3d', 'm60', 'm61', 'box3c', 'box2r']
WINDOWS = [(0.05, 0.075), (0.10, 0.125), (0.20, 0.25)]


def run(command, cwd):
    while os.getloadavg()[0] >= 7:
        time.sleep(5)
    with (cwd / 'commands.log').open('a') as log:
        log.write(repr(command) + '\n'); log.flush()
        limit = 2700 if cwd.name.startswith(('box3', 'm6')) else 900
        result = subprocess.run(['timeout', '--kill-after=30s', f'{limit}s'] + [str(c) for c in command],
                       cwd=cwd, stdout=log, stderr=subprocess.STDOUT, check=False,
                       env=dict(os.environ, B0_SERIAL_ONLY='1', OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1'))
    if result.returncode:
        marker = cwd / (pathlib.Path(command[-1]).name + '.FAILED')
        marker.write_text(json.dumps(dict(exitCode=result.returncode, command=list(map(str, command)))) + '\n')
        print(f'failed rc={result.returncode}: {command}; continuing independent jobs', flush=True)
        return False
    return True


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', type=pathlib.Path, required=True)
    p.add_argument('--bin', type=pathlib.Path, required=True)
    args = p.parse_args()
    root, binary = args.root, args.bin
    out = root / 'r3'; out.mkdir(parents=True, exist_ok=True)
    models, rows = {}, []
    for case in CASES:
        wd = out / case; wd.mkdir(exist_ok=True)
        refs = ','.join(str(root / 'r0' / case / f'ser_s{i}.b0in') for i in range(25))
        casefile = root.parent / 'cases' / (case + '.b0in')
        if case in ('box3c', 'box2r'):
            casefile = root / 'cases' / (case + '.b0in')
        completed = run([binary, 'mask', casefile, '--P', '8', '--placement', 'Pb', '--serial', refs, '--artifacts', wd / 'mask'], wd)
        files = [wd / 'mask_cells.json'] + sorted((root / 'r2' / case).glob('*/state_cells.json'))
        if completed is False:
            reason = 'mask job failed; see mask.FAILED and commands.log'
            evaluations = [dict(file=str(files[0]), model=None, candidate=unscorable(reason),
                                controls=[unscorable(reason) for _ in range(5)], damage=[], calibrationError=reason)]
            evaluations.extend(evaluate(f) for f in files[1:])
        else:
            evaluations = [evaluate(f) for f in files]
        rows.extend(evaluations)
        models[case] = evaluations[0]['model']
    # One replication is a serial run, with its mask pass fraction; never a cell or mask.
    control = {}
    for gate in ['Q2', 'Q4b', 'tail']:
        run_rates = []
        for case in CASES:
            case_rows = [r for r in rows if case in pathlib.Path(r['file']).parts]
            for i in range(5):
                run_rates.append(statistics.mean(r['controls'][i][gate]['ok'] for r in case_rows))
        control[gate] = dict(mean=statistics.mean(run_rates), interval90=bootstrap(run_rates),
                             runReplications=len(run_rates), ok=statistics.mean(run_rates) >= 0.95)
    power, discarded = [], []
    for case in CASES:
        wd = out / case
        model = models[case]
        if model is None:
            reason = next(r['calibrationError'] for r in rows if pathlib.Path(r['file']).parent == wd)
            power.extend(dict(case=case, window=w, replicate=i, unfilled=True, reason=reason, trials=[])
                         for w in range(3) for i in range(6))
            continue
        for window_id, (low, high) in enumerate(WINDOWS):
            for replicate in range(6):
                serial_index = 20 + replicate % 5
                seed = 170100 + 1000 * CASES.index(case) + 100 * window_id + replicate
                left, right = 0.0, 1.6
                prefix = wd / f'power_w{window_id}_rep{replicate}'
                accepted = None
                trials = []
                for attempt in range(20):
                    amplitude = (left + right) / 2
                    if run([binary, 'perturb', wd / 'mask_final.b0state', serial_index, amplitude, seed, prefix], wd) is False:
                        trials.append(dict(amplitude=amplitude, loss=None, reason='perturbation job failed'))
                        break
                    candidate = read(pathlib.Path(str(prefix) + '_cells.json'))['candidate']
                    loss = achieved_loss(candidate, model)
                    trials.append(dict(amplitude=amplitude, loss=loss))
                    if loss is not None and low <= loss < high:
                        accepted = dict(case=case, dim=3 if case.startswith('box3') or case.startswith('m6') else 2,
                                        window=window_id, replicate=replicate, serialIndex=serial_index,
                                        seed=seed, amplitude=amplitude, loss=loss, detected=not score(candidate, model)['Q2']['ok'],
                                        file=str(prefix) + '.b0in', trials=trials)
                        break
                    discarded.append(dict(case=case, window=window_id, replicate=replicate, attempt=attempt, amplitude=amplitude, loss=loss))
                    if loss is None or loss >= high:
                        right = amplitude
                    else:
                        left = amplitude
                if accepted:
                    power.append(accepted)
                else:
                    power.append(dict(case=case, window=window_id, replicate=replicate, unfilled=True, trials=trials))
    summary, floors, dims = [], [0.30, 0.70, 0.90], [0.50, 0.90, 1.0]
    for case in CASES:
        for w in range(3):
            group = [r for r in power if r['case'] == case and r['window'] == w and not r.get('unfilled')]
            clusters = [[r['detected'] for r in group if r['serialIndex'] == i] for i in range(20, 25)]
            cluster_rates = [statistics.mean(g) for g in clusters if g]
            rate = statistics.mean(r['detected'] for r in group) if group else 0
            summary.append(dict(case=case, window=w, n=len(group), serialRuns=len(cluster_rates), rate=rate,
                                interval90=bootstrap(cluster_rates), ok=len(group) >= 6 and len(cluster_rates) >= 3 and rate >= floors[w]))
    dimensional = []
    for dim in [2, 3]:
        for w in range(3):
            group = [r for r in power if r.get('dim') == dim and r['window'] == w and not r.get('unfilled')]
            rate = statistics.mean(r['detected'] for r in group) if group else 0
            clusters = {}
            for r in group:
                clusters.setdefault((r['case'], r['serialIndex']), []).append(r['detected'])
            interval = bootstrap([statistics.mean(g) for g in clusters.values()])
            dimensional.append(dict(dim=dim, window=w, n=len(group), rate=rate, interval90=interval, ok=rate >= dims[w]))
    controls = [r for r in rows if 'r2' in pathlib.Path(r['file']).parts]
    # Only the ten full-run known failures are Q2 negative controls; N2 masks test nulls too.
    failures = [r for r in controls if not pathlib.Path(r['file']).parent.name.startswith('level0')]
    negative = bool(failures) and len(failures) == 10 and all(r['candidate']['worstLoss'] is not None and not r['candidate']['Q2']['ok'] for r in failures)
    unscorable_rows = [dict(file=r['file'], mesh=name, reason=s['reason']) for r in rows
                  for name, s in [('candidate', r['candidate'])] + [(f'control {i}', s) for i, s in enumerate(r['controls'])]
                  if s['reason']]
    report = dict(label='pilot measurement; not certified 99% power', windows=WINDOWS, calibrationMeshes=19,
                  heldBackMeshes=5, null=control, perCasePower=summary, dimensionalPower=dimensional,
                  negativeControlsPass=negative, G0=all(r['ok'] for r in control.values()) and all(r['ok'] for r in summary)
                  and all(r['ok'] for r in dimensional) and negative,
                  masks=rows, power=power, discarded=discarded, unscorable=unscorable_rows)
    (out / 'g0.json').write_text(json.dumps(report, indent=2) + '\n')
    for failure in unscorable_rows:
        print(f"{failure['file']}: {failure['mesh']} unscorable: {failure['reason']}")
    print('G0', report['G0'], '->', out / 'g0.json')
    return 0 if report['G0'] else 2


if __name__ == '__main__':
    raise SystemExit(main())
