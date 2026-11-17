from pathlib import Path
import hashlib
import json
import time

REPO = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
P = REPO / 'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11'
B7 = P / 'build557'
B8_VALIDATION = P / 'B8/validation'
BUILD = P / 'build558'
CHANGED = {
    REPO / 'Common/src/adaptation/CNativeEngine3D.cpp',
    REPO / 'Common/src/adaptation/native_engine3d/EngineSetup.hpp',
    REPO / 'Common/src/adaptation/native_engine3d/EngineSetup.cpp',
}
EXPECTED_PINS = 1781


def sha(path):
    """Hash one file with SHA-256."""
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read(path):
    """Read a JSON receipt."""
    return json.loads(Path(path).read_text())


def build_source():
    """Retarget B7's exact build delegate without changing its build policy."""
    source = (B7 / 'build.py').read_text()
    old_doc = 'Build CUT B7 phase-ownership extraction with original transaction lifetimes.'
    assert source.count(old_doc) == 1
    source = source.replace(old_doc, 'Build CUT B8 setup extraction with original transaction lifetimes.', 1)
    old_ns = 'cut_b7_candidate557_builder'
    assert source.count(old_ns) == 1
    source = source.replace(old_ns, 'cut_b8_candidate558_builder', 1)
    assert 'ROOT = Path(__file__).resolve().parent' in source
    assert 'ROOT.parents[7]' in source
    return source


def main():
    """Create only the B8 build helper and the source inputs it will pin."""
    summary = read(P / 'B7/acceptance_summary.json')
    assert summary.get('state') == 'B7_SCOPED_G1_G2_PASSED'
    assert summary.get('candidate_build') == 557 and summary.get('source_pins') == 1780
    apply = read(P / 'B8/actual_apply_receipt.json')
    assert apply.get('applied') and apply.get('all_three_reviewed_outputs_byte_identical')
    assert apply.get('canonical_fields_and_special_members_unchanged')
    expected = {str(REPO / 'Common/src/adaptation/CNativeEngine3D.cpp'):
                    apply['applied_sha256']['Common/src/adaptation/CNativeEngine3D.cpp'],
                str(REPO / 'Common/src/adaptation/native_engine3d/EngineSetup.hpp'):
                    apply['applied_sha256']['Common/src/adaptation/native_engine3d/EngineSetup.hpp'],
                str(REPO / 'Common/src/adaptation/native_engine3d/EngineSetup.cpp'):
                    apply['applied_sha256']['Common/src/adaptation/native_engine3d/EngineSetup.cpp']}
    for path, digest in expected.items():
        assert sha(path) == digest, path
    old_pins = read(B7 / 'source_pins.json')
    assert len(old_pins) == 1780
    for path, digest in old_pins.items():
        if Path(path) in CHANGED:
            assert expected[path] != digest, (path, 'B8 source unexpectedly equals B7')
        else:
            assert sha(path) == digest, (path, 'unrelated B7 input changed')
    source = build_source()
    if BUILD.exists():
        raise FileExistsError(f'refusing to replace build directory: {BUILD}')
    BUILD.mkdir()
    helper = BUILD / 'build.py'
    helper.write_text(source)
    compile(source, str(helper), 'exec')
    pins = dict(old_pins)
    pins.update(expected)
    pins[str(helper.resolve())] = sha(helper)
    assert len(pins) == EXPECTED_PINS, len(pins)
    assert all(sha(path) == digest for path, digest in pins.items())
    pin_file = BUILD / 'source_pins.json'
    pin_file.write_text(json.dumps(pins, indent=2, sort_keys=True) + '\n')
    delta = {
        'base_build': 557,
        'accepted_base_commit': apply['base_commit'],
        'base_terminal_receipt_sha256': sha(B7 / 'primary_terminal_receipt.json'),
        'B7_source_pins_sha256': sha(B7 / 'source_pins.json'),
        'intentional_compiled_source_changes': [
            {'path': path, 'before': old_pins[path], 'after': expected[path]}
            for path in sorted(expected)
        ],
        'added_build_helper': {'path': str(helper.resolve()), 'sha256': sha(helper)},
        'source_pin_count_before': len(old_pins),
        'source_pin_count_after': len(pins),
        'runtime_helpers_are_pinned_per_validation_case': True,
        'build_launched': False,
        'created_unix': time.time(),
    }
    (BUILD / 'source_delta.json').write_text(json.dumps(delta, indent=2) + '\n')
    materialized = {
        'materialized': True,
        'build': str(BUILD),
        'build_helper_sha256': sha(helper),
        'source_pins_sha256': sha(pin_file),
        'source_pins': len(pins),
        'three_reviewed_source_deltas_match_apply_receipt': True,
        'all_other_B7_pins_unchanged': True,
        'build_started': False,
        'numerical_jobs_started': False,
    }
    (BUILD / 'materialization_receipt.json').write_text(json.dumps(materialized, indent=2) + '\n')
    print(json.dumps(materialized, indent=2))


if __name__ == '__main__':
    main()
