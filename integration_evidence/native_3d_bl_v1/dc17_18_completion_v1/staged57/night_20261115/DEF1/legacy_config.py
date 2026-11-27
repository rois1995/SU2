"""Append DEF1's two legacy defaults to an authenticated gate input."""

import hashlib
import json
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
R = NIGHT.parent
REPO = R.parents[3]
BASE = 'efbab5351e'
DEFAULTS = {'ADAP_NORM': b'ADAP_NORM= 1.0', 'ADAP_HGRAD': b'ADAP_HGRAD= 1.3'}
CONTROL_REF = NIGHT / 'FP3_E/controls/build680'
FULL_REF = NIGHT / 'FP3_E/accepted_reference.json'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(path.read_text())


def committed(path):
    """Authenticate stable metadata against the last pre-DEF1 commit."""
    data = subprocess.check_output(['git', 'show', f'{BASE}:{path.relative_to(REPO)}'], cwd=REPO)
    assert path.read_bytes() == data, path
    return path


def derived(blob):
    """Keep all existing bytes, including explicit legacy spellings and comments."""
    lines = [line for key, line in DEFAULTS.items()
             if not re.search(rb'(?im)^[ \t]*' + key.encode() + rb'[ \t]*=', blob)]
    if not lines:
        return blob, []
    separator = b'\r\n' if b'\r\n' in blob else b'\n'
    prefix = b'' if not blob or blob.endswith(b'\n') else separator
    return blob + prefix + separator.join(lines) + separator, [line.decode() for line in lines]


def metadata_config(case):
    config = case / 'run.cfg'
    if (case / 'provenance.json').is_file():
        metadata = case / 'provenance.json'
        record = read(metadata)
        recorded = record['config_sha256']
        assert record['input_hashes'].get(str(config), recorded) == recorded, metadata
    else:
        metadata = case / 'pins.json'
        pins = read(metadata)
        recorded = pins.get(str(config), pins.get(str(config.resolve())))
    assert recorded and digest(config.read_bytes()) == recorded, (config, metadata)
    return config, metadata, recorded


def gate_reference(name):
    """Reauthenticate the selected sealed FP3-E standing configuration and receipt."""
    entry = committed(CONTROL_REF / name / 'case_summary.json')
    gate = read(entry)
    assert gate['status'] == 'passed', entry
    if 'subruns' in gate:
        mode, = gate['subruns']
        gate = gate['subruns'][mode]
    case = Path(gate['case'])
    config, metadata, recorded = metadata_config(case)
    pins = read(entry)['pins']
    assert pins.get(str(config), recorded) == recorded and pins[str(metadata)] == digest(metadata.read_bytes()), entry
    return config, [entry, metadata], recorded


def write_json(destination, payload):
    text = json.dumps(payload, indent=2, sort_keys=True) + '\n'
    if destination.exists():
        assert destination.read_text() == text, destination
    else:
        with destination.open('x') as target:
            target.write(text)


def prepared(reference, sources, expected, destination, *, launch_source=None):
    """Verify the pre-insertion copy, emit the sole diff, and seal the derivation."""
    original = reference.read_bytes()
    assert digest(original) == expected, reference
    if launch_source:
        assert launch_source.read_bytes() == original, (launch_source, reference)
    final, inserted = derived(original)
    if destination.exists():
        assert destination.read_bytes() == final, destination
    else:
        with destination.open('xb') as target:
            target.write(final)
    receipt = {
        'sealed_config': str(reference), 'sealed_sha256': expected,
        'sealed_receipts': {str(path): digest(path.read_bytes()) for path in sources},
        'inserted_lines': inserted, 'derived_config': str(destination),
        'derived_sha256': digest(final),
    }
    write_json(destination.with_name('legacy_config.json'), receipt)
    return receipt


def verify_case(case, reference, sources, expected):
    """Check the exact launch bytes, recording, and content derivation."""
    path, metadata, digest_expected = metadata_config(case)
    result, inserted = derived(reference.read_bytes())
    assert digest(reference.read_bytes()) == expected and path.read_bytes() == result, path
    assert digest_expected == digest(result), metadata
    if (case / 'command.json').is_file():
        command = read(case / 'command.json')
        if 'SU2_NATIVE_FROZEN_CONFIG' in command['environment']:
            assert Path(command['environment']['SU2_NATIVE_FROZEN_CONFIG']).resolve() == path.resolve(), command
    record = {
        'sealed_config': str(reference), 'sealed_sha256': expected,
        'sealed_receipts': {str(src): digest(src.read_bytes()) for src in sources},
        'inserted_lines': inserted, 'derived_config': str(path), 'derived_sha256': digest_expected,
        'launch_metadata': {'path': str(metadata), 'sha256': digest(metadata.read_bytes())},
    }
    write_json(case / 'legacy_launch_config.json', record)
    return record, (reference, *sources, metadata, path, case / 'legacy_launch_config.json')


def self_check():
    examples = (
        (b'ADAP_HGRAD= 1.3\n', b'ADAP_HGRAD= 1.3\nADAP_NORM= 1.0\n', ['ADAP_NORM= 1.0']),
        (b'% ADAP_HGRAD= 8\n ADAP_NORM = 3', b'% ADAP_HGRAD= 8\n ADAP_NORM = 3\nADAP_HGRAD= 1.3\n',
         ['ADAP_HGRAD= 1.3']),
        (b'# unchanged\r\n  ADAP_HGRAD= 7\r\n', b'# unchanged\r\n  ADAP_HGRAD= 7\r\nADAP_NORM= 1.0\r\n',
         ['ADAP_NORM= 1.0']),
        (b' ADAP_HGRAD= 7\nADAP_NORM= 5\n', b' ADAP_HGRAD= 7\nADAP_NORM= 5\n', []),
        (b' adap_hgrad= 7\nAdap_Norm= 5\n', b' adap_hgrad= 7\nAdap_Norm= 5\n', []),
    )
    for original, final, inserted in examples:
        assert derived(original) == (final, inserted)
