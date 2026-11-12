"""Current-target construction on original M6 geometry; provisional patch evidence only."""
from pathlib import Path
import hashlib,json
from assess_neighbor405 import read,churn
R=Path(__file__).resolve().parent
C=R/'cases/curved409_checkpoint_front_20261112T012159_3789220'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    status=json.loads((C/'status.json').read_text());assert status['exit_code']==0 and status['pins_verified']
    assert json.loads((C/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    rows=[json.loads(s) for s in (C/'actual_bl_front.jsonl').read_text().splitlines()]
    def one(stage):
        found=[r for r in rows if r.get('stage')==stage];assert len(found)==1,(stage,len(found));return found[0]
    summary=rows[-1];assert summary['complete'] and not summary['mesh_published']
    plan=one('surface_planning');initial=one('first_row_result');surface=one('surface_height_result');front=one('prepared_row_result')
    assert plan['maximum_requested_length']<1.8 and not initial['constructed']
    assert initial['class']=='cap' and surface['splits']==16
    assert front['constructed'] and front['geometry_pass'] and front['complete_patch_height'] and front['external_clearance'] and front['passing_rows_preserved']
    assert front['columns']==front['passing_columns']==5 and front['fresh_unknown']==0
    fields={};independent={};paths=[]
    for stage in ('before','connectivity','optimizer'):
        capture=C/f'prepared_repair_{stage}.bin';frames=C/f'prepared_repair_{stage}_frames.jsonl'
        fields[stage],independent[stage]=read(capture,frames);paths.extend((capture,frames))
        assert independent[stage]['q_bad']==independent[stage]['J_bad']==25
    geometry=[json.loads(s) for s in (C/'actual_bl_surface_cavities.jsonl').read_text().splitlines()]
    fresh=next(x for x in geometry if x['operation']=='prepared_row_fresh')
    assert fresh['geometry_bad_faces']==0 and fresh['requested_first_height_pass']
    result=dict(scope=__doc__,case=str(C),status=status,planning=plan,initial_front=initial,surface=surface,prepared_front=front,
        prepared_geometry_height=fresh,independent=independent,repair=one('prepared_connectivity_repair'),optimizer=one('prepared_optimizer'),
        churn=dict(q=churn(fields['before'],fields['connectivity'],'q',.2),J=churn(fields['before'],fields['connectivity'],'J',.05)),
        counters=one('existing_counters'),exclusion=one('edge_exclusion_counters'),geometry_counters=one('existing_geometry_counters'),
        complete=summary,production_code_changed=False,source_CFD_recomputed=False,usable_BL=False,
        next='Coupled surface readiness must account for actual first-row height/clearance feasibility; tangential size alone is insufficient. Reuse existing operators. Do not select a new metric or larger cavities from this one context.',
        limits=['One smooth-upper context, not leading-edge/root/feature qualification.',
                'Original saved396 P1 field during private construction, not accepted evolved nodal transport.',
                '14/21 descendant faces have sampled feasible feet; seven-point sampling is not a proof that others are infeasible.',
                'Five passing columns are a local candidate, not full wall coverage, growth, transition or final q/J/size acceptance.',
                'Optimizer proposal fails oriented interface and was never admitted.',
                'Maximum tilt is apex offset from triangle centroid divided by h0; it is not independently a column-alignment angle.',
                'Nested timers overlap. No speedup or native-MPI BL generation claim.'])
    paths += [C/n for n in ('pins.json','command.json','status.json','terminal_handles_actual.json','actual_bl_front.jsonl','actual_bl_surface_cavities.jsonl','prepared_repair_parameters.bin')]
    paths += [Path(__file__),R/'assess_neighbor405.py',R/'assess_saved_row369.py']
    result['input_sha256']={str(p):sha(p) for p in paths}
    output=R/'front409_assessment.json';assert not output.exists();output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print(json.dumps(dict(status=status,planning=plan,prepared_front=front,independent=independent)))
if __name__=='__main__':main()
