"""Pin a same-binary reuse diagnostic and preserve both previously tested executables."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / 'integration_evidence/native_cluster_reuse_diagnostic_v1'
SHARED = 'integration_evidence/native_cluster_balance_profile_v1'
PREVIOUS_PACKAGE = ROOT / 'integration_evidence/native_cluster_reconstruction_reuse_v1'
spec = importlib.util.spec_from_file_location('reuse_previous_prepare', PREVIOUS_PACKAGE / 'prepare.py')
base = importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
base.PACK = PACK
sha, closed = base.sha, base.closed
CONTROL = base.CONTROL
CANDIDATE = base.CANDIDATE
ARCHIVED = ROOT / 'build-native/reconstruction-previous/test_driver'
ARCHIVED_HASH = '13751201942dfcf9731bc377e45a69b0793ce82f0e2abe0b72edfbcee0902626'
CHECKPOINT = ROOT / 'build-native/reuse_diagnostic_checkpoint.json'


def preserve():
    base.preserve()
    ARCHIVED.resolve().relative_to(ROOT)
    if not ARCHIVED.exists():
        source = closed(CANDIDATE)
        if sha(source) != ARCHIVED_HASH:
            raise ValueError('Preserve the tested job582333/582334 candidate BEFORE rebuilding; expected '+ARCHIVED_HASH)
        ARCHIVED.parent.mkdir(parents=True, exist_ok=True)
        with source.open('rb') as incoming, ARCHIVED.open('xb') as outgoing:
            shutil.copyfileobj(incoming, outgoing)
        shutil.copymode(source, ARCHIVED)
    if sha(closed(ARCHIVED)) != ARCHIVED_HASH:
        raise ValueError('Archived candidate differs; recover the tested executable without overwriting evidence')
    print('Previously tested candidate retained:', ARCHIVED)


def pins():
    result = base.pins()
    for p in PREVIOUS_PACKAGE.iterdir():
        if p.is_file(): result[str(p.relative_to(ROOT))] = sha(closed(p))
    return result


def verify(record):
    if record['status'] != 'PREPARED_NOT_VALIDATED' or record['files_sha256'] != pins():
        raise ValueError('Diagnostic source/test/tool/input checkpoint changed; rebuild and prepare again')
    for role, info in record['binaries'].items():
        if sha(closed(ROOT / info['path'])) != info['sha256']: raise ValueError(role+' executable changed')
    expected = json.loads((PACK / 'control.json').read_text())['test_driver_sha256']
    if record['binaries']['control']['sha256'] != expected or record['binaries']['archived']['sha256'] != ARCHIVED_HASH:
        raise ValueError('Previously tested executable identity differs')


def prepare():
    CHECKPOINT.resolve().relative_to(ROOT)
    record = dict(status='PREPARED_NOT_VALIDATED', files_sha256=pins(),
                  binaries={role:dict(path=str(closed(path).relative_to(ROOT)), sha256=sha(path))
                            for role,path in (('control',CONTROL), ('archived',ARCHIVED), ('profile',CANDIDATE))},
                  build_options=json.loads(closed(ROOT / 'build-native/meson-info/intro-buildoptions.json').read_text()),
                  scope='Frozen diagnostic, not a correctness/performance PASS. Retain the build log; hashes alone do not prove build provenance.')
    if record['binaries']['profile']['sha256'] in (record['binaries']['control']['sha256'], ARCHIVED_HASH):
        raise ValueError('Diagnostic executable was not rebuilt')
    verify(record)
    with CHECKPOINT.open('x') as handle: handle.write(json.dumps(record, indent=2)+'\n')
    print('Prepared:', CHECKPOINT, '\nNo solver or cluster job started.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preserve-previous', action='store_true')
    args = parser.parse_args()
    preserve() if args.preserve_previous else prepare()
