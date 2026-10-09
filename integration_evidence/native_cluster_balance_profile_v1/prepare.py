"""Preserve the validated control executable, then pin the rebuilt profiling candidate."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / 'integration_evidence/native_cluster_balance_profile_v1'
CONTROL = ROOT / 'build-native/balance-control/test_driver'
CANDIDATE = ROOT / 'build-native/UnitTests/test_driver'
CHECKPOINT = ROOT / 'build-native/balance_profile_checkpoint.json'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def closed(path):
    path = path.resolve()
    path.relative_to(ROOT)
    if not path.is_file():
        raise FileNotFoundError(path)
    return path


def preserve():
    expected = json.loads((PACK / 'control.json').read_text())['test_driver_sha256']
    if CONTROL.exists():
        if sha(closed(CONTROL)) != expected:
            raise ValueError('Existing control differs; preserve it and use a separate checkout to recover the validated executable')
    else:
        source = closed(CANDIDATE)
        if sha(source) != expected:
            raise ValueError('Preserve the validated test_driver BEFORE rebuilding. Expected ' + expected + '; recover the executable from the cefb4d11c7 checkout if it was replaced.')
        CONTROL.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, CONTROL)
        assert sha(closed(CONTROL)) == expected
    print('Validated control retained:', CONTROL)


def pins():
    names = {str(p.relative_to(ROOT)) for directory in ('Common', 'SU2_CFD', 'UnitTests')
             for p in (ROOT / directory).rglob('*') if p.is_file() and p.suffix in ('.cpp', '.hpp', '.h', '.inl', '.tpp')}
    names.update(('meson.build', 'UnitTests/meson.build', 'Common/src/adaptation/meson.build'))
    names.update(str(p.relative_to(ROOT)) for p in PACK.iterdir() if p.is_file())
    sys.path.insert(0, str(ROOT / 'integration_evidence'))
    from collect_native_cluster_results import TOOLS
    names.update(TOOLS)
    names.add('integration_evidence/native_process_sampler.py')
    for kind in ('frozen_euler_to_bl', 'frozen_bl_to_euler'):
        names.update(str(p.relative_to(ROOT)) for p in (ROOT / 'integration_evidence/native_cluster_campaign_v1/inputs' / kind).iterdir())
    return {name: sha(closed(ROOT / name)) for name in sorted(names)}


def verify(record):
    if record['status'] != 'PREPARED_NOT_VALIDATED':
        raise ValueError('Wrong checkpoint status')
    current = pins()
    if current != record['files_sha256']:
        raise ValueError('Sources, tests, inputs or audit tools changed after preparation; rebuild and prepare again')
    for role, info in record['binaries'].items():
        if sha(closed(ROOT / info['path'])) != info['sha256']:
            raise ValueError(role + ' executable changed after preparation')


def prepare():
    expected = json.loads((PACK / 'control.json').read_text())['test_driver_sha256']
    if sha(closed(CONTROL)) != expected:
        raise ValueError('Control binary does not match the validated cluster executable')
    if sha(closed(CANDIDATE)) == expected:
        raise ValueError('Candidate has not been rebuilt with profiling; rebuild UnitTests/test_driver first')
    metadata = ROOT / 'build-native/meson-info/intro-buildoptions.json'
    record = dict(status='PREPARED_NOT_VALIDATED', files_sha256=pins(),
                  binaries={role:dict(path=str(closed(path).relative_to(ROOT)), sha256=sha(path))
                            for role, path in [('control', CONTROL), ('profile', CANDIDATE)]},
                  build_options=json.loads(closed(metadata).read_text()),
                  scope='Source/test/input/tool and executable identities. Preparation alone is not compilation provenance or a correctness PASS.')
    verify(record)
    CHECKPOINT.parent.mkdir(parents=True, exist_ok=True)
    if CHECKPOINT.exists():
        raise FileExistsError('Preserve the previous checkpoint; rename ' + str(CHECKPOINT) + ' before preparing another build')
    CHECKPOINT.write_text(json.dumps(record, indent=2) + '\n')
    print('Prepared:', CHECKPOINT, '\nNo cluster job or solver was started.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preserve-control', action='store_true')
    args = parser.parse_args()
    preserve() if args.preserve_control else prepare()
