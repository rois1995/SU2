"""Observe an existing orphaned M6 job; never launch or signal its solver processes."""
import json, re, sys, time
from pathlib import Path
from run_m6 import native_assessment, save_completed_workflow, sample_tree, save, sha

attempt = Path(sys.argv[1]).resolve()
status = json.loads((attempt / 'status.json').read_text())
assert status['state'] == 'running'
pid, started = status['pid'], status['started_at']
proc = Path('/proc') / str(pid)
assert (proc / 'comm').read_text().strip() == 'time'
assert Path((proc / 'cwd').resolve()) == attempt
identity = (proc / 'stat').read_text().split()[21]
pins = json.loads((attempt / 'pins.json').read_text())
assert all(sha(p) == digest for p, digest in pins.items())
save(attempt / 'monitor_recovery.json', dict(pid=pid, proc_start_ticks=identity,
    recovered_at=time.time(), original_harness='lost during server restart', pins_verified=True,
    monitor_sha256=sha(__file__), exit_status_source='/usr/bin/time run.log trailer'))
largest = 0
with (attempt / 'recovered_resources.jsonl').open('a') as receipt:
    while proc.exists():
        fields = (proc / 'stat').read_text().split()
        if fields[21] != identity or fields[2] == 'Z': break
        rows = sample_tree(pid)
        largest = max(largest, max((r['rss_bytes'] for r in rows), default=0))
        receipt.write(json.dumps(dict(elapsed_seconds=time.time()-started, processes=rows,
            sampled_process_tree_rss_bytes=sum(r['rss_bytes'] for r in rows)))+'\n')
        receipt.flush()
        save_completed_workflow(attempt, 2, reuse=True)
        time.sleep(3)
# Missing trailer or surviving ranks prevents a terminal success claim.
rows = sample_tree(pid)
assert not any(r['name'] in ('SU2_CFD', 'mpirun', 'timeout') for r in rows)
log = (attempt / 'run.log').read_text(errors='replace')
codes = re.findall(r'^\s*Exit status: (\d+)\s*$', log, re.M)
assert len(codes) == 1, 'Cannot recover solver exit status from time trailer'
code = int(codes[0])
changed = [p for p, digest in pins.items() if sha(p) != digest]
assessment = native_assessment([attempt/'run.log', *attempt.glob('rank_output/*/rank.*/stdout')], code, 2)
save(attempt/'native_assessment.json', assessment)
workflow = save_completed_workflow(attempt, 2)
receipt_changes = []
for event in workflow:
    path = attempt/f"workflow_event_{event['event']:05d}.json"
    if path.exists():
        for name, digest in json.loads(path.read_text())['output_sha256'].items():
            if sha(attempt/name) != digest: receipt_changes.append(name)
accepted = code == 0 and not changed and not receipt_changes and assessment['native_event_accepted'] is True and len(workflow) == 2 and all(r['complete'] for r in workflow)
save(attempt/'workflow_assessment.json', dict(workflow_accepted=accepted, events=workflow,
    receipt_output_changes=receipt_changes, restart_and_unsteady_history='not validated'))
save(attempt/'status.json', dict(state='terminal', solver_exit_code=code,
    native_event_accepted=assessment['native_event_accepted'], native_state=assessment['native_state'],
    workflow_accepted=accepted, pins_verified=not changed, changed=changed,
    seconds=time.time()-started, recovered_monitor=True,
    largest_recovery_sampled_individual_process_rss_bytes=largest,
    rss_note='sampled process RSS; not a whole-job peak'))
