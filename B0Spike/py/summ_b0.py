"""B0 non-BL report tables (MPI_REMESH_PLAN.md 13.3): N0 noise, N1 contract, N2 placement, N3/N4 full runs by
scheduler, N5 carried vs background metric, classifier vs gate Q, MMG per-call cost. Usage: summ_b0.py [section]"""
import collections, glob, json, os, re, statistics as S, sys

sys.path.insert(0, os.path.dirname(__file__))
import b0gates, summ_full, summ_n2

R = summ_full.R


def n0():
    print('case\tnv\tsd_nv%\tedgeIn\tsd\tq1\tsd\tbadFrac\ttMMG range\tpeakMB')
    for c in ['naca0', 'naca1', 'box2d', 'box2x', 'box2i', 'cyl0', 'box3s', 'box3d', 'm60', 'm61']:
        rr = [json.load(open(f'{R}/n0/{c}/ser_s{i}.json')) for i in range(6)]
        g = lambda k: [r['stats'][k] for r in rr]
        nv, ei, q1, bf = g('nv'), g('edgeIn'), g('q1'), g('badFrac')
        t = [r['tMMG'] for r in rr]
        print(f"{c}\t{nv[0]:.0f}\t{100 * S.stdev(nv[1:]) / nv[0]:.2f}\t{ei[0]:.3f}\t{S.stdev(ei[1:]):.4f}\t{q1[0]:.3f}\t"
              f"{S.stdev(q1[1:]):.4f}\t{bf[0]:.2e}\t{min(t):.1f}-{max(t):.1f}\t{rr[0]['peakMB']:.0f}")


def n1():
    """Injection runs: status, rollback, hash of the final mesh vs the uninjected run of the same case/scheduler."""
    for sub in ['inj_r1', 'inj_r2']:
        agg = collections.Counter()
        for d in sorted(glob.glob(f'{R}/n1/{sub}/*/*/')):
            case, tag = d.split('/')[-3], d.split('/')[-2]
            rc = open(d + 'DONE').read().strip() if os.path.exists(d + 'DONE') else 'none'
            if not os.path.exists(d + 'result.json'):
                agg[('noresult', rc)] += 1
                print(sub, case, tag, 'rc', rc, 'NO RESULT')
                continue
            r = json.load(open(d + 'result.json'))
            sched = tag.split('_')[0]
            ref = f'{R}/n1/{sub}/{case}/{sched}_none/result.json'
            same = None
            if os.path.exists(ref) and not tag.endswith('none'):
                same = json.load(open(ref))['hash'] == r['hash']
            rolled = sum(s['nRolled'] for s in r['steps'])
            acc = sum(1 for s in r['steps'] if not s['committed'])
            agg[(r['status'], rc)] += 1
            print(f"{sub}\t{case}\t{tag}\trc {rc}\t{r['status']}\t{r.get('stopReason', '')[:30]}\trolledPieces {rolled}"
                  f"\trolledSteps {acc}\tsameAsNone {same}\tnv {r['final']['nv']:.0f}")
        print(sub, dict(agg))
    for sub in ['repeat', 'budget', 'mem']:
        for d in sorted(glob.glob(f'{R}/n1/{sub}/*/*/')):
            if not os.path.exists(d + 'result.json'):
                print(sub, d, 'NO RESULT')
                continue
            r = json.load(open(d + 'result.json'))
            un = sum(s['nUnadmitted'] for s in r['steps'])
            pr = max(max(s['vrankPredMB']) for s in r['steps'])
            me = max(max(s['vrankMeasMB']) for s in r['steps'])
            print(f"{sub}\t{d.split('/')[-3]}\t{d.split('/')[-2]}\t{r['status']}\thash {r['hash']:.0f}\tnv {r['final']['nv']:.0f}"
                  f"\tunadmitted {un}\tpredMaxMB {pr:.0f}\tmeasMaxMB {me:.0f}\ttRun {r['tRun']:.0f}")
    for d in sorted(glob.glob(f'{R}/n1/mmgcap/*/*/')):
        log = open(d + 'log.txt').read() if os.path.exists(d + 'log.txt') else ''
        res = re.search(r'RESULT (.*)', log)
        hwm = re.search(r'Maximum resident set size \(kbytes\): (\d+)', log)
        sig = re.search(r'terminated by signal (\d+)', log)
        print('mmgcap', d.split('/')[-3], d.split('/')[-2], res.group(1) if res else ('signal ' + sig.group(1) if sig else '-'),
              'HWM %.0f MB' % (int(hwm.group(1)) / 1024) if hwm else '')


