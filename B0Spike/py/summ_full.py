"""Summary of full runs (N3, N4, N5): Class I, II, IV per run."""
import glob, json, os, sys

R = '/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-AdapNoExt/bae33fc3-5b05-48ef-a3e8-4d222dffb2fe/scratchpad/runs/b0'
sys.path.insert(0, os.path.dirname(__file__))
import b0gates

SERIAL_MMG = {}


def serial_mmg(case):
    if case not in SERIAL_MMG:
        p = f'{R}/n0/{case}/ser_s0.json'
        SERIAL_MMG[case] = json.load(open(p))['tMMG'] if os.path.exists(p) else None
    return SERIAL_MMG[case]


def rows(base):
    out = []
    for f in sorted(glob.glob(f'{R}/{base}/*/*/result.json')):
        case, tag = f.split('/')[-3], f.split('/')[-2]
        r = json.load(open(f))
        g = b0gates.class2(r)
        c1 = b0gates.class1(r)
        c4 = b0gates.class4(r, serial_mmg(case), r.get('dim', 2))
        out.append(dict(case=case, tag=tag, r=r, g=g, c1=c1, c4=c4))
    return out


def fmt(x, n=3):
    if x is None:
        return '-'
    if isinstance(x, bool):
        return 'Y' if x else 'N'
    if isinstance(x, float):
        return f'{x:.{n}g}'
    return str(x)


def main(base):
    print('case\ttag\tstatus\tstop\tsteps\tnv/ser\tQ1\tQ1near(c/s)\tQ2(worstRel,width)\tQ3\tQ4rel\tQ5\tII\tcovSteps\tbatchC\tserialised\tMMG/ser\tcrit/ser\trepWork\tIVn\tunscorable reason')
    for x in rows(base):
        r, g, c4 = x['r'], x['g'], x['c4']
        if g is None:
            continue
        print('\t'.join([x['case'], x['tag'], r['status'], r.get('stopReason', '')[:24], str(len(r['steps'])),
                         f"{r['final']['nv']}/{fmt(r['serial'][0]['nv'] if r['serial'] else None)}",
                         fmt(g['Q1']['ok']),
                         f"{fmt(g['Q1near']['ok'])}({fmt(g['Q1near']['cand'])}/{fmt(g['Q1near']['serial'])})",
                         f"{fmt(g['Q2']['ok'])}({fmt(g['Q2']['worstRel'],2)},{fmt(g['Q2']['degradationWidth'])})",
                         f"{fmt(g['Q3']['ok'])}({fmt(g['Q3']['cand'],2)}/{fmt(g['Q3']['serial'],2)})",
                         f"{fmt(g['Q4']['ok'])}({fmt(g['Q4']['rel'],2)})", fmt(g['Q5']['ok']), fmt(g['classII']),
                         fmt(c4['coverageSteps']), fmt(c4['batchesC']), fmt(c4['serialisedStep']),
                         fmt(c4['tMMGoverSerial'], 2), fmt(c4['critOverSerial'], 2), fmt(c4['repeatedWork'], 2),
                         str(c4['nTargets']), fmt(g['reason'])]))


if __name__ == '__main__':
    main(sys.argv[1])
