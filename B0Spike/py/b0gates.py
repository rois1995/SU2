"""Gate evaluation of B0 runs (B_DESIGN_EXPLORATION.md section 7).

A run JSON holds the candidate's final statistics and the statistics of the serial references (identity first,
then the renumbered runs) evaluated with the candidate's former interfaces, so matched regions are the same.
"""
import json, math, statistics, sys

DB_EDGES = [0, 1, 2, 4, 8, 16]


def sd(vals):
    return statistics.stdev(vals) if len(vals) > 1 else 0.0


def delta(dabs, sigma):
    return min(2 * dabs, max(dabs, 3 * sigma))


def load(path):
    with open(path) as f:
        return json.load(f)


def legacy_class2(run, minCells=100):
    """Class II gates of one run. Returns a dict with per-gate pass/fail and margins."""
    cand = run['final']
    ser = run['serial']
    if not ser:
        return None
    s0, ren = ser[0], ser[1:]
    g = {}
    # Q-1 global and near cuts (good-is-high, lower limit)
    for key, dabs in (('edgeIn', 0.02), ('edgeInNear', 0.04)):
        sig = sd([s[key] for s in ren])
        d = delta(dabs, sig)
        x, xs = cand[key], s0[key]
        if key == 'edgeInNear' and cand['nEdgeNear'] == 0:
            g['Q1near'] = dict(cand=None, serial=xs, delta=d, ok=True, improve=False)
            continue
        g['Q1' + ('near' if key == 'edgeInNear' else '')] = dict(cand=x, serial=xs, delta=d, ok=x >= xs - d,
                                                               improve=x > xs + d)
    # Q-2 matched-region quality p1 (lower limit, 5 % relative), degradation width
    regs = {}
    for i, r in enumerate(cand['regions']):
        rs = s0['regions'][i]
        if r['nCell'] < minCells or rs['nCell'] < minCells:
            continue
        sig = sd([s['regions'][i]['q1'] for s in ren])
        d = delta(0.05 * rs['q1'], sig)
        regs[(r['ar'], r['db'])] = dict(cand=r['q1'], serial=rs['q1'], delta=d, ok=r['q1'] >= rs['q1'] - d,
                                        improve=r['q1'] > rs['q1'] + d, n=r['nCell'])
    fails = [k for k, v in regs.items() if not v['ok']]
    worst = min(((v['cand'] - v['serial']) / max(v['serial'], 1e-12), k) for k, v in regs.items()) if regs else (0, None)
    width = 0
    for k in fails:
        width = max(width, DB_EDGES[k[1] + 1] if k[1] + 1 < len(DB_EDGES) else 99)
    g['Q2'] = dict(ok=not fails, nRegions=len(regs), fails=[list(k) for k in fails], worstRel=worst[0],
                   worstRegion=list(worst[1]) if worst[1] else None, degradationWidth=width,
                   improve=any(v['improve'] for v in regs.values()))
    # near-cut region quality (db 0-1) as a separate reading
    near = [v for k, v in regs.items() if k[1] <= 1]
    g['Q2near'] = dict(ok=all(v['ok'] for v in near), improve=any(v['improve'] for v in near),
                       minRel=min(((v['cand'] - v['serial']) / max(v['serial'], 1e-12)) for v in near) if near else None)
    # Q-3 bad fraction (good-is-low, upper limit r = 0.25, floor 1e-4)
    x, xs = cand['badFrac'], s0['badFrac']
    g['Q3'] = dict(cand=x, serial=xs, ok=x <= xs * 1.25 + 1e-4, improve=x < xs - max(1e-4, 3 * sd([s['badFrac'] for s in ren])))
    # Q-4 complexity: number of vertices, two-sided 2 %
    sig = sd([s['nv'] for s in ren])
    d = delta(0.02 * s0['nv'], sig)
    g['Q4'] = dict(cand=cand['nv'], serial=s0['nv'], rel=(cand['nv'] - s0['nv']) / s0['nv'], delta=d,
                   ok=abs(cand['nv'] - s0['nv']) <= d)
    # Q-5 surface
    if run.get('surface', 1):
        okm, rows = True, []
        for i, (dm, dx) in enumerate(zip(cand['devMean'], cand['devMax'])):
            sm, sx = s0['devMean'][i], s0['devMax'][i]
            okMean = dm <= sm * 1.1 + 1e-12
            okMax = dx <= 2 * run.get('hausd', 0.01)
            serMaxOk = sx <= 2 * run.get('hausd', 0.01)
            rows.append(dict(marker=i, devMean=dm, serMean=sm, devMax=dx, serMax=sx, okMean=okMean, okMax=okMax,
                             serialMaxOk=serMaxOk))
            # the absolute bound only counts where the serial path meets it (else reported)
            okm &= okMean and (okMax or not serMaxOk or dx <= sx * 1.1)
        g['Q5'] = dict(ok=okm, markers=rows)
    else:
        g['Q5'] = dict(ok=run['status'] != 'ROLLED_BACK', note='fixed surface bitwise (acceptance check at every commit)')
    g['classII'] = all(g[k]['ok'] for k in ('Q1', 'Q1near', 'Q2', 'Q3', 'Q4', 'Q5'))
    return g


