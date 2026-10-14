#!/usr/bin/env python3
"""Audit the captured rejected patch, not curved metric or whole-event acceptance."""
import collections,hashlib,itertools,json,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant,overlap

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def faces(v):
 a,b,c,d=v;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def frame(path):
 data=json.loads(path.read_text());nodes={r[0]:tuple(map(F,r[1:])) for r in data['nodes']}
 assert len(nodes)==len(data['nodes'])
 uses=collections.defaultdict(list);volume=F(0);tets=[]
 for c in data['cells']:
  t=[nodes[n] for n in c['nodes']];v=determinant(t)/6;assert v>0;volume+=v;tets.append(t)
  for face in faces(c['nodes']):uses[tuple(sorted(face))].append((-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3)))
 assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in uses.values())
 boundary={f for f,v in uses.items() if len(v)==1};physical={tuple(sorted(f['nodes'])) for f in data['surface']}
 assert physical<=boundary and len(physical)==len(data['surface'])
 for a,b in itertools.combinations(tets,2):assert not overlap(a,b),'positive-volume overlap'
 return data,nodes,boundary-physical,volume

def main():
 folder=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else HERE/'curved_cold'
 receipt_path=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else HERE/'curved_cold.json'
 receipt=json.loads(receipt_path.read_text());assert receipt['status']=='FAIL'
 inputs={k:folder/('curved_rejected_patch_rank0_'+k+'.json') for k in ('old','proposal','diagnosis')}
 for p in inputs.values():assert sha(p)==receipt['artifacts'][p.name]
 old,on,interface,ov=frame(inputs['old']);fresh,fn,new_interface,fv=frame(inputs['proposal']);diag=json.loads(inputs['diagnosis'].read_text())
 assert diag['geometry_valid'] and diag['metric_failure']==2
 assert interface==new_interface
 assert all(on[n]==fn[n] for f in interface for n in f)
 edge=set(diag['feature'][:diag['feature_count']]);assert len(edge)==2
 assert not any(edge<=set(f) for f in interface),'selected boundary edge star is incomplete'
 bad=next(c for c in fresh['cells'] if c['id']==diag['weakest_cell'])
 fixed={n for f in interface for n in f};physical={n for f in fresh['surface'] for n in f['nodes']}
 report=dict(status='REPRODUCED_PRIVATE_SHAPE_REJECTION',scope='Exact positive volumes, no positive-volume pair overlap, oriented incidence and unchanged artificial interface of captured patches. Native recorded q/J only: no independent full composed-metric or smooth-wall certificate and no whole-event acceptance.',
  old_cells=len(old['cells']),proposal_cells=len(fresh['cells']),old_volume=str(ov),proposal_volume=str(fv),diagnosis=diag,
  weakest_cell=bad,weakest_nodes=[dict(id=n,point=list(map(float,fn[n])),fixed_interface=n in fixed,physical=n in physical) for n in bad['nodes']],
  selected_star_complete_inside_patch=True,receipt_sha256=sha(receipt_path),input_sha256={k:sha(v) for k,v in inputs.items()},
  helper_sha256=sha(Path(__file__)),predicate_helper_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py'))
 output=Path(sys.argv[3]) if len(sys.argv)>3 else HERE/'curved_cavity_audit.json';output.write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
