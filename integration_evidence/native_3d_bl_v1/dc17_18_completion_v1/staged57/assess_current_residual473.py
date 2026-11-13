"""Assess frozen 473 actual residual operator records against 472; no acceptance claim."""
import hashlib
import json
import math
import re
from pathlib import Path
from assess_surface_transaction470 import terminal_receipt, owned_rows

R = Path(__file__).resolve().parent
sha = lambda p: hashlib.file_digest(Path(p).open('rb'), 'sha256').hexdigest()
case = max((R / 'cases').glob('curved473_current_residual_serial_*'), key=lambda p: p.stat().st_mtime)
status = json.loads((case / 'status.json').read_text())
assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
pins = json.loads((case / 'pins.json').read_text())
for path, digest in pins.items():
    assert sha(path) == digest, path
handles = terminal_receipt(case)
(case / 'terminal_handles_actual.json').write_text(json.dumps(handles, indent=2) + '\n')

def no_constant(value):
    raise ValueError('nonfinite JSON ' + value)

records = [json.loads(line, parse_constant=no_constant)
           for line in (case / 'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
contexts = [row for row in records if row.get('stage') == 'current_residual_context']
ops = [row for row in records if row.get('stage') == 'operator_preflight']
results = [row for row in records if row.get('stage') == 'current_residual_result']
assert len(results) == 1 and results[0]['constructor_calls'] == len(ops) <= 24
for op in ops:
    assert op['old_cells'] <= 64 and op['new_cells'] <= 128
    assert not op['published'] and not op['mesh_accepted']
    assert op['geometry_work'] >= 0 and op['admitted_peak_bytes'] >= 0
    if op['private_feasible']:
        for key in ('constructed', 'complete_removed_stars', 'final_support_reserved',
                    'passing_heights_preserved', 'provisional_height', 'geometry_guidance_pass',
                    'metric_admission', 'parameter_preflight', 'contact_protocol', 'contact_pass'):
            assert op[key], (op['context'], op['operator'], key)

# The unchanged capture must cover every original surface Owned record byte for byte.
source = Path(json.loads((R / 'surface_stage470_assessment.json').read_text())['case'])
expected = {}
for rank in range(6):
    for identity, digest in owned_rows(source / f'stage_surface_rank{rank}_owned.bin'):
        assert identity not in expected
        expected[identity] = digest
matched = 0
owned_export = case / 'correction_unchanged_owned.bin'
for identity, digest in owned_rows(owned_export):
    assert expected.pop(identity) == digest
    matched += 1
assert not expected and matched == 303757

old_report_path = R / 'current_residual472_assessment.json'
old_report = json.loads(old_report_path.read_text())
old_ops = old_report['operations']
def index_rows(rows):
    indexed = {}
    occurrences = {}
    for row in rows:
        pair = (row.get('context'), row.get('operator'))
        n = occurrences.get(pair, 0)
        occurrences[pair] = n + 1
        indexed[(pair[0], pair[1], n)] = row
    return indexed
old_index = index_rows(old_ops)
new_index = index_rows(ops)
comparison = []
score_fields = ('old_geometry_excess', 'new_geometry_excess',
                'old_geometry_bad_cells', 'new_geometry_bad_cells',
                'old_q_bad', 'new_q_bad', 'old_J_bad', 'new_J_bad',
                'old_planning_max', 'new_planning_max')
flag_fields = ('constructed', 'geometry_guidance_pass', 'geometry_counts_measured',
               'metric_admission', 'parameter_preflight', 'contact_protocol', 'contact_pass',
               'final_support_reserved', 'provisional_height', 'private_feasible', 'published')

def measured(row, field):
    if row is None or field not in row:
        return None
    if field.endswith('_geometry_bad_cells') and not row.get('geometry_counts_measured'):
        return None
    if field in ('old_q_bad', 'new_q_bad', 'old_J_bad', 'new_J_bad') and not row.get('centroid_shape_measured'):
        return None
    value = row[field]
    return value if isinstance(value, (int, float)) and math.isfinite(value) and value >= 0 else None

for key in sorted(set(old_index) | set(new_index)):
    old = old_index.get(key)
    new = new_index.get(key)
    values = {}
    for field in score_fields:
        values['old472_' + field] = measured(old, field)
        values['new473_' + field] = measured(new, field)
        a, b = values['old472_' + field], values['new473_' + field]
        values['delta473_minus472_' + field] = b - a if a is not None and b is not None else None
    values['old472_flags'] = {f: old.get(f) for f in flag_fields} if old else None
    values['new473_flags'] = {f: new.get(f) for f in flag_fields} if new else None
    values['old472_geometry_work'] = old.get('geometry_work') if old else None
    values['new473_geometry_work'] = new.get('geometry_work') if new else None
    values['old472_admitted_peak_bytes'] = old.get('admitted_peak_bytes') if old else None
    values['new473_admitted_peak_bytes'] = new.get('admitted_peak_bytes') if new else None
    values['old472_reason'] = old.get('reason') if old else None
    values['new473_reason'] = new.get('reason') if new else None
    comparison.append({'context': key[0], 'operator': key[1], 'occurrence': key[2],
                       'match': 'both' if old and new else 'old_only' if old else 'new_only', **values})

# The 473 fixture deliberately skips ConstrainMovePoint for the mapped residual control.
# Witness coordinates let us report whether its operator candidate actually moved and by how much.
def witness_nodes(rows, context, operator, side):
    out = {}
    for row in rows:
        if (row.get('stage') != 'centroid_directional_witness' or row.get('context') != context or
                row.get('operator') != operator or row.get('side') != side):
            continue
        for node in row['vertices']:
            ident = node[0]
            xyz = tuple(float(v) for v in node[1:])
            if ident in out:
                assert out[ident] == xyz
            out[ident] = xyz
    return out

def control_projection(report_rows, context_rows, name):
    context = next((row for row in context_rows if row.get('context') == 'unchanged_wall_control'), None)
    old_nodes = witness_nodes(report_rows, 'unchanged_wall_control', 'MoveVertex', 'old')
    fresh_nodes = witness_nodes(report_rows, 'unchanged_wall_control', 'MoveVertex', 'fresh')
    out = {'operator': 'MoveVertex', 'context': 'unchanged_wall_control', 'evidence_source': name,
           'old_witness_node_count': len(old_nodes), 'fresh_witness_node_count': len(fresh_nodes)}
    if context is None:
        out['available'] = False
        out['reason'] = 'No unchanged wall control context record.'
        return out
    vertex = context.get('vertex')
    old_xyz = old_nodes.get(vertex)
    added = sorted(set(fresh_nodes) - set(old_nodes))
    out.update({'control_vertex': vertex, 'old_control_xyz': old_xyz,
                'new_node_ids_in_fresh_witness': added})
    if old_xyz is not None and added:
        new_id = added[0]
        new_xyz = fresh_nodes[new_id]
        out.update({'available': True, 'fresh_moved_node_id': new_id, 'fresh_moved_xyz': new_xyz,
                    'movement_distance_from_original': math.dist(old_xyz, new_xyz)})
        if len(added) == 1:
            # Raw proposal recipe from the test fixture; compare the observed fresh coordinate to it.
            neighbors = [old_nodes[i] for i in sorted(old_nodes) if i != vertex]
            if neighbors:
                count = len(neighbors)
                avg = [sum(p[d] for p in neighbors) for d in range(3)]
                raw = tuple(.9 * old_xyz[d] + .1 * avg[d] / count for d in range(3))
                out['raw_unprojected_proposal_xyz'] = raw
                out['fresh_matches_raw_proposal'] = all(abs(a-b) <= 8*math.ulp(max(1.0, abs(a), abs(b)))
                                                        for a,b in zip(new_xyz, raw))
    else:
        out.update({'available': False,
                    'reason': 'No constructed MoveVertex fresh witness; do not infer a projected coordinate.'})
    return out

old_contexts = old_report['contexts']
projection = {
    'old472': control_projection(old_report['operations'] + old_report.get('centroid_directional_witnesses', []),
                                 old_contexts, 'current_residual472_assessment.json'),
    'new473': control_projection(records, contexts, 'current_residual473 actual planning JSONL')
}
# Old 472 assessment stores operation/summary records but not witness rows; load its original JSONL if present.
old_case = Path(old_report['case'])
old_jsonl = old_case / 'actual_bl_cavity_planning.jsonl'
if old_jsonl.exists():
    old_records = [json.loads(line, parse_constant=no_constant) for line in old_jsonl.read_text().splitlines()]
    projection['old472'] = control_projection(old_records, old_contexts, str(old_jsonl))

summary = []
for op in ops:
    failures = []
    if not op['constructed']:
        failures.append('constructor_refusal')
    else:
        for key, label in (('complete_removed_stars', 'incomplete_removed_star'),
                            ('passing_heights_preserved', 'passing_height_or_old_measurement'),
                            ('provisional_height', 'unresolved_clearance_or_missing_height_child'),
                            ('geometry_guidance_pass', 'geometry_guidance_upper_bound_refusal_not_proven_violation'),
                            ('metric_admission', 'surface_metric_admission'),
                            ('parameter_preflight', 'association_preflight'),
                            ('contact_protocol', 'contact_protocol_unresolved'),
                            ('contact_pass', 'external_contact_refusal')):
            if not op.get(key, False):
                failures.append(label)
    summary.append({'context': op['context'], 'operator': op['operator'],
                    'constructed': op['constructed'], 'private_feasible': op['private_feasible'],
                    'failures': failures, 'reason': op['reason'],
                    'old_geometry_excess': measured(op, 'old_geometry_excess'),
                    'new_geometry_excess': measured(op, 'new_geometry_excess'),
                    'old_geometry_bad': measured(op, 'old_geometry_bad_cells'),
                    'new_geometry_bad': measured(op, 'new_geometry_bad_cells'),
                    'old_q_bad': measured(op, 'old_q_bad'), 'new_q_bad': measured(op, 'new_q_bad'),
                    'old_J_bad': measured(op, 'old_J_bad'), 'new_J_bad': measured(op, 'new_J_bad'),
                    'old_planning_max': measured(op, 'old_planning_max'),
                    'new_planning_max': measured(op, 'new_planning_max'),
                    'geometry_work': op['geometry_work'], 'admitted_peak_bytes': op['admitted_peak_bytes']})
controls = [row for row in contexts if row['context'] == 'unchanged_wall_control']
report = {
    'case': str(case), 'status': status, 'pins_verified': len(pins),
    'actual_handles_terminal': True, 'original470Owned_records_byte_identical': matched,
    'contexts': contexts, 'operations': ops, 'summary': summary, 'result': results[0],
    'old472_to_new473_operator_comparison': {
        'old_case': str(old_case), 'old_assessment_sha256': sha(old_report_path),
        'old_operation_count': len(old_ops), 'new_operation_count': len(ops),
        'matched_operator_keys': sum(1 for row in comparison if row['match'] == 'both'),
        'old_only_operator_keys': sum(1 for row in comparison if row['match'] == 'old_only'),
        'new_only_operator_keys': sum(1 for row in comparison if row['match'] == 'new_only'),
        'rows': comparison,
        'control_projection': projection,
        'interpretation': 'Measurements are reported as observed. No operator is presumed to improve; a failed upper-bound guidance check is not a proof of physical violation.'},
    'all_requested_contexts_measured': all(
        any(row.get('context') == name and row.get('native_input_topology') and row.get('geometry_complete')
            for row in contexts)
        for name in ('unchanged_wall_control', 'current_rank4_geometry', 'current_rank5_clearance')),
    'geometry_clean_control_measured': bool(controls and controls[0]['geometry_complete'] and controls[0]['geometry_excess'] == 0),
    'search_refusals': [row for row in records if row.get('stage') in ('current_residual_search_refused', 'current_residual_unresolved')],
    'feasible_unpublished_candidates': sum(bool(row['private_feasible']) for row in ops),
    'production_changed': True, 'production_qualified': False, 'goal_complete': False,
    'limits': ['Private actual-surface operator preflight only; no publication or mesh acceptance.',
               'Original Owned equality applies to the saved surface snapshot; it does not establish output mesh quality.',
               'Per-operator counts and scores are local to each measured patch, not whole-field quality deltas.',
               'No full composed-volume edge audit, final bidirectional geometry certificate, or first-row readiness claim.',
               'The 1e-4 geometry acceptance gate, q/J/size/height/body/resource gates remain unchanged.',
               'Native serial 52-tag test suite is reported separately; it is not required to parse this residual result.']}
output = R / 'current_residual473_assessment.json'
assert not output.exists(), 'Preserve existing evidence rather than overwrite.'
output.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'case': str(case), 'operations': len(ops), 'contexts': len(contexts),
                  'original_owned_rows_byte_identical': matched,
                  'matched_operator_keys': report['old472_to_new473_operator_comparison']['matched_operator_keys'],
                  'feasible_unpublished_candidates': report['feasible_unpublished_candidates']}, indent=2))
