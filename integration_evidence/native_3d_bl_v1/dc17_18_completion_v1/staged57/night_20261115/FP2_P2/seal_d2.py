"""Seal and verify the P2 first-collapse promotion decision from its selected receipt."""
from argparse import ArgumentParser
from pathlib import Path

from build_p2 import HERE, cut, verify_build
from run_shared import evaluate_first
from seal_d1 import verify_d1


def decision(build):
    if isinstance(build, int):
        build = HERE / f'build{build}'
    label = int(build.name.removeprefix('build'))
    status = verify_build(label)
    fingerprint = {key: status[key] for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    d1_path, d1 = verify_d1(build)
    assert d1['promote'], d1_path
    path = HERE / 'shared' / build.name / 'closure1/first-collapse' / 'case_summary.json'
    row = cut.read(path)
    assert all(row[key] == value for key, value in fingerprint.items()), path
    for artifact, digest in row['pins'].items():
        assert cut.sha(artifact) == digest, artifact
    part = row['subruns']['first-collapse']
    wait = part['wait']
    assert wait['terminal'] and cut.terminal(wait['controller'])
    if 'hard_checks' in part['switch_receipt']:
        raw, receipt, first, missed = evaluate_first(path.parent / 'first-collapse', 1, wait)
        assert str(raw) == part['raw'] and cut.sha(raw) == part['raw_sha256']
        assert receipt == part['switch_receipt'] and first == part['first_failure'] and missed == part['missed_comparison']
    else:
        receipt = part['switch_receipt']
        assert part['status'] == 'FAILED', path
        first, missed = part['first_failure'], part['missed_comparison']
    hard = receipt.get('hard_checks', {})
    no_findings = first is None and bool(hard) and all(value == 'passed' for value in hard.values())
    promote = no_findings and not receipt.get('wrapper_only')
    expected = ('OPEN' if missed else 'passed') if promote else ('FAILED' if first else 'OPEN')
    assert row['status'] == part['status'] == expected
    sources = (path, d1_path, Path(__file__).resolve(), build / 'build_status.json')
    pins = {**row['pins'], **{str(source): cut.sha(source) for source in sources}}
    return build / 'd2_promotion.json', {
        **fingerprint, 'status': expected, 'promote': promote, 'first_difference': first or missed,
        'shared_receipt': str(path), 'hard_checks': hard, 'observations': receipt, 'pins': pins,
    }


def verify_d2(build):
    path, record = decision(build)
    assert cut.read(path) == record, path
    return path, record


def seal_d2(build):
    path, record = decision(build)
    if path.exists():
        assert cut.read(path) == record, path
    else:
        pending = path.with_suffix('.tmp')
        cut.save(pending, record)
        pending.replace(path)
    return path, record


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    path, record = seal_d2(parser.parse_args().build)
    print('D2', record['status'], 'promotion', record['promote'], path)
    raise SystemExit(0 if record['promote'] else 75 if record['status'] == 'OPEN' else 1)
