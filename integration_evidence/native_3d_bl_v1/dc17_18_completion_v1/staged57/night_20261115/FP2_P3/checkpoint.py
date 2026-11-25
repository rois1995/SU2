"""Seal the actual FP2-P3 campaign receipts and verify prerequisite closures."""
from pathlib import Path

from build_p3 import HERE, cut

ORDER = HERE / 'inputs/FP2_P3_CLOSURE_DEFAULT.md'
STATUSES = ('passed', 'FAILED', 'OPEN', 'not evaluated')
KEYS = ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')


def fingerprint(row):
    return {key: row[key] for key in KEYS}


def status_of(rows):
    statuses = {row['status'] for row in rows}
    assert statuses <= set(STATUSES)
    return 'FAILED' if 'FAILED' in statuses else 'passed' if statuses == {'passed'} else 'OPEN'


def build_dir(label):
    return HERE / f'build{label}'


def verify_pins(pins):
    for name, sha in pins.items():
        assert cut.sha(Path(name)) == sha, name


def pins_for(paths):
    return {str(path): cut.sha(path) for path in paths}


def require(label, *keys):
    build = build_dir(label)
    expected = fingerprint(cut.read(build / 'build_status.json'))
    paths = [build / f'{key}.json' for key in keys]
    for key, path in zip(keys, paths, strict=True):
        row = cut.read(path)
        assert row['part'] == key and fingerprint(row) == expected and row['status'] == 'passed', path
        verify_pins(row['pins'])
    return paths


def seal(label, part, children, prerequisites=()):
    """Seal selected child receipts with their exact digests and dependencies."""
    build = build_dir(label)
    receipt = build / 'build_status.json'
    expected = fingerprint(cut.read(receipt))
    dependencies = require(label, *prerequisites)
    rows = {}
    for name, source in children.items():
        path = Path(source)
        row = cut.read(path)
        candidates = {key: row[key] for key in KEYS if key in row}
        assert candidates and all(expected[key] == value for key, value in candidates.items()), path
        assert row['status'] in STATUSES, path
        verify_pins(row.get('pins', {}))
        rows[name] = {'status': row['status'], 'path': str(path), 'sha256': cut.sha(path)}
    status = status_of(rows.values())
    path = build / f'{part}.json'
    record = {**expected, 'part': part, 'status': status, 'passed': status == 'passed',
              'receipts': rows, 'pins': pins_for((ORDER, ORDER.with_suffix('.sha256'), receipt,
                                                build / 'source_pins.json', *dependencies,
                                                *map(Path, children.values())))}
    if path.exists():
        assert cut.read(path) == record, path
    else:
        pending = path.with_suffix('.tmp')
        cut.save(pending, record)
        pending.rename(path)
    return path, record


def host_preflight():
    """Use the existing real-host preparation for every heavy launch."""
    import subprocess
    import tempfile
    import time
    with tempfile.TemporaryDirectory(prefix='fp2-p3-host-') as temp:
        destination = Path(temp) / 'receipt.json'
        while True:
            check = subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(destination)],
                                   cwd=cut.REPO, capture_output=True, text=True)
            if check.returncode == 0:
                return
            details = check.stdout + check.stderr
            if 'assert not owned,owned' not in details:
                raise RuntimeError(details)
            print('FP2-P3 waiting for inherited host preflight.', flush=True)
            time.sleep(30)
