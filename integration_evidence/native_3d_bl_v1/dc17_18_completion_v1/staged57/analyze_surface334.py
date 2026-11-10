"""Summarize bounded intrinsic surface forms; these are not edge lengths or the composed flow target."""
import hashlib,json,math
from pathlib import Path
R=Path(__file__).resolve().parent
case=R/'cases/curved334_surface_components_20261110T222410_4141734'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
status=json.loads((case/'status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
context=R/'m6_bl_inputs/chord331_from330_v1/actual_contexts.json'
contexts=json.loads(context.read_text())['contexts']
records=[json.loads(line) for line in (case/'run.log').read_text().splitlines() if line.startswith('{')]
rows=[row for row in records if row.get('surface_component')]
assert len(rows)==27 and len(contexts)==5
keys=set()
for row in rows:
    key=(row['edge'],row['side'],row['location']);assert key not in keys;keys.add(key)
    assert row['diagnostic_not_edge_quadrature']
    assert [row['a'],row['b']]==contexts[row['edge']]['edge']
    for name in ('inherited_tangent_q','base_full_q','full_D1_q','controlled_flow_floor_tangent_q'):
        assert math.isfinite(row[name]) and row[name]>0,(key,name)
    assert math.isfinite(row['normal_h0_q']) and row['normal_h0_q']>=0
    residual=abs(row['base_full_q']-row['normal_h0_q']-row['inherited_tangent_q'])
    assert residual<=1e-7*max(1,row['base_full_q']),(key,residual)
summary=[]
for edge,c in enumerate(contexts):
    samples=[row for row in rows if row['edge']==edge];assert samples
    for side in {row['side'] for row in samples}:
        assert {row['location'] for row in samples if row['side']==side}=={0,1,2}
    bounds=lambda name,root=False:[min(math.sqrt(row[name]) if root else row[name] for row in samples),max(math.sqrt(row[name]) if root else row[name] for row in samples)]
    summary.append(dict(label=c['label'],edge=c['edge'],samples=len(samples),sides=sorted({row['side'] for row in samples}),
        sqrt_inherited_tangent_form=bounds('inherited_tangent_q',True),sqrt_full_chord_form=bounds('base_full_q',True),
        sqrt_normal_h0_form=bounds('normal_h0_q',True),curvature_range=[min(k for row in samples for k in row['curvature']),max(k for row in samples for k in row['curvature'])]))
completion=next(row for row in records if row.get('stage')=='surface_components_complete')
assert completion['count']==len(rows) and completion['geometry_work']==3*len(rows)
result=dict(scope='Original M6 sided endpoint/face-centroid quadratic forms only. No mesh mutation, edge quadrature, actual flow-Hessian replay, geometry/height certificate, global complexity decomposition or BL qualification.',
    source_pins={str(p):sha(p) for p in (context,case/'pins.json',case/'run.log',case/'status.json',case/'terminal_handles_actual.json',Path(__file__))},
    completion=completion,contexts=summary,
    confirmed='Full-volume normal-height term dominates these root and smooth-wall chord forms; inherited tangent forms remain near one. The selected spanwise leading-edge edge has little normal contribution.',
    unresolved=['Actual flow/geometry/height target combination and gradation','Intrinsic directional integration and principal-direction alignment','Global inherited-tangent contribution to provisional 1.175M complexity','Admissible coupled surface/volume replacements and first-child clearance','Full-target final chord-size compliance after tangential preparation'])
(R/'surface_components334_assessment.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
