"""Summarize actual geometry eligibility; never promote identity controls to production metric acceptance."""
import hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parent;case=R/'cases/curved337_surface_cavities_20261110T225500_31104'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
rows=[json.loads(line) for line in (case/'actual_bl_surface_cavities.jsonl').read_text().splitlines()]
assert rows[-1]['complete'] and rows[-1]['contexts']==5 and rows[-1]['trials']==40
assert not rows[-1]['mesh_published'] and not rows[-1]['production_metric_certified']
summary=[];trials=[];controls={}
for index,row in enumerate(rows):
    if row.get('operation')=='unchanged_control':controls[row['context']]=row
    if 'trial' not in row:continue
    old=controls[row['context']];out=dict(row)
    if row['patch_measurement_complete']:
        fresh=rows[index-1];assert fresh['context']==row['context'] and fresh['operation']==row['operation'] and 'trial' not in fresh
        assert not fresh['production_metric_certified']
        out.update(removed_points=old['points']-fresh['points'],geometry_bad_faces_before=old['geometry_bad_faces'],geometry_bad_faces_after=fresh['geometry_bad_faces'],
            maximum_geometry_upper_before=old['maximum_geometry_error'],maximum_geometry_upper_after=fresh['maximum_geometry_error'],
            euclidean_control_q_bad_delta=fresh['euclidean_control_q_bad']-old['euclidean_control_q_bad'],euclidean_control_J_bad_delta=fresh['euclidean_control_J_bad']-old['euclidean_control_J_bad'],
            requested_first_height_pass=fresh['requested_first_height_pass'])
    trials.append(out)
assert len(trials)==40
for label,old in controls.items():
    t=[row for row in trials if row['context']==label]
    assert len(t)==8 and {row['trial'] for row in t}==set(range(8))
    summary.append(dict(context=label,old_cells=old['cells'],old_points=old['points'],old_geometry_bad_faces=old['geometry_bad_faces'],old_geometry_upper=old['maximum_geometry_error'],
        constructed=sum(row['constructed'] for row in t),clear=sum(row['external_clearance'] for row in t),complete=sum(row['patch_measurement_complete'] for row in t),
        geometry_clean_after=sum(row.get('geometry_bad_faces_after',-1)==0 for row in t),trials=t))
result=dict(scope='Actual original303 M6 geometry/topology/body-clearance eligibility only. Shape values use Euclidean identity controls, NOT the actual BL/flow metric. No first layer constructed or mesh published.',
    counts=dict(contexts=5,trials=40,constructed=sum(row['constructed'] for row in trials),clear=sum(row['external_clearance'] for row in trials),measured=sum(row['patch_measurement_complete'] for row in trials)),
    patch_geometry_work=rows[-1]['geometry_work'],mobility_setup_work=181938600,
    contexts=summary,
    confirmed=['All planned bounded supports completed without work/time exhaustion.','Existing operators can produce positive externally clear replacements on actual M6 surface/volume stars.','Smooth-upper directed collapses fail strict geometry while other sampled smooth operations retain it.','Selected root/LE original contexts already fail strict geometry; removing faces is not proof of closure.','Symmetry trial0 removes one point with zero geometry failures and unchanged zero Euclidean q/J failures.'],
    unresolved=['Geometry error values are conservative upper certificates; gate failure alone does not prove the exact physical error exceeds tolerance.','Exact same303 production sensor/combined-target conformity and metric q/J are unproven.','Requested first height fails on all original/pre-layer controls and measured replacements; no coherent BL constructed.','Global inherited-tangent complexity contribution, physical curvature directions, front coverage/progression and native MPI BL generation remain unproven.'],
    pins={str(p):sha(p) for p in (case/'pins.json',case/'status.json',case/'actual_bl_surface_cavities.jsonl',case/'mapped_runtime_actual.json',case/'terminal_handles_actual.json',Path(__file__))})
(R/'surface_cavities337_assessment.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result['counts']));print(json.dumps([dict(context=x['context'],constructed=x['constructed'],old_geometry_bad_faces=x['old_geometry_bad_faces']) for x in summary]))