def class2(run, minCells=300):
    """E0 calibration replaces noise terms, preserving nominal effect margins."""
    from e0gates import calibrate, score, stats_cells, unscorable
    # The same fields exist for scored, legacy and unscorable runs. Unknown
    # measurements stay null; they must never masquerade as zero loss.
    g = dict(Q1=dict(ok=False, cand=None, serial=None, delta=None, improve=False),
             Q1near=dict(ok=False, cand=None, serial=None, delta=None, improve=False),
             Q2=dict(ok=False, nRegions=0, fails=[], worstRel=None, worstRegion=None,
                     degradationWidth=None, improve=False),
             Q2near=dict(ok=False, minRel=None, improve=False),
             Q3=dict(ok=False, cand=None, serial=None, improve=False),
             Q4=dict(ok=False, cand=None, serial=None, rel=None, delta=None),
             Q4a=dict(ok=False, error=None, boundary=0.02),
             Q5=dict(ok=False, markers=[], note=None),
             calibration=None, calibrationError=None, G0Eligible=False, classII=False)
    for key, value in unscorable(None).items():
        if isinstance(value, dict):
            g.setdefault(key, {}).update(value)
        else:
            g[key] = value
    ser = run['serial']
    model = None
    try:
        if len(ser) < 20 or any('quality' not in r for s in [run['final']] + ser[:20] for r in s['regions']):
            raise ValueError('19 calibration meshes and per-cell region qualities required')
        meshes = [stats_cells(s) for s in ser[:20]]
        model = calibrate(meshes[0], meshes[1:], min_cells=minCells)
        calibrated = score(stats_cells(run['final']), model)
    except ValueError as error:
        calibrated = unscorable(str(error))
        g['calibrationError'] = str(error)
    else:
        for key, value in legacy_class2(run, minCells).items():
            if isinstance(value, dict):
                g[key].update(value)
            else:
                g[key] = value
    for key, value in calibrated.items():
        if isinstance(value, dict):
            g[key].update(value)
        else:
            g[key] = value
    g['Q2']['worstRel'] = -g['worstLoss'] if g['worstLoss'] is not None else None
    if g['worstLoss'] is None:
        g['Q2'].update(degradationWidth=None, improve=False, worstRegion=None)
    losses = g['Q2']['losses'] or []
    boundaries = g['Q2']['boundary'] or []
    failed = [i for i, (loss, boundary) in enumerate(zip(losses, boundaries)) if loss > boundary + 1e-12]
    if model is not None:
        g['Q2']['nRegions'] = len(model['masks'])
        g['Q2']['fails'] = [model['masks'][i] for i in failed]
        if losses:
            g['Q2']['degradationWidth'] = max((DB_EDGES[min(5, b % 6 + 1)] if b % 6 < 5 else 99
                                             for i in failed for b in model['masks'][i]), default=0)
        near = [i for i, group in enumerate(model['masks']) if any(b % 6 <= 1 for b in group)]
        g['Q2near'].update(ok=all(losses[i] <= boundaries[i] + 1e-12 for i in near) if losses else False,
                           minRel=-max((losses[i] for i in near), default=0) if losses else None)
    if g['calibrationError'] is not None:
        g['calibration'] = model
        return g
    cand = run['final']
    error = abs(cand['complexityCarried'] - cand['complexityTarget']) / max(cand['complexityTarget'], 1e-300)
    g['Q4a'].update(ok=error <= 0.02, error=error)
    g['Q4'].update(ok=g['Q4a']['ok'] and g['Q4b']['ok'], rel=(cand['nv'] - ser[0]['nv']) / ser[0]['nv'])
    g['calibration'] = model
    g['G0Eligible'] = len(ser) >= 25 and g['reason'] is None
    g['classII'] = all(g[k]['ok'] for k in ('Q1', 'Q1near', 'Q2', 'Q3', 'Q4a', 'Q4b', 'Q5', 'tail'))
    return g


