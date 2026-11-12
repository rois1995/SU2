#!/usr/bin/env python3
"""Small negative controls for new transport checks; no Lean acceptance authority."""
import io
import os
import tarfile
import tempfile
from pathlib import Path
from common import limits, require, unpack

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    archive = root / 'fixture.tar.gz'
    with tarfile.open(archive, 'w:gz') as tar:
        entry = tarfile.TarInfo('safe.txt')
        entry.size = 2
        tar.addfile(entry, io.BytesIO(b'OK'))
    unpack(archive, root / 'valid')
    assert (root / 'valid/safe.txt').read_bytes() == b'OK'
    for name, target in [('../escaped', None), ('link', '../../escaped')]:
        with tarfile.open(archive, 'w:gz') as tar:
            entry = tarfile.TarInfo(name)
            if target:
                entry.type = tarfile.SYMTYPE
                entry.linkname = target
            tar.addfile(entry)
        try:
            unpack(archive, root / 'invalid')
        except tarfile.FilterError:
            pass
        else:
            raise AssertionError('Unsafe archive accepted')
    assert not (root / 'escaped').exists()
    try:
        require(False, 'negative validation fixture')
    except ValueError:
        pass
    else:
        raise AssertionError('Validation did not fail closed')
os.environ.update(LEAN_PATH='/unapproved/cache', LEAN='/unapproved/compiler',
                  LAKE_HOME='/unapproved/lake', GIT_DIR='/unapproved/repository')
limits()
assert not any(name in os.environ for name in ['LEAN_PATH', 'LEAN', 'LAKE_HOME', 'GIT_DIR'])
assert os.environ['LAKE_NO_CACHE'] == '1'
print('Transport and environment negative controls passed.')
