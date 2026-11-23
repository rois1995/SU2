"""Run the isolated restart which the host preflight prevented from launching."""

import run_rcfd as runner

from audit_c1 import audit

root = runner.HERE
build_dir = runner.C1 / 'build647'
work = root / 'G5/build647/OFF'
case = work / 'full647_run1'
config = runner.config_for('OFF')
build, _, _ = runner.lab.prepare(build_dir)
runner.bind_inherited()

# Reuse the already-sealed full run. Its controller and launcher stay pinned.
runner.workflow.verify_helpers(work)
runner.BASE['status_ok'](case, build, config)
wait = runner.cut.read(work / 'full_wait.json')
assert wait['returncode'] == 0 and wait['terminal'] and runner.cut.terminal(wait['controller'])
assert wait['controller'] == runner.cut.read(work / 'full_launch.json')['controller']
assert all(p.name == "restart_preflight_failure.json" for p in work.glob("restart_*"))
assert not (work / 'disk_restart_full647_run1_event2').exists()

prior = work / 'failed_attempt.json'
receipt = work / 'restart_preflight_failure.json'
if prior.exists():
    row = runner.cut.read(prior)
    assert row['stage'] == 'isolated restart' and 'restart_host_preflight.json' in row['error']
    assert not receipt.exists()
    prior.rename(receipt)
assert receipt.is_file()
runner.cut.save(root / 'before_off_restart_terminal_audit.json', audit(root))

def reuse_bound_helpers(folder, label, pinned_build, pinned_config):
    assert (folder, label, pinned_build, pinned_config) == (work, 647, build_dir, config)
    runner.workflow.verify_helpers(folder)

# run_case verifies the full_wait receipt and launches only the remaining stage.
runner.bind_campaign = lambda mode, paths: {'prepare': reuse_bound_helpers}
payloads = runner.runpy.run_path(str(runner.workflow.PAYLOADS))
runner.run_case(work, 'OFF', build_dir, build, config, [], payloads)
runner.verify_run(work, 'OFF', build, payloads)
print('RCFD1 R-OFF PASSED after resuming the unlaunched isolated restart.', flush=True)