def n2():
    rows = summ_n2.main('n2')
    grp = collections.defaultdict(list)
    for r in rows:
        grp[(r['case'], r['pl'], r['floor'])].append(r)
    print('case\tpl\tfloor\tPs\timb(max)\tcut faces\tviolArea\tmeanCf\tdNear(Q1 near, mean)\tnearQ1rel(mean)\tdepthP95/ser\tdepthP99/ser')
    for k in sorted(grp):
        xs = grp[k]
        m = lambda h: S.mean([x[h] for x in xs if x.get(h) is not None]) if any(x.get(h) is not None for x in xs) else float('nan')
        print(f"{k[0]}\t{k[1]}\t{k[2]}\t{','.join(str(x['P']) for x in xs)}\t{max(x['imb'] for x in xs):.2f}\t"
              f"{'/'.join(str(x['cut']) for x in xs)}\t{m('violArea'):.3f}\t{m('meanCf'):.2f}\t{m('dNear'):.3f}\t"
              f"{m('nearQ1rel'):.2f}\t{m('depthP95'):.2f}/{m('serDepthP95'):.2f}\t{m('depthP99'):.2f}/{m('serDepthP99'):.2f}")
    # FLOOR vs NONE: paired by case, placement, P
    win = collections.Counter()
    for r in rows:
        if r['floor'] != 1:
            continue
        o = [x for x in rows if x['case'] == r['case'] and x['pl'] == r['pl'] and x['P'] == r['P'] and x['floor'] == 0]
        if not o:
            continue
        o = o[0]
        win['Q1near FLOOR better' if r['dNear'] > o['dNear'] else 'Q1near NONE better'] += 1
        if r.get('nearQ1rel') is not None and o.get('nearQ1rel') is not None:
            win['Q2near FLOOR better' if r['nearQ1rel'] > o['nearQ1rel'] else 'Q2near NONE better'] += 1
    print('FLOOR vs NONE (paired):', dict(win))
    # macro-orientation predictive value
    mv = [r['mv'] for r in rows if r['mv'] and r['mv'].get('nViol', 0) > 0 and r['mv'].get('nLateral', 0) > 0]
    if mv:
        print('macro: runs with both classes', len(mv), 'mean depth viol %.2f lateral %.2f; frac deep viol %.2f lateral %.2f' % (
            S.mean(x['meanDepthViol'] for x in mv), S.mean(x['meanDepthLateral'] for x in mv),
            S.mean(x['fracDeepViol'] for x in mv), S.mean(x['fracDeepLateral'] for x in mv)))


def value_range(values, n=2, median=False):
    values = [v for v in values if v is not None]
    if not values:
        return '-'
    bounds = f'[{min(values):.{n}f},{max(values):.{n}f}]'
    return f'{S.median(values):.{n}f} {bounds}' if median else bounds


def full(bases=('n3', 'n4')):
    rows = []
    for b in bases:
        rows += summ_full.rows(b)
    grp = collections.defaultdict(list)
    for x in rows:
        r = x['r']
        grp[(r['dim'], r['sched'], r['P'], r['L'])].append(x)
    print('dim\tsched\tP\tL\truns\tstatus\tClassII\tQ1\tQ1near\tQ2\tQ3\tQ4\tQ2 worstRel med [min,max]\tnv rel [min,max]\tcrit/ser\tMMG/ser\tunscorable reasons')
    for k in sorted(grp):
        xs = grp[k]
        st = collections.Counter(x['r']['status'] for x in xs)
        g = lambda q: sum(x['g'][q]['ok'] for x in xs)
        q2 = [x['g']['Q2']['worstRel'] for x in xs]
        nv = [x['g']['Q4']['rel'] for x in xs]
        cr = [x['c4']['critOverSerial'] for x in xs]
        mm = [x['c4']['tMMGoverSerial'] for x in xs]
        reasons = collections.Counter(x['g']['reason'] for x in xs if x['g']['reason'])
        print(f"{k[0]}\t{k[1]}\t{k[2]}\t{k[3]}\t{len(xs)}\t{dict(st)}\t{sum(x['g']['classII'] for x in xs)}\t{g('Q1')}\t{g('Q1near')}\t"
              f"{g('Q2')}\t{g('Q3')}\t{g('Q4')}\t{value_range(q2, median=True)}\t{value_range(nv, 3)}\t"
              f"{value_range(cr)}\t{value_range(mm)}\t{dict(reasons)}")


def n5():
    print('case\tsched\tP\tcarried: status nv Q2worst Q3 bad\tbackground: status nv Q2worst Q3 bad\thash equal')
    for f in sorted(glob.glob(f'{R}/n5/*/*/result.json')):
        case, tag = f.split('/')[-3], f.split('/')[-2]
        rb = json.load(open(f))
        sched, P, L = rb['sched'], int(rb['P']), int(rb['L'])
        cf = f'{R}/n3/{case}/Pb_P{P}_L{L}/result.json' if sched == 'sbase' else f'{R}/n4/{case}/Pb_sbatch_P{P}/result.json'
        if not os.path.exists(cf):
            print(case, tag, 'no carried counterpart', cf)
            continue
        rc = json.load(open(cf))
        out = []
        for r in (rc, rb):
            g = b0gates.class2(r)
            out.append(f"{r['status']} {r['final']['nv']:.0f} {summ_full.fmt(g['Q2']['worstRel'], 2)} {r['final']['badFrac']:.2e} {r['final']['nBad']:.0f} unscorable={summ_full.fmt(g['reason'])}")
        print(f"{case}\t{sched}\t{P}\t{out[0]}\t{out[1]}\t{rc['hash'] == rb['hash']}")


def classifier():
    conf = collections.Counter()
    unscorable = collections.Counter()
    for b in ('n3', 'n4', 'n3adv', 'n5'):
        for x in summ_full.rows(b):
            r, g = x['r'], x['g']
            if g['reason']:
                unscorable[g['reason']] += 1
            conf[(r['finalD'] == 0, g['Q2']['ok'] and g['Q3']['ok'] and g['Q1near']['ok'])] += 1
    print('(classifier D empty, gate Q passes): count', dict(conf))
    print('unscorable reasons:', dict(unscorable))


if __name__ == '__main__':
    what = sys.argv[1:] or ['n0', 'n1', 'n2', 'full', 'n5', 'classifier']
    for w in what:
        print(f'===== {w}')
        globals()[w]()
