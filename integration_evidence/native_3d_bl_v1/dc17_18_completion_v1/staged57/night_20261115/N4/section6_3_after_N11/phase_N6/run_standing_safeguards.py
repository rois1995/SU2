"""Run and compare the accepted geometry safeguards after the N6 G5 restart."""
from pathlib import Path
import subprocess
import sys

from run_g5 import ROOT, WORKSPACE, cut, progress

sys.path.insert(0, str(cut.NIGHT / 'N16'))
from run_c import compare_safeguard


def main():
    build = 578
    assert cut.read(WORKSPACE / 'acceptance_summary.json')['passed']
    assert cut.terminal(cut.read(cut.NIGHT / 'N6/G5_campaign_launch.json'))
    assert cut.read(WORKSPACE / f'full{build}/strict_full_identity.json')['passed']
    disk = f'disk_restart_full{build}_event2'
    assert cut.read(WORKSPACE / disk / 'disk_restart_validation.json')['disk_restart_verified']
    workspace = ROOT / 'safeguards'
    runner = workspace / f'run_build{build}_safeguards.py'
    base = ROOT.parent / 'phase_D/safeguards'
    adopted = base / 'run_build572_safeguards.py'
    assert runner.read_text() == adopted.read_text().replace('572', str(build))
    cut.save(workspace / 'runner_pins.json', {'source': str(adopted), 'source_sha256': cut.sha(adopted),
             'runner': str(runner), 'runner_sha256': cut.sha(runner)})
    with (workspace / 'self_check.json').open('x') as output:
        subprocess.run([cut.PYTHON, str(runner), 'self-check'], stdout=output, check=True)
    assert cut.read(workspace / 'self_check.json')['passed']
    results = {}
    for mode, reference in cut.read(base / 'acceptance_summary.json')['safeguards'].items():
        cases = workspace / 'cases'
        before = set(cases.iterdir()) if cases.exists() else set()
        run = cut.run_gate(workspace, mode, [cut.PYTHON, str(runner), mode])
        case, = set(cases.iterdir()) - before
        status = cut.read(case / 'status.json')
        process = cut.read(case / 'safeguard_process_receipt.json')
        assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
        assert not status['changed'] and not process['live_owned_descendants']
        assert process['state'] == 'terminal' and process['exit_code'] == 0
        identities = process['identities'] + [run['controller']]
        assert all(cut.terminal(row) for row in identities)
        pins = cut.read(case / 'pins.json')
        assert all(cut.sha(path) == digest for path, digest in pins.items())
        result = compare_safeguard(mode, Path(reference['case']), case)
        result.update(case=str(case), seconds=status['seconds'], terminal_process_count=len(identities),
                      verified_pins=len(pins), sampled_tree_peak_rss_bytes=process['peak_sampled_tree_rss_bytes'])
        cut.save(workspace / f'{mode}_receipt.json', result)
        assert result['passed'] and result['exact'], (mode, result)
        results[mode] = result
        progress('N6', f'{mode} passed in {status["seconds"]:.3f} s with exact saved-state comparison.')
    cut.save(workspace / 'acceptance_summary.json', {'passed': True, 'build': build, 'safeguards': results})


if __name__ == '__main__':
    main()
