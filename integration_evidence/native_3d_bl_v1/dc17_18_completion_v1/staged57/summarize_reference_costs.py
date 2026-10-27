"""Summarize a completed single-rank private surface-move cost case."""
import json
import math
import sys
from pathlib import Path
from run_cases import sha, save

PREFIX = 'NATIVE_REFERENCE_EVENT '
FIELDS = ('current_points', 'current_faces', 'reference_triangles', 'face_pieces',
          'trace_pieces', 'maximum_integer_bits', 'arithmetic_work', 'admitted_peak_bytes',
          'accepted_resident_bytes', 'payload_bytes', 'seconds', 'lifetime_process_peak_rss_bytes')
BOUNDED = FIELDS[:-2]


def main():
    if len(sys.argv) != 2: raise SystemExit('usage: summarize_reference_costs.py CASE_DIRECTORY')
    case = Path(sys.argv[1]).resolve(); status = case / 'status.json'; log = case / 'run.log'
    output = case / 'reference_cost_summary.json'
    if output.exists(): raise RuntimeError('Receipt already exists: ' + str(output))
    paths = (status, log, Path(__file__).resolve()); pins = {str(p): sha(p) for p in paths}
    state = json.loads(status.read_text())
    if state.get('state') != 'terminal' or state.get('exit_code') != 0 or not state.get('pins_verified'):
        raise ValueError('Case is not terminal, successful and pin-verified')
    rows = []
    for line in log.read_text().splitlines():
        if line.startswith(PREFIX): rows.append(json.loads(line[len(PREFIX):]))
    if len(rows) != 16 or [r.get('event') for r in rows] != list(range(16)):
        raise ValueError('Expected exactly events 0–15 in order')
    for row in rows:
        if row.get('rank') != 0 or row.get('scope') != 'private_overlapping_surface_moves':
            raise ValueError('Expected one rank and private overlapping surface-move scope')
        for key in FIELDS:
            value = row.get(key)
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
                raise ValueError('Invalid positive finite field: ' + key)
    phases = []
    for phase in range(4):
        baseline = rows[4 + phase]
        for event in range(8 + phase, 16, 4):
            compared = {key: {'baseline': baseline[key], 'value': rows[event][key],
                              'within_bound': rows[event][key] <= baseline[key]} for key in BOUNDED}
            phases.append({'phase': phase, 'baseline_event': 4 + phase, 'event': event, 'fields': compared})
    if any(not field['within_bound'] for comparison in phases for field in comparison['fields'].values()):
        raise ValueError('Per-phase cost or descriptor bound failed')
    if any(sha(Path(p)) != h for p, h in pins.items()): raise RuntimeError('Inputs changed during summary')
    save(output, {'scope': 'private overlapping surface moves; not full adaptation workflow',
                  'rss_note': 'lifetime process peak RSS, not allocation peak', 'sha256': pins,
                  'rows': rows, 'ranges': {key: {'min': min(r[key] for r in rows),
                                                  'max': max(r[key] for r in rows)} for key in FIELDS},
                  'phase_comparisons': phases})
    print(output)


if __name__ == '__main__': main()
