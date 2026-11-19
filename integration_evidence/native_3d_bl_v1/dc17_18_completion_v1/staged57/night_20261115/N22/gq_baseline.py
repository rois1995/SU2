"""Capture N22 GQ from the accepted, unchanged N21 wide build."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import sys

ITEM = Path(__file__).resolve().parent
sys.path.insert(0, str(ITEM.parent / 'N18'))
from run_de import bind_identity, check_build, cut, git

BUILD = 589
NAME = 'N22Baseline'
WORK = cut.PHASE / NAME / 'validation'
CASE = WORK / 'cases/N22_GQ_N21_589'
MANIFEST = ITEM / 'gq_reference.json'


def launch_controller():
    """Use the historical stop and resource controller in reference-capture mode."""
    wrapper = WORK / 'run_g2_gq_prefix_589.py'
    spec = spec_from_file_location('n22_reference_prefix', wrapper)
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    module.publish_aliases()
    module.os.environ['OMPI_MCA_osc'] = 'pt2pt'
    env, _ = module.load_runner()['namespace']()
    env['verify_pins']()
    sys.argv = [str(env['N4_SCRIPT']), str(CASE.resolve()), '--gate', 'GQ']
    env['main']()


def capture():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    audit = cut.read(ITEM.parent / 'N21/final_terminal_audit.json')
    assert audit['all_owned_handles_terminal'] and audit['build'] == BUILD
    assert all(cut.terminal(row) for row in audit['identities'])
    build_dir = cut.PHASE / 'build589'
    status = check_build(build_dir)
    accepted = cut.read(ITEM.parent / 'N21/accepted_reference.json')
    assert accepted['accepted'] and status['binary_sha256'] == accepted['solver_sha256'] == audit['binary_sha256']
    assert cut.sha(status['binary']) == status['binary_sha256']
    assert all(cut.sha(path) == digest for path, digest in cut.read(build_dir / 'source_pins.json').items())
    assert not git('diff', '--name-only', 'HEAD', '--', 'Common', 'SU2_CFD', 'UnitTests')
    if not WORK.exists():
        assert bind_identity(NAME, BUILD) == WORK
    wait = cut.run_gate(ITEM, 'gq_baseline', [cut.PYTHON, '-B', str(Path(__file__).resolve()), 'controller'])
    result = cut.read(CASE / 'status.json')
    receipt = cut.read(CASE / 'GQ_gate_receipt.json')
    assert result['mode'] == 'reference_capture' and result['gate'] == 'GQ'
    assert result['gate_passed'] and result['all_owned_handles_terminal']
    assert result['inputs_unchanged'] and result['source_and_binary_pins_unchanged']
    assert receipt['quality_window_rows'] == dict.fromkeys(map(str, range(6)), 3)
    logs = sorted(CASE.glob('rank_output/*/rank.*/stdout'))
    assert len(logs) == 6
    cut.save(MANIFEST, {'passed': True, 'build': BUILD, 'commit': git('rev-parse', 'HEAD'),
            'case': str(CASE), 'rank_stdout_sha256': {str(path): cut.sha(path) for path in logs},
            'status_sha256': cut.sha(CASE / 'status.json'), 'binary_sha256': status['binary_sha256'],
            'source_pins_sha256': cut.sha(build_dir / 'source_pins.json'), 'wait': wait})
    print(f'N21 wide build589 GQ reference sealed: {MANIFEST}', flush=True)


if __name__ == '__main__':
    if sys.argv[1:] == ['controller']:
        launch_controller()
    else:
        assert sys.argv[1:] == ['capture']
        capture()
