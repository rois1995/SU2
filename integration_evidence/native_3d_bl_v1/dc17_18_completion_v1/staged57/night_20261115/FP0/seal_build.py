"""Verify that build warnings came from unchanged N21 dependencies."""

import re
import subprocess

from reuse_objects import ACTIVE, BUILD, NINJA, TEST_FILES, commands
from run_build import ITEM, REPO, SOURCE, cut, sha


def seal():
    assert cut.terminal(cut.read(ITEM / 'build_controller_launch.json'))
    receipt = ITEM / 'build592.json'
    state = cut.read(receipt)
    assert state['state'] == 'terminal' and state['wait']['returncode'] == 0
    assert cut.terminal(state['wait']['controller'])
    assert sha(BUILD / 'UnitTests/test_driver') == state['test_binary_sha256']
    expected = cut.read(ITEM / 'n21_source_view.json')['verified']
    tests = {(SOURCE / path).resolve() for path in TEST_FILES}
    old, new = commands(ACTIVE), commands(BUILD)
    baseline_config = ITEM / 'configure_outer.log'
    configuration_warnings = {line.partition('WARNING:')[2].strip()
                              for line in baseline_config.read_text().splitlines() if 'WARNING:' in line}
    warnings = []
    current = None
    log = ITEM / 'build592_outer.log'
    for line in log.read_text(errors='replace').splitlines():
        target = re.match(r'^\[\d+/\d+\] Compiling (?:C|C\+\+) object (.+\.o)$', line)
        if target:
            current = target[1]
        elif re.search(r'\bwarning:', line, re.IGNORECASE):
            warnings.append((current, line))
    assert [line for _, line in warnings] == state['warnings']
    unchanged, new_warnings = [], []
    for obj in dict.fromkeys(target for target, _ in warnings):
        messages = [line for target, line in warnings if target == obj]
        if obj is None and all(line.partition('WARNING:')[2].strip() in configuration_warnings for line in messages):
            unchanged.append({'object': None, 'baseline_log': str(baseline_config),
                              'baseline_log_sha256': sha(baseline_config), 'warnings': messages})
            continue
        if obj is None or obj not in old or new[obj]['command'] != old[obj]['command']:
            new_warnings.extend(messages)
            continue
        name = (BUILD / new[obj]['file']).resolve()
        if name in tests:
            new_warnings.extend(messages)
            continue
        # Ninja's compiler dependencies prove the FP0 test files cannot enter this translation unit.
        deps = subprocess.check_output([NINJA, '-C', BUILD, '-t', 'deps', obj], text=True)
        paths = [(BUILD / line.strip()).resolve() for line in deps.splitlines() if line.startswith('    ')]
        assert paths and name in paths, (obj, name)
        if tests.intersection(paths):
            new_warnings.extend(messages)
            continue
        if str(name) in expected:
            expected_source = expected[str(name)]
        else:
            original = REPO / name.relative_to(SOURCE) if name.is_relative_to(SOURCE) else name
            expected_source = sha(original)
            if original.is_relative_to(REPO):
                subprocess.run(['git', 'diff', '--exit-code', '4fe083d460', '--', original],
                               cwd=REPO, check=True, stdout=subprocess.DEVNULL)
        assert sha(name) == expected_source, name
        unchanged.append({'object': obj, 'source': str(name), 'source_sha256': sha(name),
                          'command_identical': True, 'no_fp0_test_dependency': True, 'warnings': messages})
    assert not new_warnings, new_warnings
    receipt.rename(ITEM / 'build592_initial_warning_receipt.json')
    state.update(passed=True, new_warnings=new_warnings, unchanged_source_warnings=unchanged,
                 build_log_sha256=sha(log))
    cut.save(receipt, state)
    print(f'FP0 build592 sealed with {len(warnings)} unchanged-source warnings and no new warnings.')


if __name__ == '__main__':
    seal()
