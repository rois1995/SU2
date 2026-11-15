"""Apply the existing P6c physical workflow gates to the single F3 integration milestone."""
import hashlib
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
CASE = HERE / 'full_workflow'
P6 = HERE.parent / 'purpose_query_fix_20261114/p6c_20261115'
path = P6 / 'validate_completed_workflow.py'
spec = importlib.util.spec_from_file_location('physical_gate', path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.self_check()
status = json.loads((CASE / 'status.json').read_text())
assert status['state'] == 'terminal' and status['all_owned_handles_terminal']
assert status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
result = module.validator()(CASE)
memory = []
for event in result['events']:
    rows = event['native_summaries']
    present = len(rows) == 6 and all('process_peak_rss' in row for row in rows)
    total = sum(int(row['process_peak_rss']) for row in rows) if present else None
    passed = present and total <= 20_000_000_000
    memory.append({'event': event['event'], 'reported_rank_peak_RSS_sum_bytes': total, 'passed': passed})
    if not passed:
        result['errors'].append(f"event {event['event']}: missing or excessive native rank peak-RSS sum")
result['whole_job_20GB_gate'] = memory
result['workflow_accepted'] = result['workflow_accepted'] and all(row['passed'] for row in memory)
result['validator_sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
result['scope'] = 'One F3 integration milestone; unchanged physical/transfer/continued-CFD gates, no isolated restart rerun.'
(CASE / 'independent_workflow_receipt.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
raise SystemExit(not result['workflow_accepted'])
