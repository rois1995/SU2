"""Verify actual ParaView scalar fields against the paired double restart."""
import argparse, hashlib, json, sys
from pathlib import Path
import numpy as np


def agree(vtu, restart):
    return np.isfinite(vtu).all() and np.isfinite(restart).all() and np.array_equal(
        vtu, np.asarray(restart, dtype=np.float32).astype(float))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path,nargs='?')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--self-check',action='store_true')
    args=parser.parse_args()
    if args.self_check:
        original=np.array([1.000000001,2.])
        assert agree(original.astype(np.float32).astype(float),original)
        assert not agree(np.array([2.,1.]),original)
        assert not agree(np.array([np.nan,2.]),original)
        print('Scalar-field pairing self-check PASS');return
    if not args.directory or not args.output:parser.error('directory and --output are required')
    source=Path(__file__).resolve().parent.parent
    sys.path.insert(0,str(source/'TestCases/adaptation/capability'))
    import capcheck
    runtime=args.directory/'evidence.json'
    initial=runtime.read_bytes();data=json.loads(initial)
    if data.get('current_run') or [r['ranks'] for r in data['runs']]!=[1,2,4] or not all(r['verified'] for r in data['runs']):
        raise ValueError('All production runtimes must finish successfully before this check')
    checks=[];hashes={}
    for run in data['runs']:
        if sorted(map(int,run['outputs']))!=list(range(4)):raise ValueError('Missing production output cycle')
        for cycle,item in run['outputs'].items():
            for kind in ('restart','mesh','vtu'):
                path=Path(item[kind]);digest=hashlib.sha256(path.read_bytes()).hexdigest()
                if digest!=item[kind+'_sha256']:raise ValueError('Runtime output changed: '+str(path))
                hashes[str(path)]=digest
            rp,rf,_=capcheck.read_restart(item['restart']);vp,vf,_=capcheck.read_vtu(item['vtu'])
            if not agree(vp[:,:2],rp) or not (vp[:,2]==0).all():raise ValueError('VTU coordinates disagree with restart')
            required={'Density','Energy','Pressure','Temperature','Mach','Metric_XX','Metric_XY','Metric_YY'}
            if not required<=set(vf) or not set(vf)<=set(rf):raise ValueError('Missing/unknown plotted scalar fields')
            for name,values in vf.items():
                if not agree(values,rf[name]):raise ValueError('VTU/restart scalar mismatch: '+name)
            checks.append(dict(ranks=run['ranks'],cycle=int(cycle),points=len(rp),scalar_fields=sorted(vf)))
    if runtime.read_bytes()!=initial or any(hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h for p,h in hashes.items()):
        raise ValueError('Inputs changed during field comparison')
    record=dict(verified=True,cases=checks,input_files_sha256=hashes,
                runtime_evidence_sha256=hashlib.sha256(initial).hexdigest(),
                checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                reader_sha256=hashlib.sha256(Path(capcheck.__file__).read_bytes()).hexdigest(),
                scope='Every plotted scalar and XY coordinate equals the paired double restart rounded to Float32; vector fields are outside this reader/check')
    with args.output.open('x') as stream:json.dump(record,stream,indent=2);stream.write('\n')
    print(f'Actual scalar output pairing PASS: {len(checks)} rank/cycle cases')


if __name__=='__main__':main()
