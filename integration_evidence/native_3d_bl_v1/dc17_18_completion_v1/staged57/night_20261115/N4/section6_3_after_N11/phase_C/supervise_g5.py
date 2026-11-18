"""Run the prepared six-rank phase-C workflow and its isolated event-two restart."""
from argparse import ArgumentParser
from pathlib import Path
import runpy
import sys

import run_cut_step as cut


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    args = parser.parse_args()
    build = args.build
    workspace = cut.PHASE / 'phase_C/G5'
    case = workspace / f'full{build}'
    source = cut.PHASE / f'build{build}/source_pins.json'
    end = cut.read(cut.PHASE / 'C11/validation/acceptance_summary.json')
    assert all(end[gate]['passed'] for gate in ('G3', 'G4', 'GQ'))
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert all(cut.sha(path) == digest for path, digest in cut.read(source).items())
    full = workspace / f'run_full{build}.py'
    restart = workspace / f'run_restart{build}.py'
    reference = cut.NIGHT / 'N14/section6_12/validation/full552'
    adopted = cut.read(reference / 'n8_payload_identity.json')
    assert adopted['passed'] and adopted['workflow_gates_passed']
    assert all(cut.sha(reference / path) == digest for path, digest in adopted['rank_stdout_sha256'].items())
    cut.record_progress(11, build, 'Phase-end G3, G4 and GQ passed. The full two-event G5 is running.')
    try:
        cut.run_gate(workspace, 'G5_full', [sys.executable, str(full), '--case', str(case)])
        workflow = cut.read(case / 'n8_payload_identity.json')
        assert workflow['passed'] and workflow['workflow_gates_passed']
        adapter = cut.PHASE / 'candidate_full_preparation/validate_full.py'
        compare = runpy.run_path(str(adapter))['full_identity']
        identity = compare(reference, case)
        cut.save(case / 'strict_full_identity.json', identity)
        assert identity['passed'], 'See strict_full_identity.json for the first full-workflow difference.'
        cut.record_progress(11, build, 'G5 full workflow and six payload identities passed. Starting isolated event-two restart.')
        cut.run_gate(workspace, 'G5_restart', [sys.executable, str(restart), '--source-case', str(case)])
        restart_case = workspace / f'disk_restart_{case.name}_event2'
        receipt = cut.read(restart_case / 'disk_restart_validation.json')
        assert receipt['disk_restart_verified']
        assert cut.read(restart_case / 'post_terminal_handles.json')['all_owned_handles_terminal']
        cut.record_progress(11, build, 'G5 full workflow and isolated restart passed. Phase report and terminal audit pending.')
    except Exception as error:
        cut.record_progress(11, build, f'G5 controller stopped at {error}. Verify handles and preserve all case evidence.')
        raise


if __name__ == '__main__':
    main()
