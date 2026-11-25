"""Run one FP2-P3 stage and seal its actual results."""
from argparse import ArgumentParser
from pathlib import Path
import subprocess
import sys

from checkpoint import HERE, build_dir, cut, require, seal, status_of, verify_pins
from p3_fixtures import run as run_fixtures

CONTROL = ('B0','B1','B2a','B2b','B2c')
SHORT = ('C1','C2','C3','C4')
PAIRS = ('C7','C5','C6')
SHARED = ('first-collapse','exact-c492','merit481','native2d','saved-current')
PYTHON = (sys.executable,'-B')


def command(script, *args):
    subprocess.run([*PYTHON, str(HERE/script), *map(str,args)], check=False)


def result(label, part):
    build=build_dir(label)
    if part in ('B0','B1','C1'):
        run_fixtures(label,part)
        return build/f'{part}.json'
    if part in ('B2a','C3','C4'):
        dependencies=('B0','B1') if part=='B2a' else ('B',)
        require(label,*dependencies)
        source=HERE/'scoped'/build.name/f'{part}.json'
        if not source.exists():
            command('run_scoped.py',build,part)
        return seal(label,part,{'scoped':source},dependencies)[0]
    if part in ('B2b','B2c',*PAIRS):
        dependencies=CONTROL[:CONTROL.index(part)] if part in CONTROL else ('B',*SHORT)
        require(label,*dependencies)
        source=HERE/'G5'/build.name/part/'case_summary.json'
        if not source.exists():
            command('run_full.py',label,part)
        return seal(label,part,{'workflow':source},dependencies)[0]
    if part=='C2':
        require(label,'B')
        sources={}
        for group in SHARED:
            source=HERE/'shared'/build.name/'closure1'/group/'case_summary.json'
            if not source.exists():
                command('run_shared.py',label,1,group)
            if source.exists():
                sources[group]=source
        assert len(sources)==len(SHARED), ('Missing C2 receipts',sorted(set(SHARED)-sources.keys()))
        return seal(label,part,sources,('B',))[0]
    assert part=='B'
    actual={key:build/f'{key}.json' for key in CONTROL}
    assert all(path.exists() for path in actual.values()), actual
    return seal(label,part,actual)[0]



def reselect_saved481(label):
    build = build_dir(label)
    top = build / 'B2a.json'
    original = HERE / 'scoped' / build.name / 'B2a.json'
    scoped = cut.read(original)
    verify_pins(scoped['pins'])
    corrected = HERE / 'shared' / build.name / 'closure0/saved481_repaired/case_summary.json'
    new = cut.read(corrected)
    verify_pins(new['pins'])
    assert scoped['parts']['saved481']['status'] == 'FAILED' and new['replaces']['classification'].startswith('R3')
    archive = top.with_name('B2a_before_saved481_repair.json')
    if not archive.exists():
        assert cut.read(top)['status'] == 'FAILED'
        top.rename(archive)
    scoped['parts']['saved481'] = {'status': new['status'], 'path': str(corrected),
                                  'first_difference': new['first_difference']}
    scoped['pins'].update({str(path): cut.sha(path) for path in (original, archive, corrected)})
    scoped['pins'].update(new['pins'])
    scoped['status'] = status_of(scoped['parts'].values())
    scoped['passed'] = scoped['status'] == 'passed'
    selected = original.with_name('B2a_saved481_reselected.json')
    if selected.exists():
        assert cut.read(selected) == scoped
    else:
        cut.save(selected, scoped)
    path, result = seal(label, 'B2a', {'scoped': selected}, ('B0', 'B1'))
    with (HERE.parent / 'progress.md').open('a') as stream:
        stream.write(f"\nFP2-P3 B2a reselected from the corrected saved481 attempt. {result['status']}. Receipt {path}.\n")
    return result['status']



def record(label,part):
    path=result(label,part)
    row=cut.read(path)
    message=f"FP2-P3 build{label} {part} {row['status']}. Evidence {path.relative_to(HERE)}."
    with (HERE.parent/'progress.md').open('a') as stream:
        stream.write(f'\n{message}\n')
    print(message,flush=True)
    return row['status']


if __name__=='__main__':
    parser=ArgumentParser(description=__doc__)
    parser.add_argument('build',type=int)
    parser.add_argument('part',choices=(*CONTROL,'B',*SHORT,*PAIRS))
    args=parser.parse_args()
    status=record(args.build,args.part)
    raise SystemExit(0 if status=='passed' else 1 if status=='FAILED' else 75)
