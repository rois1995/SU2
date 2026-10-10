"""Preserve job582199's validated executable before rebuilding the reuse candidate."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / 'integration_evidence/native_cluster_reconstruction_reuse_v1'
SHARED = 'integration_evidence/native_cluster_balance_profile_v1'
CONTROL = ROOT / 'build-native/reconstruction-control/test_driver'
CANDIDATE = ROOT / 'build-native/UnitTests/test_driver'
CHECKPOINT = ROOT / 'build-native/reconstruction_reuse_checkpoint.json'


def sha(path):
    with path.open('rb') as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def closed(path):
    path = path.resolve()
    path.relative_to(ROOT)
    if not path.is_file():
        raise FileNotFoundError(path)
    return path


def preserve():
    CONTROL.resolve().relative_to(ROOT)
    expected = json.loads((PACK / 'control.json').read_text())['test_driver_sha256']
    if CONTROL.exists():
        if sha(closed(CONTROL)) != expected:
            raise ValueError('Existing control differs; preserve it and recover the validated job582199 executable')
    else:
        source = closed(CANDIDATE)
        if sha(source) != expected:
            raise ValueError('Preserve the job582199 test_driver BEFORE rebuilding. Expected '+expected)
        CONTROL.parent.mkdir(parents=True, exist_ok=True)
        with source.open('rb') as incoming, CONTROL.open('xb') as outgoing:
            shutil.copyfileobj(incoming, outgoing)
        shutil.copymode(source, CONTROL)
        assert sha(closed(CONTROL)) == expected
    print('Validated control retained:', CONTROL)


def pins():
    names = {str(p.relative_to(ROOT)) for directory in ('Common', 'SU2_CFD', 'UnitTests')
             for p in (ROOT / directory).rglob('*') if p.is_file() and p.suffix in ('.cpp', '.hpp', '.h', '.inl', '.tpp')}
    names.update(('meson.build', 'UnitTests/meson.build', 'Common/src/adaptation/meson.build'))
    for package in (PACK, ROOT / SHARED):
        names.update(str(p.relative_to(ROOT)) for p in package.iterdir() if p.is_file())
    sys.path.insert(0, str(ROOT / 'integration_evidence'))
    from collect_native_cluster_results import TOOLS
    names.update(TOOLS)
    names.add('integration_evidence/native_process_sampler.py')
    for kind in ('frozen_euler_to_bl', 'frozen_bl_to_euler'):
        names.update(str(p.relative_to(ROOT)) for p in (ROOT / 'integration_evidence/native_cluster_campaign_v1/inputs' / kind).iterdir())
    return {name: sha(closed(ROOT / name)) for name in sorted(names)}


def verify(record):
    if record['status'] != 'PREPARED_NOT_VALIDATED' or pins() != record['files_sha256']:
        raise ValueError('Source/test/input/tool checkpoint changed: rebuild and prepare again')
    for role, info in record['binaries'].items():
        if sha(closed(ROOT / info['path'])) != info['sha256']:
            raise ValueError(role + ' executable changed after preparation')
    expected = json.loads((PACK / 'control.json').read_text())['test_driver_sha256']
    if record['binaries']['control']['sha256'] != expected:
        raise ValueError('Control is not the validated job582199 executable')


def prepare():
    CHECKPOINT.resolve().relative_to(ROOT)
    expected = json.loads((PACK / 'control.json').read_text())['test_driver_sha256']
    if sha(closed(CONTROL)) != expected:
        raise ValueError('Control differs from the validated cluster executable')
    if sha(closed(CANDIDATE)) == expected:
        raise ValueError('Candidate was not rebuilt: it still equals the control')
    metadata = ROOT / 'build-native/meson-info/intro-buildoptions.json'
    record = dict(status='PREPARED_NOT_VALIDATED', files_sha256=pins(),
                  binaries={role:dict(path=str(closed(path).relative_to(ROOT)), sha256=sha(path))
                            for role, path in [('control', CONTROL), ('profile', CANDIDATE)]},
                  build_options=json.loads(closed(metadata).read_text()),
                  scope='Identities only; retain build logs. Preparation is not compilation provenance or numerical PASS.')
    verify(record)
    CHECKPOINT.parent.mkdir(parents=True, exist_ok=True)
    with CHECKPOINT.open('x') as handle:
        handle.write(json.dumps(record, indent=2)+'\n')
    print('Prepared:', CHECKPOINT, '\nNo cluster job or solver was started.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preserve-control', action='store_true')
    args = parser.parse_args()
    preserve() if args.preserve_control else prepare()
