"""E0 frozen-mask rank calibration. Stdlib only; cells come from the spike's shared metrics."""
import math
import random
import statistics


def quantile(values, p=0.01):
    values = sorted(values)
    if not values:
        raise ValueError('empty scored region')
    return values[math.floor(p * (len(values) - 1))]


def scale(values, floor):
    return max(floor, statistics.stdev(values) if len(values) > 1 else 0.0)


def cell_bin(c):
    if (type(c['ar']) is not int or not 0 <= c['ar'] < 4 or
            type(c['db']) is not int or not 0 <= c['db'] < 6):
        raise ValueError('cell outside the 24-bin model')
    if 'class' in c and (type(c['class']) is not int or not 0 <= c['class'] < 8 or c['class'] % 4 != c['ar']):
        raise ValueError('cell outside the 8-class classifier')
    return 6 * c['ar'] + c['db']


def valid_quality(q):
    return isinstance(q, (int, float)) and not isinstance(q, bool) and math.isfinite(q) and q > 0


def unscorable(reason):
    return dict(Q2=dict(ok=False, reason=reason, losses=None, boundary=None, k=None, s=None),
                tail=dict(ok=False, reason=reason, rates=None, boundary=None, k=None),
                Q4b=dict(ok=False, reason=reason, error=None, boundary=None),
                worstLoss=None, reason=reason)


def pools(mesh):
    if type(mesh['nv']) is not int or mesh['nv'] <= 0:
        raise ValueError('mesh vertex count must be positive')
    if mesh.get('ne', len(mesh['cells'])) != len(mesh['cells']):
        raise ValueError('cell population does not match ne')
    result = {i: [] for i in range(24)}
    for c in mesh['cells']:
        b = cell_bin(c)
        if not valid_quality(c['q']):
            raise ValueError('non-finite or non-positive cell quality')
        result[b].append(c['q'])
    return result


def merge_masks(meshes, min_cells=300):
    """Freeze counts-only merges before scoring; distance-neighbour first, no cells dropped.

    All 24 bins, including those empty throughout calibration, use this rule.
    An AR class too small even after all distance bins merge joins the nearest AR
    class (lower AR wins ties). This fallback is declared here, not candidate-tuned.
    """
    ps = [pools(m) for m in meshes]
    groups = [[i] for i in range(24)]
    def count(g):
        return min(sum(len(p[i]) for i in g) for p in ps)
    while len(groups) > 1:
        small = next((i for i, g in enumerate(groups) if count(g) < min_cells), None)
        if small is None:
            break
        g = groups[small]
        ar = min(g) // 6
        candidates = [j for j in range(len(groups)) if j != small]
        def key(j):
            h = groups[j]
            return (min(h) // 6 != ar, abs(min(h) // 6 - ar),
                    min(abs(i % 6 - k % 6) for i in g for k in h), min(h))
        other = min(candidates, key=key)
        groups[other] = sorted(groups[other] + g)
        groups.pop(small)
        groups.sort(key=min)
    if not groups or any(count(g) < min_cells for g in groups):
        raise ValueError('mask cannot meet the 300-cell minimum; more cells are required')
    return groups


def regional(mesh, masks):
    if any(not g for g in masks) or sorted(i for g in masks for i in g) != list(range(24)):
        raise ValueError('frozen masks must partition all 24 bins exactly once')
    p = pools(mesh)
    return [[q for i in g for q in p[i]] for g in masks]


def calibrate(reference, calibration, masks=None, min_cells=300):
    """Construct masks from reference/calibration counts only, before candidate scoring."""
    if len(calibration) != 19:
        raise ValueError('E0 requires exactly 19 calibration meshes')
    if masks is None:
        masks = merge_masks([reference] + calibration, min_cells)
    ref = regional(reference, masks)
    null = [regional(m, masks) for m in calibration]
    if any(len(q) < min_cells for regions in [ref] + null for q in regions):
        raise ValueError('calibration region below minimum')
    qref = [quantile(q) for q in ref]
    if any(q <= 0 for q in qref):
        raise ValueError('reference p1 must be positive')
    losses = [[(qref[r] - quantile(m[r])) / qref[r] for r in range(len(ref))] for m in null]
    s = [scale([x[r] for x in losses], 0.005) for r in range(len(ref))]
    k = max(0.0, max((x[r] - 0.05) / s[r] for x in losses for r in range(len(ref))))
    tref = [sum(q < 0.5 * qref[r] for q in ref[r]) / len(ref[r]) for r in range(len(ref))]
    tails = [[sum(q < 0.5 * qref[r] for q in m[r]) / len(m[r]) for r in range(len(ref))] for m in null]
    st = [scale([x[r] for x in tails], 1e-4) for r in range(len(ref))]
    kt = max(0.0, max((x[r] - 1.25 * tref[r] - 1e-4) / st[r] for x in tails for r in range(len(ref))))
    errors = [abs(m['nv'] - reference['nv']) / reference['nv'] for m in calibration]
    return dict(masks=masks, minCells=min_cells, qref=qref, s=s, k=k,
                q2Boundary=[0.05 + k * v for v in s], tref=tref, st=st, kt=kt,
                tailBoundary=[1.25 * t + 1e-4 + kt * v for t, v in zip(tref, st)],
                vertexBoundary=0.02 + max(0.0, max(errors) - 0.02), nv=reference['nv'])


