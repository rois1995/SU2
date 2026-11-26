"""Use the frozen primary order pins once in the first-collapse worker."""

from argparse import ArgumentParser
from pathlib import Path
from unittest.mock import patch

import run_shared as shared
from checkpoint import build_dir, fingerprint, seal

CUT = shared.cut
SELF = Path(__file__).resolve()
ORIGINAL = shared.SELF
HASH = shared.hashes
PART = 'first-collapse'


def supplemental(build, paths):
    primary = CUT.read(build/'source_pins.json')
    extra = HASH(paths)
    duplicates = primary.keys() & extra.keys()
    assert duplicates == {str(shared.ORDER), str(shared.ORDER.with_suffix('.sha256'))}
    assert all(primary[name] == extra.pop(name) for name in duplicates)
    return extra


def worker(build, attempt):
    def pins(paths):
        return supplemental(build, (ORIGINAL, *paths))

    with patch.multiple(shared, SELF=SELF, hashes=pins):
        shared.first(attempt, build, 1)


def repair(label):
    build = build_dir(label)
    current = shared.verify_build(label)
    root = shared.HERE/'shared'/build.name/'closure1'
    original = root/PART/'case_summary.json'
    failed = CUT.read(original)
    shared.verified_pins(failed)
    old_attempt = original.parent/PART/'attempt1'
    old_log = old_attempt/'controller_outer.log'
    assert failed['failure_kind'] == 'measurement' and not (old_attempt/'case/status.json').exists()
    assert 'assert not (pins.keys() & extra.keys())' in old_log.read_text()
    assert CUT.terminal(CUT.read(old_attempt/'controller_wait.json')['controller'])

    destination = root/'first_collapse_repaired'
    summary = destination/'case_summary.json'
    if not summary.is_file():
        with patch.object(shared, 'SELF', SELF):
            check = shared.job(destination/PART,label,1,PART,PART)
        CUT.save(summary, {
            **fingerprint(current),'status':check['status'],'first_difference':check['first_failure'],
            'failure_kind':'result' if check['status']=='FAILED' else None,
            'environment':shared.environment(1),'closure_setting':'absent','pair':None,
            'subruns':{PART:check},'pins':{
                **check['pins'],**HASH((original,old_log,SELF,ORIGINAL,old_attempt/'controller_wait.json'))}})
    renewed = build/'C2.json'
    archive = renewed.with_name('C2_before_first_collapse_repair.json')
    if not archive.is_file():
        assert CUT.read(renewed)['receipts'][PART]['status']=='FAILED'
        renewed.rename(archive)
    children = {group:Path(row['path']) for group,row in CUT.read(archive)['receipts'].items()}
    children[PART] = summary
    _, row = seal(label,'C2',children,('B',))
    return row


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('closure', type=int, nargs='?', choices=(1,))
    parser.add_argument('group', nargs='?', choices=(PART,))
    parser.add_argument('--worker', choices=(PART,))
    parser.add_argument('--attempt-dir', type=Path)
    parser.add_argument('--seconds', type=int)
    args = parser.parse_args()
    if args.worker:
        assert args.closure==1 and args.group==PART and args.attempt_dir
        worker(build_dir(args.build),args.attempt_dir)
    else:
        row=repair(args.build)
        print('C2 reselected',row['status'],flush=True)
        raise SystemExit({'passed':0,'FAILED':1,'OPEN':75}[row['status']])
