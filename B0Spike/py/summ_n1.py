"""N1 summary: engine contract (rollback, acceptance, failure injection, memory, MMG cap, repeats)."""
import glob, json, os, re, sys

R = '/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-AdapNoExt/bae33fc3-5b05-48ef-a3e8-4d222dffb2fe/scratchpad/runs/b0'


def injections(sub):
    rows = []
    for d in sorted(glob.glob(f'{R}/n1/{sub}/*/*/')):
        case, tag = d.split('/')[-3], d.split('/')[-2]
        done = open(d + 'DONE').read().strip() if os.path.exists(d + 'DONE') else 'running'
        res = d + 'result.json'
        if not os.path.exists(res):
            rows.append((case, tag, done, 'no result', '', '', '', ''))
            continue
        r = json.load(open(res))
        st = r['steps']
        rolled = sum(s['nRolled'] for s in st)
        stages = sorted({re.search(r'stage (\w+)', f).group(1) for s in st for f in s['failures'] if 'stage ' in f})
        acc = [f for s in st for f in s['failures'] if f.startswith('acceptance')]
        rows.append((case, tag, done, r['status'], r.get('stopReason', ''), rolled, ','.join(stages),
                     'rolled back step' if acc else '', r['hash'], r['final']['nv']))
    return rows


def mmgcap():
    rows = []
    for d in sorted(glob.glob(f'{R}/n1/mmgcap/*/*/')):
        case, tag = d.split('/')[-3], d.split('/')[-2]
        log = open(d + 'log.txt').read() if os.path.exists(d + 'log.txt') else ''
        res = re.search(r'RESULT (.*)', log)
        hwm = re.search(r'Maximum resident set size \(kbytes\): (\d+)', log)
        ex = re.search(r'Exit status: (\d+)', log)
        sig = re.search(r'Command terminated by signal (\d+)', log)
        rows.append((case, tag, res.group(1) if res else ('signal ' + sig.group(1) if sig else 'no result'),
                     int(hwm.group(1)) / 1024 if hwm else None, ex.group(1) if ex else None))
    return rows


if __name__ == '__main__':
    what = sys.argv[1]
    if what == 'inj':
        for r in injections(sys.argv[2]):
            print('\t'.join(str(x) for x in r))
    elif what == 'cap':
        for r in mmgcap():
            print('\t'.join(str(x) for x in r))
