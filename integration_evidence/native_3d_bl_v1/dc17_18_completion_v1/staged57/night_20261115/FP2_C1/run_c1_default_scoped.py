"""Run the build647 continuation's scoped default-path gates."""
from pathlib import Path
import os
import subprocess
import sys
from unittest.mock import patch

import run_c1_fixtures as fixtures

HERE = fixtures.HERE
NIGHT = fixtures.NIGHT
sys.path.insert(0, str(NIGHT / 'RCFD1'))
from audit_rcfd import audit

ORDER = HERE / 'inputs/FP2_C1_CONTINUE_1.md'
ORDER_SHA = '650c8f2f3234949c7fd45b3c3fb5c5f871a66e2e617089b109ed268e889b75a7'


def main():
    cut, sha = fixtures.cut, fixtures.sha
    assert sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    build, restoration = (HERE / f'build{label}' for label in (647, 649))
    original, rebuilt = (cut.read(directory / 'build_status.json') for directory in (build, restoration))
    assert original['build'] == 647 and rebuilt['build'] == 649
    expected = {key: original[key] for key in ('binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    assert expected == {key: rebuilt[key] for key in expected}
    assert sha(restoration / 'source_pins.json') == expected['source_pins_sha256']
    assert sha(Path(original['binary'])) == expected['binary_sha256']
    fixtures.verify_build(original, build / 'source_pins.json')
    changed = subprocess.check_output(['git', 'diff', '--stat', '4730bd59ec', '--',
                                       'Common', 'SU2_CFD', 'UnitTests'], cwd=cut.REPO)
    assert not changed
    audit(NIGHT / 'RCFD1')
    audit(HERE)
    for key in tuple(os.environ):
        if key.startswith(('SU2_NATIVE_', 'SU2_FP1_', 'SU2_FP2_')):
            del os.environ[key]
    print('Build649 restoration matches all three build647 hashes. Starting scoped default proof.', flush=True)
    with patch.multiple(fixtures.scope, HERE=HERE, ORDER=fixtures.ORDER, ORDER_SHA=fixtures.ORDER_SHA,
                        DRIVER=Path(__file__).resolve(), SOURCE_CHECK=HERE / 'source_check.md'):
        receipt = fixtures.scope.regression(build)
    assert receipt['passed'] and receipt['G1'] == [375, 114, 228]
    print('FP2-C1 scoped default proof passed on build647.', flush=True)


if __name__ == '__main__':
    main()
