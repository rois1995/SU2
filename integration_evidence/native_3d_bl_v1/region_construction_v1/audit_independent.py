#!/usr/bin/env python3
"""Independent exact embedding/height and exported-tensor arithmetic for a complete region candidate."""
import collections,csv,itertools,json,math,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
from construct import HERE,ROOT,SOURCE,sha,authorities
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(HERE.parent/'alignment_audit_v1'))
from audit_cavities import determinant,sub,dot,cross,overlap
from audit_coupled import surface,connected,spd,quad
from baseline import footprints

def main():
 start=time.monotonic();case=HERE/(sys.argv[1] if len(sys.argv)>1 else 'metric_region');folder=case/'native_field'
 receipt=json.loads((case/'native_field.json').read_text());assert receipt['status']=='PASS_NATIVE_FIELD_ONLY'
 for name,h in receipt['artifacts'].items():assert sha(folder/name)==h
 data=json.loads((folder/'audited_grid.json').read_text());original=json.loads((SOURCE/'audited_grid.json').read_text())
 assert all(data[k]==original[k] for k in ('original','facets','facet_groups','marker_names'))
 nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']};assert len(nodes)==len(data['nodes']) and len(set(nodes.values()))==len(nodes)
 uses=collections.defaultdict(list);links=collections.defaultdict(list);tets=[];boxes=[];volume=F(0);difference=0.;qmin=jmin=1.
 samples={int(r['cell']):r for r in csv.DictReader((folder/'grid_samples.csv').open())}
 for index,c in enumerate(data['cells']):
  t=[nodes[n] for n in c['nodes']];dv=determinant(t);assert dv>0;volume+=dv/6;tets.append(t)
  boxes.append(tuple((min(float(p[k]) for p in t),max(float(p[k]) for p in t)) for k in range(3)))
  a,b,d,e=c['nodes']
  for face in [(b,d,e),(a,e,d),(a,b,e),(a,d,b)]:uses[tuple(sorted(face))].append((index,(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3))))
  for i,n in enumerate(c['nodes']):links[n].append(tuple(v for j,v in enumerate(c['nodes']) if j!=i))
  row=samples[c['id']];m=tuple(F(float(row[k])) for k in ('xx','xy','xz','yy','yz','zz'));dm=spd(m);lengths={}
  for i,j in itertools.combinations(range(4),2):lengths[i,j]=math.sqrt(float(quad(m,sub(t[j],t[i]))))
  weighted=float(dv)*math.sqrt(float(dm));q=12*(weighted/2)**(2/3)/sum(v*v for v in lengths.values());jac=min(1.,min(math.sqrt(2)*weighted/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if j!=i) for i in range(4)))
  assert q>=.2 and jac>=.05
  delta=max(abs(q-float(row['q'])),abs(jac-float(row['j'])));assert delta<2e-10;difference=max(difference,delta);qmin=min(qmin,q);jmin=min(jmin,jac)
 assert len(samples)==len(tets)
 assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values())
 boundary={f for f,v in uses.items() if len(v)==1};assert boundary=={tuple(sorted(f['nodes'])) for f in data['surface']};surface(list(boundary),True)
 boundary_nodes={n for f in boundary for n in f};adjacency={i:set() for i in range(len(tets))}
 for values in uses.values():
  if len(values)==2:a,b=values[0][0],values[1][0];adjacency[a].add(b);adjacency[b].add(a)
 assert connected(adjacency)
 for n,triangles in links.items():surface(triangles,n not in boundary_nodes)
 # Reuse exact separating-axis predicates; bounding boxes only discard provably disjoint interiors.
 candidates=0
 for i,j in itertools.combinations(range(len(tets)),2):
  if any(boxes[i][k][1]<=boxes[j][k][0] or boxes[j][k][1]<=boxes[i][k][0] for k in range(3)):continue
  candidates+=1;assert not overlap(tets[i],tets[j]),('positive-volume overlap',i,j)
 feet=footprints(data,'cylinder');assert feet and all(not r['outside'] for r in feet);height_error=0.;tilt=0.
 for r in feet:
  a,b,c=[nodes[n] for n in r['face']];n=cross(sub(b,a),sub(c,a));relative=sub(nodes[r['apex']],a);normal2=dot(n,n);signed=dot(relative,n);height=abs(float(signed))/math.sqrt(float(normal2));height_error=max(height_error,abs(height-.05)/.05)
  foot=tuple(nodes[r['apex']][i]-signed*n[i]/normal2 for i in range(3));center=tuple((a[i]+b[i]+c[i])/3 for i in range(3));tilt=max(tilt,math.sqrt(float(dot(sub(foot,center),sub(foot,center))))/.05)
 assert height_error<=1e-10
 geometry,_=authorities(SOURCE/'curved_cylinder_initial_euler.su2.native_ref');geometry_error=0.;sag=0.
 for f in data['surface']:
  kind,origin,extent,tolerance,axis=geometry[f['facet']];axis=axis/np.linalg.norm(axis);points=np.array([[float(x) for x in nodes[n]] for n in f['nodes']]);v=points-origin;z=v@axis;radial=v-z[:,None]*axis
  if kind==4:
   geometry_error=max(geometry_error,float(np.max(np.abs(z))));assert np.max(np.linalg.norm(radial,axis=1))<=extent[0]+tolerance
  elif kind==3:
   geometry_error=max(geometry_error,float(np.max(np.abs(np.linalg.norm(radial,axis=1)-extent[0]))));assert z.min()>=-tolerance and z.max()<=extent[1]+tolerance
   for k in range(3):
    a,b=radial[k],radial[(k+1)%3];d=b-a;t=np.clip(-a@d/(d@d),0,1) if d@d else 0;sag=max(sag,float(extent[0]-np.linalg.norm(a+t*d)))
  else:raise AssertionError('Unexpected geometry in declared unchanged benchmark')
 assert geometry_error<=1e-12 and sag<=min(.04,.025)
 lengths=list(csv.DictReader((folder/'grid_edges.csv').open()));assert len(lengths)==receipt['metrics']['edges'];assert all(float(r['length'])<=1.8 for r in lengths)
 report=dict(status='COMPLETE_CANDIDATE_GEOMETRY_HEIGHT_AND_NATIVE_FIELD_PASS',cells=len(tets),nodes=len(nodes),wall_faces=len(feet),pair_candidates=candidates,vertex_links_checked=len(links),volume=str(volume),minimum_q=qmin,minimum_j=jmin,maximum_native_shape_difference=difference,maximum_height_relative_error=height_error,maximum_centroid_tilt=tilt,maximum_authority_node_error=geometry_error,maximum_wall_chord_sag=sag,oversized_edges=0,maximum_native_full_length=max(float(r['length']) for r in lengths),original_donors_and_reference_unchanged=True,native_receipt_sha256=sha(case/'native_field.json'),helper_sha256=sha(Path(__file__)),predicates_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py'),topology_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),feet_sha256=sha(HERE.parent/'alignment_audit_v1/baseline.py'),authority_parser_sha256=sha(HERE/'construct.py'),elapsed_seconds=time.monotonic()-start,scope='Complete prototype candidate only: exact positive volumes, oriented/connected ball embedding and every vertex link, no positive-volume overlap, exact normal feet/altitude, analytic authority nodes/chord sag and independent exported-tensor q/J arithmetic. Full composed lengths are native measurements, not independent integration proof. No production constructor/integrated engine acceptance, MPI, CFD/current-BDF transfer/restart, accuracy or performance qualification.')
 (case/'independent.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
