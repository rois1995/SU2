"""Verify Part F wiring and the exact purpose gate without a solver launch."""
from hashlib import file_digest
from pathlib import Path
import sys
import tempfile

sys.dont_write_bytecode = True
import workflow_f as workflow


def check_purpose():
    first = [{'event': 1, 'native_seconds': 2000}, {'event': 2, 'native_seconds': 1344}]
    second = [{'event': 1, 'native_seconds': 2100}, {'event': 2, 'native_seconds': 1288.032}]
    assert workflow.purpose(first, second)['passed']
    second[1]['native_seconds'] = 1288.0321
    assert not workflow.purpose(first, second)['passed']
    first[0]['native_seconds'] = 2300
    second[0]['native_seconds'] = 2240
    second[1]['native_seconds'] = 900
    assert not workflow.purpose(first, second)['passed']


def check_binding(draft):
    with tempfile.TemporaryDirectory(prefix='recipe_preflight_', dir=draft) as temp:
        live_path = workflow.WORKSPACE
        try:
            workflow.WORKSPACE = Path(temp) / 'G5'
            workflow.prepare(580)
            helper_pins = workflow.cut.read(workflow.WORKSPACE / 'helper_pins.json')
            workflow.verify_pins(helper_pins['bound'])
            assert len(helper_pins['bound']) == 7 and helper_pins['build'] == 580
            for stem in workflow.RUNNERS:
                text = (workflow.WORKSPACE / f'{stem}580.py').read_text()
                assert 'full580_run1' in text and 'build575' not in text
        finally:
            workflow.WORKSPACE = live_path


def check_historical_restart():
    g5 = workflow.cut.PHASE / 'phase_N6/G5'
    receipt = workflow.cut.read(g5 / 'disk_restart_full578_event2/disk_restart_validation.json')
    with (g5 / 'full578/full_workflow_receipt.json').open('rb') as file:
        assert file_digest(file, 'sha256').hexdigest() == receipt['source_event_receipt_sha256']


def main():
    draft = Path(__file__).resolve().parent
    for file in draft.glob('*.py'):
        compile(file.read_bytes(), str(file), 'exec')
    check_purpose()
    check_binding(draft)
    check_historical_restart()
    baseline = workflow.T0
    old_attempt = workflow.N16_C / 'full575_run1'
    prefix = workflow.compare_event_one(baseline, old_attempt)
    assert prefix['passed'] and len(prefix['ranks']) == 6
    print('Accepted prefix rows per rank:', [row['reference_rows'] for row in prefix['ranks']])
    print('Passed recipe binding, source compile, exact gate arithmetic, and the N6 restart receipt hash.')


if __name__ == '__main__':
    main()
