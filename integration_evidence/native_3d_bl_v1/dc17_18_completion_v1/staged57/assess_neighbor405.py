"""Existing84-cell M6 support, exact saved target: private repair evidence, not a published BL mesh."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
import assess_saved_row369 as previous
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(capture,frames):
    cells,_=previous.read_patch(capture);rows=[json.loads(s) for s in frames.read_text().splitlines()]
    assert len(rows)==len(cells) and {r['cell'] for r in rows}==set(cells)
    result={};error=0
    for row in rows:
        ids,xyz=cells[row['cell']];xx,xy,xz,yy,yz,zz=row['metric'];m=np.array([[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]],dtype=np.longdouble)
        q,j=previous.quality(xyz,m);error=max(error,abs(q-row['q']),abs(j-row['J']));assert error<2e-7
        key=tuple(sorted(tuple(map(float,p)) for p in xyz));assert key not in result
        result[key]=dict(q=q,J=j,wall=row['wall_child'],id=row['cell'])
    return result,dict(cells=len(cells),points=len({i for ids,xyz in cells.values() for i in ids}),q_bad=sum(x['q']<.2 for x in result.values()),J_bad=sum(x['J']<.05 for x in result.values()),maximum_independent_error=error)
def churn(old,new,field,gate):
    retired=old.keys()-new.keys();introduced=new.keys()-old.keys();shared=old.keys()&new.keys()
    out=dict(retired_bad=sum(old[k][field]<gate for k in retired),introduced_bad=sum(new[k][field]<gate for k in introduced),unchanged_fixed=sum(old[k][field]<gate<=new[k][field] for k in shared),unchanged_new_bad=sum(new[k][field]<gate<=old[k][field] for k in shared))
    out['net_removed']=out['retired_bad']-out['introduced_bad']+out['unchanged_fixed']-out['unchanged_new_bad'];return out

def main():
    c=Path(sys.argv[1]).resolve();status=json.loads((c/'status.json').read_text());assert status['state']=='terminal' and status['pins_verified']
    assert json.loads((c/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    log=c/'actual_bl_saved_tangent.jsonl';rows=[json.loads(x) for x in log.read_text().splitlines()]
    measured={x['context']:x for x in rows if x.get('status')=='measured'};old_label='saved_prepared_repair_before_plus_neighbor_star'
    old_paths=[c/'saved_neighbor_actual_field_before.bin',c/(old_label+'_frames.jsonl')];old,old_summary=read(*old_paths);assert old_summary['cells']==84
    assert old_summary['q_bad']==68 and old_summary['J_bad']==68
    trial=next((x for x in rows if x['context']=='saved_neighbor_actual_field_repair'),None)
    paths=[log,c/'status.json',c/'command.json',*old_paths,Path(previous.__file__)];new_summary=None;new_measured=None;preservation=None;net=None;churn_report=None
    if trial and trial['improved']:
        label='saved_neighbor_actual_field_replacement';new_paths=[c/(label+'.bin'),c/(label+'_frames.jsonl')];paths+=new_paths
        new,new_summary=read(*new_paths);new_measured=measured[label]
        assert new_summary['q_bad']==new_measured['wall_q_bad']+new_measured['other_q_bad'] and new_summary['J_bad']==new_measured['wall_J_bad']+new_measured['other_J_bad']
        assert new_measured['nonpositive_cells']==0 and new_measured['strict_geometry_pass']
        preservation=next(x for x in rows if x['context']=='saved_neighbor_actual_field_preservation');assert preservation['passing_children_preserved']
        churn_report=dict(q=churn(old,new,'q',.2),J=churn(old,new,'J',.05))
        assert churn_report['q']['net_removed']==old_summary['q_bad']-new_summary['q_bad'];assert churn_report['J']['net_removed']==old_summary['J_bad']-new_summary['J_bad']
        net=dict(q_bad_removed=churn_report['q']['net_removed'],J_bad_removed=churn_report['J']['net_removed'],points_removed=old_summary['points']-new_summary['points'],cells_removed=old_summary['cells']-new_summary['cells'],q_bad_removed_per_second=churn_report['q']['net_removed']/trial['seconds'],J_bad_removed_per_second=churn_report['J']['net_removed']/trial['seconds'])
    result=dict(scope=__doc__,case=str(c),status=status,before=old_summary,trial=trial,after=new_summary,after_measurement=new_measured,preservation=preservation,churn=churn_report,net=net,helper_sha256=sha(__file__),input_sha256={str(p):sha(p) for p in paths},limits=['Original396P1field on saved358coordinates, not evolved generated-node metric transport.','Unfinished neighbor height debt may remain; exact after-debt count is not exported, maximum error and independent preservation are reported.','Churn matches exact sorted physical cell coordinates; moved cells retire/introduce, not tracked by renumbered IDs.','No exact edge conformity, global ownership/dependency/clearance admission, publication or accepted mesh claim.'])
    output=R/'neighbor_patch405_assessment.json';assert not output.exists();output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print(json.dumps({k:result[k] for k in ('status','before','trial','after','preservation','churn','net')}))
if __name__=='__main__':main()
