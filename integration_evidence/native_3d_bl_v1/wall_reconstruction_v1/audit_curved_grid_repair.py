#!/usr/bin/env python3
"""Audit the actual engine edit against the identical independently audited base grid."""
import hashlib,itertools,json
from pathlib import Path
from audit_curved_cavity import frame
from audit_curved_grid import footprints,cross,sub,dot,overlap
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 folder=HERE/'curved_grid_refine';rp=HERE/'curved_grid_refine.json';receipt=json.loads(rp.read_text());assert receipt['status']=='PASS'
 oldpath=folder/'audited_grid.json';newpath=folder/'curved_grid_partial_rejected.json'
 for p in [oldpath,newpath,folder/'grid_refine_summary.json']:assert sha(p)==receipt['artifacts'][p.name]
 baseline=json.loads((HERE/'curved_grid_independent.json').read_text());br=HERE/'curved_grid_audit.json';base=json.loads(br.read_text())
 assert baseline['status']=='REJECTED_LENGTH_TARGET_GEOMETRY_SHAPE_PASS'
 assert sha(br)==baseline['native_receipt_sha256'] and sha(oldpath)==base['artifacts']['audited_grid.json']
 assert sha(Path(__file__).with_name('audit_curved_grid.py'))==baseline['helper_sha256']
 predicates=HERE.parents[2]/'integration_evidence/native_3d_core_v1/audit_cavities.py'
 assert sha(predicates)==baseline['predicates_sha256']
 old=json.loads(oldpath.read_text());new=json.loads(newpath.read_text())
 assert old['original']==new['original'] and old['facets']==new['facets'] and old['facet_groups']==new['facet_groups'] and old['marker_names']==new['marker_names']
 on={n[0]:tuple(n[1:]) for n in old['nodes']};nn={n[0]:tuple(n[1:]) for n in new['nodes']}
 assert all(on[n]==nn[n] for n in on.keys()&nn.keys()),'stationary identity moved'
 oc={tuple(sorted(c['nodes'])):c for c in old['cells']};nc={tuple(sorted(c['nodes'])):c for c in new['cells']}
 removed=[oc[k] for k in oc.keys()-nc.keys()];added=[nc[k] for k in nc.keys()-oc.keys()]
 assert removed and added and len(removed)<=128 and len(added)<=128
 patches=[]
 for label,data,cells in [('old',old,removed),('proposal',new,added)]:
  uses={tuple(sorted(f)) for c in cells for f in itertools.combinations(c['nodes'],3)}
  patch=dict(data,cells=cells,surface=[f for f in data['surface'] if tuple(sorted(f['nodes'])) in uses])
  path=folder/('engine_changed_'+label+'.json');path.write_text(json.dumps(patch)+'\n');patches.append(frame(path))
 _,pn,outer,ov=patches[0];_,qn,new_outer,nv=patches[1]
 assert outer==new_outer and ov==nv and all(pn[n]==qn[n] for f in outer for n in f)
 old_physical={tuple(sorted(f['nodes'])):f['facet'] for f in old['surface']}
 new_physical={tuple(sorted(f['nodes'])):f['facet'] for f in new['surface']}
 old_patch_faces={tuple(sorted(f['nodes'])) for f in patches[0][0]['surface']}
 new_patch_faces={tuple(sorted(f['nodes'])) for f in patches[1][0]['surface']}
 assert {k:v for k,v in old_physical.items() if k not in old_patch_faces}=={k:v for k,v in new_physical.items() if k not in new_patch_faces}
 outside=[c for k,c in oc.items() if k in nc];tests=0
 def box(t):return tuple((min(float(p[k]) for p in t),max(float(p[k]) for p in t)) for k in range(3))
 other=[([pn[n] for n in c['nodes']],None) for c in outside]
 other=[(t,box(t)) for t,_ in other]
 for c in added:
  t=[qn[n] for n in c['nodes']];b=box(t)
  for u,a in other:
   if any(b[k][1]<=a[k][0] or a[k][1]<=b[k][0] for k in range(3)):continue
   tests+=1;assert not overlap(t,u),'replacement overlaps an unchanged cell'
 walls=footprints(new,'cylinder');assert len(walls)==baseline['wall_faces'] and all(not r['outside'] for r in walls)
 maximum=0.
 for r in walls:
  a,b,c=[qn[n] for n in r['face']];normal=cross(sub(b,a),sub(c,a));h=abs(float(dot(sub(qn[r['apex']],a),normal)))/float(dot(normal,normal))**.5
  maximum=max(maximum,abs(h-.05)/.05)
 assert maximum<=1e-10
 metrics=json.loads((folder/'grid_refine_summary.json').read_text());assert metrics['remaining_excess']<metrics['initial_excess'] and metrics['oversized_edges']>0
 report=dict(status='PASS_ENGINE_PROGRESS_GEOMETRY_ONLY',scope='Reused identical independently audited base; exact positive local volumes/incidence/no positive-volume overlap, fixed interface/volume, unchanged exterior coordinates/facets/donors, local-to-exterior overlap checks, prescribed wall altitude and normal feet. Native metric progress remains partial; no independent field/edge quadrature or CFD/transfer acceptance.',removed_cells=len(removed),added_cells=len(added),unchanged_cells=len(outside),exterior_pair_candidates=tests,maximum_height_relative_error=maximum,native_metrics=metrics,native_receipt_sha256=sha(rp),base_audit_sha256=sha(HERE/'curved_grid_independent.json'),input_sha256={'old':sha(oldpath),'new':sha(newpath)},helper_sha256=sha(Path(__file__)),predicates_sha256=sha(predicates),cavity_helper_sha256=sha(HERE/'audit_curved_cavity.py'))
 (HERE/'curved_grid_repair_independent.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