def achieved_loss(candidate, model):
    """Window tuning uses regional loss only; no gate threshold or pass/fail is inspected."""
    try:
        regions = regional(candidate, model['masks'])
    except ValueError:
        return None
    if any(len(q) < model['minCells'] for q in regions):
        return None
    return max((model['qref'][r] - quantile(q)) / model['qref'][r] for r, q in enumerate(regions))


def score(candidate, model):
    try:
        regions = regional(candidate, model['masks'])
    except ValueError as error:
        return unscorable(str(error))
    if any(len(q) < model['minCells'] for q in regions):
        return unscorable('frozen region below minimum')
    loss = [(model['qref'][r] - quantile(q)) / model['qref'][r] for r, q in enumerate(regions)]
    tails = [sum(q < 0.5 * model['qref'][r] for q in qs) / len(qs) for r, qs in enumerate(regions)]
    err = abs(candidate['nv'] - model['nv']) / model['nv']
    # Tiny comparison tolerance only covers floating-point roundoff at a boundary.
    return dict(Q2=dict(ok=all(x <= b + 1e-12 for x, b in zip(loss, model['q2Boundary'])), reason=None,
                        losses=loss, boundary=model['q2Boundary'], k=model['k'], s=model['s']),
                tail=dict(ok=all(x <= b + 1e-12 for x, b in zip(tails, model['tailBoundary'])), reason=None,
                          rates=tails, boundary=model['tailBoundary'], k=model['kt']),
                Q4b=dict(ok=err <= model['vertexBoundary'] + 1e-12, reason=None, error=err, boundary=model['vertexBoundary']),
                worstLoss=max(loss), reason=None)


def stats_cells(st):
    bins = [cell_bin(r) for r in st['regions']]
    if sorted(bins) != list(range(24)):
        raise ValueError('statistics must contain all 24 bins exactly once')
    if any(r['nCell'] != len(r['quality']) for r in st['regions']):
        raise ValueError('region quality population does not match nCell')
    mesh = dict(nv=st['nv'], cells=[dict(ar=r['ar'], db=r['db'], q=q) for r in st['regions'] for q in r['quality']])
    if 'ne' in st:
        mesh['ne'] = st['ne']
    pools(mesh)
    return mesh


def bootstrap(values, repetitions=2000, seed=1701):
    """Run-level cluster bootstrap of already aggregated per-run mask fractions."""
    if not values:
        return None
    rng = random.Random(seed)
    means = sorted(statistics.mean(rng.choices(values, k=len(values))) for _ in range(repetitions))
    return [means[int(0.05 * repetitions)], means[min(repetitions - 1, int(0.95 * repetitions))]]


def attribution(artifact, model):
    lookup = {i: r for r, group in enumerate(model['masks']) for i in group}
    def rates(mesh):
        acc = {}
        for c in mesh['cells']:
            try:
                r = lookup.get(cell_bin(c))
            except ValueError:
                r = None
            if r is None:
                key = ('invalid', -1, -1)
            elif c['distance'] >= 8 or c['face'] < 0:
                key = ('unattributed', -1, -1)
            else:
                f = artifact['faces'][c['face']]
                key = (['level0', 'cut', 'patch'][f['kind']], f['step'], c['db'])
            n, bad = acc.get(key, (0, 0))
            acc[key] = (n + 1, bad + (r is None or not valid_quality(c['q']) or c['q'] < model['qref'][r]))
        return acc
    candidate = rates(artifact['candidate'])
    serial = [rates(m) for m in artifact['serial']]
    rows = []
    for key in sorted(set(candidate).union(*(set(m) for m in serial))):
        n, bad = candidate.get(key, (0, 0))
        ns, bs = serial[0].get(key, (0, 0))
        # Missing populations remain explicit, never become a zero-rate comparison.
        null = [b / count - bs / ns for m in serial[1:20] for count, b in [m.get(key, (0, 0))] if count and ns]
        excess = bad / n - bs / ns if n and ns else None
        rows.append(dict(kind=key[0], step=key[1], db=key[2], n=n, serialN=ns,
                         excess=excess, serialNullExcess=null,
                         null95=sorted(null)[math.ceil(0.95 * len(null)) - 1] if null else None,
                         uncertainty90=bootstrap(null), modelMasks=model['masks']))
    return rows
