"""Read the existing owned workflow telemetry without touching processes or source."""
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
CASE = HERE / 'full_workflow'
spec = importlib.util.spec_from_file_location('owned_controller', HERE / 'run_full_workflow.py')
controller = importlib.util.module_from_spec(spec)
spec.loader.exec_module(controller)
status = json.loads((CASE / 'status.json').read_text())
resource = json.loads((CASE / 'host_resources.jsonl').read_text().splitlines()[-1])
rows = controller.collapse_rows(CASE, None, latest=True)
assert rows, 'No existing native progress rows'
keys = ('rank', 'stage', 'phase', 'elapsed', 'accepted', 'q_bad', 'j_bad', 'geometry_bad', 'size_bad')
print(json.dumps({'state': status['state'], 'controller_seconds': round(resource['elapsed']),
                  'loadavg': resource['host']['loadavg'],
                  'rows': [{key: row.get(key) for key in keys} for rank, row in sorted(rows.items())]}))
