"""Prepare a reusable cluster correctness checkpoint after rebuilding both executables."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PACK = 'integration_evidence/native_cluster_metric_comparison_v1'
PREPARED = 'integration_evidence/native_post_rebase_metric_v1/predict_history_v1/prepared_cases.json'


def closed(root, path):
    path = path.resolve()
    path.relative_to(root)
    assert path.is_file(), path
    return path


def sha(path):
    with path.open('rb') as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def source_files(root):
    return {str(path.relative_to(root)) for directory in ('Common', 'SU2_CFD', 'UnitTests')
            for path in (root / directory).rglob('*')
            if path.is_file() and path.suffix in ('.cpp', '.hpp', '.h', '.inl', '.tpp')}


def checkpoint(root, binary, test_binary):
    # Historical pins supply paths; new checkpoints capture their current bytes.
    files = set(json.loads(closed(root, root / PACK / 'correctness_pins.json').read_text()))
    files.update(source_files(root))
    files.add(PREPARED)
    files.add(PACK + '/correctness_pins.json')
    files.add(PACK + '/prepare_correctness.py')
    for row in json.loads(closed(root, root / PREPARED).read_text()):
        files.update(row['case'] + '/' + name for name in ('run.cfg', 'input.su2'))
    programs = {}
    for role, path in [('SU2_CFD', binary), ('test_driver', test_binary)]:
        path = closed(root, path)
        programs[role] = dict(path=str(path.relative_to(root)), sha256=sha(path))
        for parent in path.parents:
            metadata = parent / 'meson-info/intro-buildoptions.json'
            if metadata.is_file():
                closed(root, metadata)
                files.add(str(metadata.relative_to(root)))
                break
            if parent == root:
                break
    return dict(status='PREPARED_NOT_VALIDATED', created_utc=datetime.now(timezone.utc).isoformat(),
                scope='Current sources, suite inputs and built binaries; preparation is not a correctness PASS or proof of binary build provenance.',
                files_sha256={name: sha(closed(root, root / name)) for name in sorted(files)}, binaries=programs)


def verify(root, record):
    assert record['status'] == 'PREPARED_NOT_VALIDATED'
    captured_sources = {name for name in record['files_sha256']
                        if Path(name).parts[0] in ('Common', 'SU2_CFD', 'UnitTests')
                        and Path(name).suffix in ('.cpp', '.hpp', '.h', '.inl', '.tpp')}
    assert source_files(root) == captured_sources, 'Source/test file set changed after preparation'
    for name, digest in record['files_sha256'].items():
        assert sha(closed(root, root / name)) == digest, 'Source or suite changed after preparation: ' + name
    for role, row in record['binaries'].items():
        assert sha(closed(root, root / row['path'])) == row['sha256'], 'Binary changed after preparation: ' + role


def selftest():
    # Tiny fake files only: no MPI, CFD, compilation or scheduler submission.
    with tempfile.TemporaryDirectory(prefix='native-correctness-checkpoint-') as tmp:
        root = Path(tmp)
        for name, text in {PACK + '/correctness_pins.json': '{}', PREPARED: '[]',
                           'Common/new.cpp': 'new source', 'UnitTests/new.hpp': 'new test',
                           PACK + '/prepare_correctness.py': 'fake checkpoint helper',
                           'build/cfd': 'CFD bytes', 'build/test_driver': 'test bytes'}.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        record = checkpoint(root, root / 'build/cfd', root / 'build/test_driver')
        verify(root, record)
        assert 'Common/new.cpp' in record['files_sha256'] and 'UnitTests/new.hpp' in record['files_sha256']
        for name in ('Common/new.cpp', 'build/cfd'):
            path = root / name
            original = path.read_bytes()
            path.write_bytes(original + b' changed')
            try:
                verify(root, record)
            except AssertionError:
                pass
            else:
                raise AssertionError('Changed checkpoint was admitted: ' + name)
            # Explicit preparation admits intentional changes; it still declares no PASS.
            updated = checkpoint(root, root / 'build/cfd', root / 'build/test_driver')
            verify(root, updated)
            assert updated['status'] == 'PREPARED_NOT_VALIDATED'
            path.write_bytes(original)
        added = root / 'Common/added_after_preparation.cpp'
        added.write_text('new file')
        try:
            verify(root, record)
        except AssertionError:
            pass
        else:
            raise AssertionError('New source after preparation was admitted')
        verify(root, checkpoint(root, root / 'build/cfd', root / 'build/test_driver'))
        added.unlink()
        escaping = root / 'build/escaping'
        escaping.symlink_to('/bin/true')
        try:
            checkpoint(root, escaping, root / 'build/test_driver')
        except ValueError:
            pass
        else:
            raise AssertionError('Binary path escaped the repository')
    print('PASS: fake-file checkpoint creation, new source/test discovery, drift rejection, refresh and closed paths; no solver run.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-native/SU2_CFD/src/SU2_CFD')
    parser.add_argument('--test-binary', type=Path, default=ROOT / 'build-native/UnitTests/test_driver')
    parser.add_argument('--output', type=Path, default=ROOT / 'build-native/native_correctness_checkpoint.json')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        selftest()
    else:
        output = args.output.resolve()
        output.relative_to(ROOT)
        record = checkpoint(ROOT, args.binary, args.test_binary)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(record, indent=2) + '\n')
        print(output, 'PREPARED_NOT_VALIDATED;', len(record['files_sha256']), 'source/suite files; submit SGE correctness job next.')
