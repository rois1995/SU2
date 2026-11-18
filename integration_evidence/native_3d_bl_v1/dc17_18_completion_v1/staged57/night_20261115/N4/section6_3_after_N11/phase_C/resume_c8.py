"""Resume C8 at the first numerical G3 launch, then continue the reviewed C9–C11 campaign."""
from pathlib import Path
import sys

import run_cut_step as cut


def main():
    stage = cut.PHASE / 'C8/validation'
    build = cut.PHASE / 'build566'
    failed = stage / 'R3_G3_alias_collision'
    stopped = cut.read(failed / 'G3_launch.json')
    assert all(cut.terminal(stopped[label]) for label in ('controller', 'supervisor'))
    assert cut.read(failed / 'G3_wait.json')['returncode'] == 1
    assert 'FileExistsError' in (failed / 'G3_outer.log').read_text()
    assert not list((stage / 'cases').glob('C8_G3_*'))
    assert cut.read(build / 'build_status.json')['pins_verified']
    assert all(cut.sha(path) == digest for path, digest in cut.read(build / 'source_pins.json').items())
    for cohort in ('serial', 'mpi2', 'mpi4'):
        identity = cut.read(stage / f'G1_candidate_{cohort}_summary.json')
        wait = cut.read(stage / f'G1_{cohort}_wait.json')
        assert identity['identity_pass'] and identity['changed_case_count'] == 0
        assert wait['returncode'] == 0 and wait['terminal'] and cut.terminal(wait['controller'])
    prefix, _ = cut.seal_prefix(stage, 8, 566, 'G2')
    assert prefix['passed']
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    source = cut.read(stage / 'candidate_source.json')
    touched = list(map(Path, source['source_paths']))
    cut.record_progress(8, 566, 'G1 and G2 passed. Alias-compatible G3 wrapper verified. Starting G3, then GQ.')
    try:
        cut.run_gate(stage, 'G3', [cut.PYTHON, str(stage / 'launch_g3.py')])
        cut.run_gate(stage, 'GQ', [cut.PYTHON, str(stage / 'run_g2_gq_prefix_566.py'),
                                  '--gate', 'GQ', '--case-name', 'C8_GQ_566'])
        cut.finalize(8, 566, stage, build, touched)
    except Exception as error:
        cut.record_progress(8, 566, f'controller stopped at {error}. Preserve the case and verify handles before diagnosis.')
        raise
    sys.argv = [str(cut.PHASE / 'phase_C/run_cut_step.py'), '9', '--last', '11']
    cut.main()


if __name__ == '__main__':
    main()