def class1(run, realistic=True, P=None):
    committed = all(s['committed'] for s in run['steps'])
    st = run['status']
    okStatus = st in ('COMPLETE', 'INCOMPLETE_QUALITY') if realistic else True
    return dict(ok=committed and okStatus, committed=committed, status=st, stop=run.get('stopReason', ''))


def class4(run, serialMMG, dim):
    steps = run['steps']
    out = {}
    # coverage steps (S-base / S-B4): steps until C = 0 counted from level 0
    nC = None
    for i, s in enumerate(steps):
        if i + 1 < len(steps) and steps[i + 1]['C'] == 0:
            nC = i + 1
            break
    if nC is None and run['finalC'] == 0:
        nC = len(steps)
    out['coverageSteps'] = nC
    out['batchesC'] = sum(1 for s in steps if s['kind'] == 'batchC')
    out['batchesD'] = sum(1 for s in steps if s['kind'] == 'batchD')
    ser = [s['largestWork'] / s['capLoad'] for s in steps if s.get('capLoad', 0) > 0]
    out['maxPieceOverCapLoad'] = max(ser) if ser else None
    out['serialisedStep'] = any(x > 2 for x in ser)
    out['tMMG'] = run['tMMGtotal']
    out['tMMGoverSerial'] = run['tMMGtotal'] / serialMMG if serialMMG else None
    out['critPath'] = run['critPathTotal']
    out['critOverSerial'] = run['critPathTotal'] / serialMMG if serialMMG else None
    out['repeatedWork'] = sum(s['workElems'] for s in steps) / run['neIn']
    out['placementOverhead'] = run['partition']['tPlacement'] / max(run['tRun'], 1e-9)
    tgtCov = 3 if dim == 2 else 4
    out['targets'] = dict(
        coverage=(nC is not None and nC <= tgtCov) if run['sched'] in ('sbase', 'sb4') else (out['batchesC'] <= 8),
        noSerialised=not out['serialisedStep'],
        mmgTime=out['tMMGoverSerial'] is not None and out['tMMGoverSerial'] <= (1.5 if dim == 2 else 1.6),
        placement=out['placementOverhead'] <= 0.05)
    out['nTargets'] = sum(out['targets'].values())
    return out


if __name__ == '__main__':
    for p in sys.argv[1:]:
        r = load(p)
        g = class2(r)
        print(p, r['status'], json.dumps({k: (v['ok'] if isinstance(v, dict) and 'ok' in v else v) for k, v in g.items()}) if g else '')
