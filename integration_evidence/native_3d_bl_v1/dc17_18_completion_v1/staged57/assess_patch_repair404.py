"""Bounded saved-target patch feasibility evidence; no accepted-mesh or global-publication claim."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
import assess_saved_row369 as previous
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    case=Path(sys.argv[1]).resolve();status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['pins_verified']
    terminal=json.loads((case/'terminal_handles_actual.json').read_text());assert terminal['all_owned_handles_terminal']
    paths=[case/x for x in ('status.json','command.json','actual_bl_saved_tangent.jsonl','actual_bl_front.jsonl')]
    rows=[json.loads(x) for x in paths[2].read_text().splitlines()]
    trials=[x for x in rows if x['context']=='saved_row_actual_field_repair'];assert len(trials)<=1
    measured={x['context']:x for x in rows if x.get('status')=='measured'}
    base=measured['saved_prepared_repair_before'];assert base['strict_geometry_pass'] and base['strict_first_height_pass']
    assert base['positive_cells']==25 and base['nonpositive_cells']==0
    trial=trials[0] if trials else None;replacement=measured.get('saved_row_actual_field_replacement')
    command=json.loads(paths[1].read_text());patch_path=Path(command['environment']['SU2_NATIVE_BL_SAVED_PATCH']);paths.append(patch_path)
    summaries=[]
    for label,patch in [('saved_prepared_repair_before',patch_path),('saved_row_actual_field_replacement',case/'saved_row_actual_field_replacement.bin')]:
        if label not in measured:continue
        frame_path=case/(label+'_frames.jsonl');paths += [frame_path,patch]
        cells,_=previous.read_patch(patch);frames=[json.loads(x) for x in frame_path.read_text().splitlines()]
        assert len(frames)==len(cells) and {x['cell'] for x in frames}==set(cells)
        bad_q=bad_j=0;max_error=0
        for row in frames:
            ids,xyz=cells[row['cell']];xx,xy,xz,yy,yz,zz=row['metric']
            q,j=previous.quality(xyz,np.array([[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]],dtype=np.longdouble))
            max_error=max(max_error,abs(q-row['q']),abs(j-row['J']));assert max_error<2e-7
            bad_q+=q<.2;bad_j+=j<.05
        raw=measured[label];assert int(bad_q)==raw['wall_q_bad']+raw['other_q_bad'] and int(bad_j)==raw['wall_J_bad']+raw['other_J_bad']
        summaries.append(dict(context=label,cells=len(cells),q_bad=int(bad_q),J_bad=int(bad_j),independent_max_absolute_error=max_error,strict_geometry_pass=raw['strict_geometry_pass'],strict_first_height_pass=raw['strict_first_height_pass'],positive=raw['nonpositive_cells']==0))
    net=None
    if trial and trial['improved']:
        assert replacement is not None and len(summaries)==2
        old,new=summaries;net=dict(q_bad_removed=old['q_bad']-new['q_bad'],J_bad_removed=old['J_bad']-new['J_bad'],cells_removed=old['cells']-new['cells'],q_bad_removed_per_second=(old['q_bad']-new['q_bad'])/trial['seconds'],J_bad_removed_per_second=(old['J_bad']-new['J_bad'])/trial['seconds'])
    result=dict(scope=__doc__,case=str(case),status=status,trial=trial,patches=summaries,net=net,helper_sha256=sha(__file__),input_sha256={str(p):sha(p) for p in paths},limits=['Original396P1field on saved358geometry, not generated-node transported metric reconstruction.','Fixed-skin private feasibility; no global clearance, distributed ownership/version admission or exact metric-edge conformity measured.','Centroid q/J net changes are useful patch diagnostics, not whole-field stage/final acceptance.','No production target/operator/normal law/acceptance gate changed.'])
    output=R/'patch_repair404_assessment.json';assert not output.exists();output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print(json.dumps({k:result[k] for k in ('status','trial','patches','net')}))
if __name__=='__main__':main()
