"""Summarize a completed single-rank private surface-move cost case."""
import json
import math
import re
import sys
from pathlib import Path
from run_cases import sha, save

PREFIX = 'NATIVE_REFERENCE_EVENT '
FIELDS = ('current_points', 'current_faces', 'reference_triangles', 'face_pieces',
          'trace_pieces', 'maximum_integer_bits', 'arithmetic_work', 'admitted_peak_bytes',
          'accepted_resident_bytes', 'payload_bytes', 'seconds', 'lifetime_process_peak_rss_bytes')
BOUNDED = FIELDS[:-2]
PUBLICATION_PREFIX = 'NATIVE_PUBLICATION_EVENT '
PUBLICATION_FIELDS = ('operator_work', 'publication_work', 'source_envelope_work',
    'operator_peak_bytes', 'publication_peak_bytes', 'source_envelope_peak_bytes',
    'arithmetic_work_excludes_decode', 'face_pieces', 'trace_pieces', 'maximum_integer_bits',
    'payload_bytes', 'resident_bytes', 'ascii_bytes', 'archive_load_peak_bytes',
    'seconds', 'lifetime_process_peak_rss_bytes')
PUBLICATION_BOUNDED = ('face_pieces', 'trace_pieces', 'maximum_integer_bits',
                       'arithmetic_work_excludes_decode', 'combined_admitted_peak_bytes')


def main():
    if len(sys.argv) != 2: raise SystemExit('usage: summarize_reference_costs.py CASE_DIRECTORY')
    case = Path(sys.argv[1]).resolve(); status = case / 'status.json'; log = case / 'run.log'
    command = case / 'command.json'; receipt_pins = case / 'pins.json'
    paths = (status, log, command, receipt_pins, Path(__file__).resolve()); pins = {str(p): sha(p) for p in paths}
    state = json.loads(status.read_text())
    if state.get('state') != 'terminal' or state.get('exit_code') != 0 or not state.get('pins_verified'):
        raise ValueError('Case is not terminal, successful and pin-verified')
    records = {'surface': [], 'publication': []}
    for line in re.sub(r'\x1b\[[0-9;]*m', '', log.read_text()).splitlines():
        for mode, prefix in (('surface', PREFIX), ('publication', PUBLICATION_PREFIX)):
            if line.startswith(prefix): records[mode].append(json.loads(line[len(prefix):]))
    modes = [key for key, rows in records.items() if rows]
    if len(modes) != 1: raise ValueError('Expected exactly one event mode')
    mode = modes[0]; rows = records[mode]
    output = case / ('publication_cost_summary.json' if mode == 'publication' else 'reference_cost_summary.json')
    if output.exists(): raise RuntimeError('Receipt already exists: ' + str(output))
    groups = {}
    entity_labelled = mode == 'publication' and any('entity_initial' in row for row in rows)
    legacy_labelled = mode == 'publication' and any('legacy_initial' in row for row in rows)
    for row in rows:
        legacy = row.get('legacy_initial', False) if mode == 'publication' else False
        entity = row.get('entity_initial', False) if mode == 'publication' else False
        if not isinstance(legacy, bool) or not isinstance(entity, bool): raise ValueError('Invalid initial-state variant')
        if entity_labelled and ('legacy_initial' not in row or 'entity_initial' not in row): raise ValueError('Missing variant labels')
        groups.setdefault((legacy, entity), []).append(row)
    if entity_labelled and set(groups) != {(False,False),(True,False),(False,True),(True,True)}:
        raise ValueError('Expected all four initial-state variants')
    if legacy_labelled and not entity_labelled and (set(groups) != {(False,False),(True,False)} or any('legacy_initial' not in row for row in rows)):
        raise ValueError('Expected both explicitly labelled initial-state variants')
    for variant, events in groups.items():
        if len(events) != 16 or [r.get('event') for r in events] != list(range(16)):
            raise ValueError('Expected exactly events 0–15 in order per variant: ' + str(variant))
    fields = PUBLICATION_FIELDS if mode == 'publication' else FIELDS
    bounded = PUBLICATION_BOUNDED if mode == 'publication' else BOUNDED
    expected_scope = 'private_neighbor_moves_publication_source_schema8' if mode == 'publication' else 'private_overlapping_surface_moves'
    for row in rows:
        row_scope = expected_scope.replace('schema8','schema9') if mode == 'publication' and row.get('entity_initial', False) else expected_scope
        if row.get('scope') != row_scope or (mode == 'surface' and row.get('rank') != 0):
            raise ValueError('Unexpected private event scope or rank')
        for key in fields:
            value = row.get(key)
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or (value < 0 if key == 'face_pieces' and row.get('entity_initial', False) else value <= 0):
                raise ValueError('Invalid positive finite field: ' + key)
        if mode == 'publication':
            if row.get('entity_initial', False) and row['face_pieces'] != 0: raise ValueError('Entity variant retains face covers')
            if row['arithmetic_work_excludes_decode'] != row['operator_work'] + row['publication_work'] + row['source_envelope_work']:
                raise ValueError('Inconsistent summed work')
            row['combined_admitted_peak_bytes'] = row.get('aggregate_admitted_peak_bytes', max(row['operator_peak_bytes'], row['publication_peak_bytes'], row['source_envelope_peak_bytes']))
    build_pins = json.loads(receipt_pins.read_text())
    matches = [(path, digest) for path, digest in build_pins.items() if path.endswith('/UnitTests/Common/adaptation/CNativeCurvedAtlas3D_tests.cpp')]
    if len(matches) != 1: raise ValueError('Missing unique recorded test source hash')
    phases = []
    for variant, events in groups.items():
        for phase in range(4):
            baseline = events[4 + phase]
            for event in range(8 + phase, 16, 4):
                compared = {key: {'baseline': baseline[key], 'value': events[event][key],
                                  'within_bound': events[event][key] <= baseline[key]} for key in bounded}
                phases.append({'legacy_initial': variant[0], 'entity_initial': variant[1], 'phase': phase, 'baseline_event': 4 + phase,
                               'event': event, 'fields': compared})
    if any(not field['within_bound'] for comparison in phases for field in comparison['fields'].values()):
        raise ValueError('Per-phase cost or descriptor bound failed')
    if any(sha(Path(p)) != h for p, h in pins.items()): raise RuntimeError('Inputs changed during summary')
    save(output, {'scope': ('private_neighbor_moves_publication_source_schema8_or_9' if entity_labelled else expected_scope) + '; excludes CFD transfer and solution restart',
                  'arithmetic_note': 'publication arithmetic excludes archive decode work' if mode == 'publication' else 'surface move arithmetic only',
                  'test_source_pin': {'path': matches[0][0], 'sha256': matches[0][1]},
                  'rss_note': 'lifetime process peak RSS, not allocation peak', 'sha256': pins,
                  'rows': rows, 'ranges': {key: {'min': min(r[key] for r in rows),
                                                  'max': max(r[key] for r in rows)} for key in fields + (('combined_admitted_peak_bytes',) if mode == 'publication' else ())},
                  'phase_comparisons': phases})
    print(output)


if __name__ == '__main__': main()
