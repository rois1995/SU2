"""Execute the 481 and saved-target safeguards on the qualified phase-C binary."""
from pathlib import Path

import run_cut_step as cut


def main():
    phase = cut.PHASE / 'phase_C'
    g5 = phase / 'G5'
    full = g5 / 'full570'
    restart = g5 / 'disk_restart_full570_event2'
    assert cut.read(full / 'strict_full_identity.json')['passed']
    assert cut.read(restart / 'disk_restart_validation.json')['disk_restart_verified']
    controller = cut.read(phase / 'G5_campaign_launch.json')
    assert cut.terminal(controller)
    workspace = phase / 'safeguards'
    runner = workspace / 'run_build570_safeguards.py'
    pin = cut.read(workspace / 'runner_pins.json')
    assert cut.sha(runner) == pin['candidate_sha256']
    assert cut.read(workspace / 'no_launch_checks.json')['passed']
    results = {}
    for mode in pin['modes']:
        cases = workspace / 'cases'
        before = set(cases.glob('*'))
        run = cut.run_gate(workspace, mode, [cut.PYTHON, str(runner), mode])
        case, = set(cases.glob('*')) - before
        status = cut.read(case / 'status.json')
        process = cut.read(case / 'safeguard_process_receipt.json')
        assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
        assert not status['changed'] and not process['live_owned_descendants']
        assert process['state'] == 'terminal' and process['exit_code'] == 0
        assert cut.terminal(run['controller']) and all(cut.terminal(row) for row in process['identities'])
        pins = cut.read(case / 'pins.json')
        assert all(cut.sha(path) == digest for path, digest in pins.items())
        results[mode] = {'passed': True, 'case': str(case), 'seconds': status['seconds'],
                         'terminal_process_count': len(process['identities']) + 1,
                         'source_input_and_runtime_pins': len(pins),
                         'sampled_tree_peak_rss_bytes': process['peak_sampled_tree_rss_bytes']}
        cut.save(workspace / f'{mode}_receipt.json', results[mode])
    cut.save(workspace / 'acceptance_summary.json', {'passed': True, 'build': 570, 'safeguards': results})
    print('All phase-C standing safeguards passed.', flush=True)


if __name__ == '__main__':
    main()
