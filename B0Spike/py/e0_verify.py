"""G-R audit of the declared E0 outputs; no meshing, no MPI."""
import json
import pathlib
import sys


def main(root):
    errors = []
    def require(ok, text):
        if not ok:
            errors.append(text)
    for case in ['box3s', 'box3d', 'naca1', 'box2d']:
        for sched in ['sbase', 'sbatch']:
            paths = [root / 'r1' / f'r{r}' / case / sched / 'result.json' for r in [1, 2]]
            if any(not p.exists() for p in paths):
                errors.append(f'missing reproducibility pair {case}/{sched}'); continue
            runs = [json.loads(p.read_text()) for p in paths]
            require(all(isinstance(r['hash'], str) and 0 <= int(r['hash']) < 2**64 for r in runs), f'exact hash {case}/{sched}')
            require(runs[0]['hash'] == runs[1]['hash'], f'full state differs: {case}/{sched}')
            for r in runs:
                require(r.get('finalValid', False), f'invalid final state: {case}/{sched}')
                require(r['planMismatches'] == 0, f'plan mismatch: {case}/{sched}')
                require(all(s['committed'] for s in r['steps']), f'uninjected rollback: {case}/{sched}')
    files = sorted((root / 'inject').glob('r*/*/result.json'))
    require(len(files) == 128, f'injection series: expected 128 outputs, found {len(files)}')
    late = sorted((root / 'late_mpi').glob('*/result.json'))
    require(len(late) == 2, f'late MPI controls: expected 2 outputs, found {len(late)}')
    for p in files + late:
        r = json.loads(p.read_text())
        require(r.get('finalValid', False), f'invalid rollback state: {p}')
        require(r['planMismatches'] == 0, f'plan mismatch: {p}')
        require(r['injectionsObserved'] == 1, f'unobserved/repeated injection: {p}')
        target_step = int(p.parent.name.rsplit('_s', 1)[1])
        steps = [s for s in r['steps'] if s['step'] == target_step]
        require(len(steps) == 1 and (steps[0]['nRolled'] > 0 or not steps[0]['committed']), f'no rollback at selected step: {p}')
        require(r['status'] in ['COMPLETE', 'INCOMPLETE_COVERAGE', 'INCOMPLETE_QUALITY'], f'injection status: {p}')
        if '_commit_' in p.parent.name:
            require(r['status'] == 'INCOMPLETE_COVERAGE', f'commit injection expected INCOMPLETE_COVERAGE: {p}')
        require(not any(f.startswith('acceptance:') and 'injected' not in f for s in r['steps'] for f in s['failures']),
                f'unexpected acceptance failure after injection: {p}')
        require((p.parent / 'state_final.b0state').exists(), f'no final rollback state: {p}')
    report = dict(GR=not errors, errors=errors, reproducibilityPairs=8, injectionOutputs=len(files))
    (root / 'gr.json').write_text(json.dumps(report, indent=2) + '\n')
    print('G-R', report['GR'], '\n'.join(errors))
    return 0 if report['GR'] else 2


if __name__ == '__main__':
    raise SystemExit(main(pathlib.Path(sys.argv[1])))
