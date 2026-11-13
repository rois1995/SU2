"""Assess uncommitted actual residual candidates; no mesh acceptance or geometry-truth inference."""
import hashlib,json,re
from pathlib import Path
from assess_surface_transaction470 import terminal_receipt,owned_rows
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
case=max((R/'cases').glob('curved472_current_residual_trace_serial_*'),key=lambda p:p.stat().st_mtime)
status=json.loads((case/'status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
pins=json.loads((case/'pins.json').read_text())
for p,h in pins.items():assert sha(p)==h,p
handles=terminal_receipt(case);(case/'terminal_handles_actual.json').write_text(json.dumps(handles,indent=2)+'\n')
log=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text())
passed=re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',log)
assert len(passed)==1 and int(passed[0][1])==1 and int(passed[0][0])>607514,passed
def no_constant(value):raise ValueError('nonfinite JSON '+value)
records=[json.loads(l,parse_constant=no_constant) for l in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
contexts=[r for r in records if r.get('stage')=='current_residual_context']
ops=[r for r in records if r.get('stage')=='operator_preflight']
results=[r for r in records if r.get('stage')=='current_residual_result']
assert len(results)==1 and results[0]['constructor_calls']==len(ops)<=24
for op in ops:
    assert op['old_cells']<=64 and op['new_cells']<=128 and not op['published'] and not op['mesh_accepted']
    if op['private_feasible']:
        for key in ('constructed','complete_removed_stars','final_support_reserved','passing_heights_preserved','provisional_height','geometry_guidance_pass','metric_admission','parameter_preflight','contact_protocol','contact_pass'):assert op[key],(op['context'],key)
source=Path(json.loads((R/'surface_stage470_assessment.json').read_text())['case'])
expected={}
for rank in range(6):
    for identity,digest in owned_rows(source/f'stage_surface_rank{rank}_owned.bin'):
        assert identity not in expected;expected[identity]=digest
matched=0
for identity,digest in owned_rows(case/'correction_unchanged_owned.bin'):
    assert expected.pop(identity)==digest;matched+=1
assert not expected and matched==303757
summary=[]
for op in ops:
    failures=[]
    if not op['constructed']:failures.append('constructor_refusal')
    else:
        for key,label in [('complete_removed_stars','incomplete_removed_star'),('passing_heights_preserved','passing_height_or_old_measurement'),('provisional_height','unresolved_clearance_or_missing_height_child'),('geometry_guidance_pass','geometry_guidance_upper_bound_refusal_not_proven_violation'),('metric_admission','surface_metric_admission'),('parameter_preflight','association_preflight'),('contact_protocol','contact_protocol_unresolved'),('contact_pass','external_contact_refusal')]:
            if not op[key]:failures.append(label)
    summary.append(dict(context=op['context'],operator=op['operator'],constructed=op['constructed'],private_feasible=op['private_feasible'],failures=failures,reason=op['reason'],
        old_geometry_bad=op['old_geometry_bad_cells'] if op['geometry_counts_measured'] else None,new_geometry_bad=op['new_geometry_bad_cells'] if op['geometry_counts_measured'] else None,
        old_q_bad=op['old_q_bad'] if op['centroid_shape_measured'] else None,new_q_bad=op['new_q_bad'] if op['centroid_shape_measured'] else None,
        old_J_bad=op['old_J_bad'] if op['centroid_shape_measured'] else None,new_J_bad=op['new_J_bad'] if op['centroid_shape_measured'] else None))
controls=[r for r in contexts if r['context']=='unchanged_wall_control']
report=dict(case=str(case),status=status,pins_verified=len(pins),actual_handles_terminal=True,original470Owned_records_byte_identical=matched,
    diagnostic_test_assertions=int(passed[0][0]),contexts=contexts,operations=ops,summary=summary,result=results[0],
    all_requested_contexts_measured=len(contexts)==3 and all(r['native_input_topology'] and r['geometry_complete'] for r in contexts),
    geometry_clean_control_measured=bool(controls and controls[0]['geometry_complete'] and controls[0]['geometry_excess']==0),
    search_refusals=[r for r in records if r.get('stage') in ('current_residual_search_refused','current_residual_unresolved')],
    feasible_unpublished_candidates=sum(bool(r['private_feasible']) for r in ops),production_changed=False,goal_complete=False,
    limits=['Private surface-planning admission only; no pre-constructor reservation or publication claim.',
    'No full composed-volume edge audit or final bidirectional geometry certificate.',
    'Upper-bound guidance refusal and negative clearance lower bound do not prove a geometric violation.',
    'Centroid q/J diagnostics do not establish final whole-field shape acceptance.',
    'Bounded existing operator composition is narrower than reference implementations; no pyAMG equivalence/superiority claim.'])
(R/'current_residual472_trace_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ('contexts','summary','result','all_requested_contexts_measured','geometry_clean_control_measured','feasible_unpublished_candidates','original470Owned_records_byte_identical')},indent=2))
