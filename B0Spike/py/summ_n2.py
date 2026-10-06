"""N2 summary: level-0 cut placement P0/Pb/P3 x FLOOR/NONE (B_DESIGN_EXPLORATION.md 6.1)."""
import glob, json, os, statistics, sys

R = '/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-AdapNoExt/bae33fc3-5b05-48ef-a3e8-4d222dffb2fe/scratchpad/runs/b0'
sys.path.insert(0, os.path.dirname(__file__))
import b0gates


def level0(run):
    """Level-0 statistics against the serial meshes evaluated with the same cut faces."""
    c = run['level0']
    ser = run['level0Serial']
    s0, ren = ser[0], ser[1:]
    out = dict(nv=c['nv'], edgeIn=c['edgeIn'], edgeInNear=c['edgeInNear'], serEdgeInNear=s0['edgeInNear'],
               dNear=c['edgeInNear'] - s0['edgeInNear'],
               sdNear=b0gates.sd([s['edgeInNear'] for s in ren]),
               q1=c['q1'], serQ1=s0['q1'], badFrac=c['badFrac'], serBad=s0['badFrac'],
               depthP95=c['depthP95'], depthP99=c['depthP99'], depthMax=c['depthMax'],
               serDepthP95=s0['depthP95'], serDepthP99=s0['depthP99'], serDepthMax=s0['depthMax'],
               nFacesDeg=c['nFacesDegraded'], serFacesDeg=s0['nFacesDegraded'])
    # near-cut matched regions (db 0,1): worst relative q1
    worst = None
    for i, r in enumerate(c['regions']):
        rs = s0['regions'][i]
        if r['db'] > 1 or r['nCell'] < 50 or rs['nCell'] < 50:
            continue
        rel = (r['q1'] - rs['q1']) / max(rs['q1'], 1e-12)
        worst = rel if worst is None else min(worst, rel)
    out['nearQ1rel'] = worst
    return out


def main(base='n2'):
    rows = []
    for f in sorted(glob.glob(f'{R}/{base}/*/*/result.json')):
        case = f.split('/')[-3]
        tag = f.split('/')[-2]
        try:
            r = json.load(open(f))
        except Exception as e:
            print('bad', f, e)
            continue
        p = r['partition']
        l0 = level0(r) if 'level0' in r else {}
        st = r['steps'][0]
        rows.append(dict(case=case, tag=tag, pl=r['placement'], P=r['P'], floor=r['floor'], a=r['a'], b=r['b'],
                         imb=p['imbalanceWork'], cut=p['cutFaces'], aniso=p['nAniso'], areaCut=p['areaCut'],
                         fracAnisoArea=p['areaAniso'] / max(p['areaCut'], 1e-30),
                         viol=p['nViol'], unres=p['nUnres'], violArea=p['areaViol'] / max(p['areaAniso'], 1e-30) if p['areaAniso'] else 0,
                         run=p['longestViolRun'], meanCf=p['meanCf'], p95Cf=p['p95Cf'], mv=r.get('macroVsDepth', {}),
                         rolled=st['nRolled'], excl=st['nExcluded'], low=st['lowFailures'], tMMG=st['tMMGsum'],
                         crit=st['critPath'], tPlace=p['tPlacement'], **l0))
    return rows


if __name__ == '__main__':
    base = sys.argv[1] if len(sys.argv) > 1 else 'n2'
    rows = main(base)
    hdr = ['case', 'pl', 'P', 'floor', 'a', 'b', 'imb', 'cut', 'aniso', 'fracAnisoArea', 'viol', 'unres', 'violArea', 'run',
           'meanCf', 'p95Cf', 'rolled', 'excl', 'low', 'dNear', 'sdNear', 'nearQ1rel', 'depthP95', 'serDepthP95',
           'depthP99', 'serDepthP99', 'depthMax', 'serDepthMax', 'nFacesDeg', 'serFacesDeg', 'tMMG', 'crit']
    print('\t'.join(hdr))
    for r in rows:
        print('\t'.join(('%.4g' % r[h]) if isinstance(r.get(h), float) else str(r.get(h)) for h in hdr))
