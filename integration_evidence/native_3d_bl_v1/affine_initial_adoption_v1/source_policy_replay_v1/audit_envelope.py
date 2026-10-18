#!/usr/bin/env python3
"""Exact full saved-annulus envelope checks and a focused replay of the two M6 folds."""
import collections,hashlib,importlib.util,itertools,json,resource,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('source_policy',HERE/'run.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
from audit_cavities import cross,dot,sub,determinant
START=time.monotonic()

def exact_rows(p):return [m.exact(q) for q in p]
def axes(a,b):
 for t in (a,b):
  for i,j,k in itertools.combinations(range(4),3):yield cross(sub(t[j],t[i]),sub(t[k],t[i]))
 for i,j in itertools.combinations(range(4),2):
  for k,l in itertools.combinations(range(4),2):yield cross(sub(a[j],a[i]),sub(b[l],b[k]))
def conforming(a,ai,b,bi):
 shared=set(ai)&set(bi);assert len(shared)<4
 alive_a=set(ai);alive_b=set(bi)
 for axis in axes(a,b):
  if not any(axis):continue
  pa=[dot(axis,x) for x in a];pb=[dot(axis,x) for x in b]
  if max(pa)<min(pb) or max(pb)<min(pa):return not shared
  bounds=[]
  if max(pa)==min(pb):bounds.append(max(pa))
  if max(pb)==min(pa):bounds.append(max(pb))
  for bound in bounds:
   alive_a&={i for i,q in zip(ai,pa) if q==bound};alive_b&={i for i,q in zip(bi,pb) if q==bound}
   if shared and (alive_a==shared or alive_b==shared):return True
 return False # Sufficient exact supporting-face certificate, not a complete intersection solver.
def separated(a,b,band):
 # AABB lower bound, then exact projected interval gaps. Any passing axis proves Euclidean separation.
 gap=[max(F(0),min(x[k] for x in a)-max(x[k] for x in b),min(x[k] for x in b)-max(x[k] for x in a)) for k in range(3)]
 if dot(gap,gap)>=band*band:return True
 ea=[sub(a[j],a[i]) for i,j in ((0,1),(1,2),(2,0))];eb=[sub(b[j],b[i]) for i,j in ((0,1),(1,2),(2,0))]
 trial=[cross(ea[0],ea[1]),cross(eb[0],eb[1])]+[cross(x,y) for x in ea for y in eb]
 for axis in trial:
  norm=dot(axis,axis)
  if not norm:continue
  pa=[dot(axis,x) for x in a];pb=[dot(axis,x) for x in b]
  distance=max(F(0),min(pa)-max(pb),min(pb)-max(pa))
  if distance*distance>=band*band*norm:return True
 return False

def selfcheck():
 a=[(F(0),F(0),F(0)),(F(1),F(0),F(0)),(F(0),F(1),F(0)),(F(0),F(0),F(1))]
 b=[a[0],a[2],a[1],(F(0),F(0),F(-1))]
 assert conforming(a,[0,1,2,3],b,[0,2,1,4])
 assert not conforming(a,[0,1,2,3],a,[4,5,6,7])
 b=[tuple(x+2 for x in q) for q in a];assert conforming(a,[0,1,2,3],b,[4,5,6,7])
 b=[(-x,-y,-z) for x,y,z in a];assert conforming(a,[0,1,2,3],b,[0,4,5,6])
 assert not conforming(a,[0,1,2,3],b,[4,5,6,7]),'Unnamed touching point must reject.'
 tri=a[:3];above=[(x,y,z+F(1,8)) for x,y,z in tri]
 assert separated(tri,above,F(1,8)) and not separated(tri,above,F(1,4))

def make_cells(g,directions,band,width=1):
 nodes={int(n):g['points'][n] for n in directions};span=max(nodes)+1
 nodes.update({n+span:g['points'][n]+width*band*v for n,v in directions.items() if v is not None})
 records=[];signed=[]
 for face,ids in enumerate(g['faces']):
  ids=sorted(map(int,ids))
  if not all(n+span in nodes for n in ids):continue
  order=ids+[n+span for n in ids];a,b,c=[g['points'][n] for n in ids]
  outward=np.dot(np.cross(b-a,c-a),g['normals'][face])>0
  for slot,ix in enumerate(((0,1,2,5),(0,1,4,5),(0,3,4,5))):
   cell=[order[j] for j in ix]
   if (slot!=1)!=outward:cell[0],cell[1]=cell[1],cell[0]
   value=determinant([m.exact(nodes[n]) for n in cell]);signed.append(value)
   records.append(dict(face=face,slot=slot,nodes=cell,determinant=str(value)))
 return nodes,records,signed,span

def export(path,nodes,cells):
 ids=sorted({i for c in cells for i in c['nodes']});local={i:j for j,i in enumerate(ids)}
 lines=['NDIME= 3','NELEM= '+str(len(cells))]+['10 '+' '.join(str(local[n]) for n in c['nodes']) for c in cells]
 lines+=['NPOIN= '+str(len(ids))]+[' '.join(format(float(x),'.17g') for x in nodes[n])+' '+str(local[n]) for n in ids]
 lines+=['NMARK= 0'];path.write_text('\n'.join(lines)+'\n')

def annulus():
 source=HERE.parent/'annulus_v1_n1/affine_annulus_bl_budget_20_m1_initial.json';d=json.loads(source.read_text());p=np.array([n[1:] for n in d['nodes']]);c=np.array([r['nodes'] for r in d['cells']]);faces=np.array([[v[0] for v in f['vertices']] for f in d['facets']]);markers=[d['marker_names'][f['marker']] for f in d['facets']]
 g=m.prepare(p,c,faces,markers,np.broadcast_to(np.eye(3),(len(p),3,3)))
 directions={n:m.outward_direction(g,n)[0] for n in g['stars']};assert all(v is not None for v in directions.values())
 band=1e-13;nodes,records,volumes,span=make_cells(g,directions,band,width=2);assert min(volumes)>0
 all_cells=[dict(nodes=list(map(int,r))) for r in c]+records
 exact={n:m.exact(q) for n,q in nodes.items()};pair_tests=0;uncertified=[]
 for i,a in enumerate(all_cells):
  aa=[exact[n] for n in a['nodes']]
  for j,b in enumerate(all_cells[i+1:],i+1):
   if i<len(c) and j<len(c):continue # Original embedded source is an existing accepted input assumption.
   bb=[exact[n] for n in b['nodes']]
   if any(max(q[k] for q in aa)<min(q[k] for q in bb) or max(q[k] for q in bb)<min(q[k] for q in aa) for k in range(3)):continue
   pair_tests+=1
   if not conforming(aa,a['nodes'],bb,b['nodes']):uncertified.append([i,j])
  m.budget()
 # The augmented mesh must have exactly its mapped outer skin; shared source faces become interior.
 uses=collections.defaultdict(list)
 for row in all_cells:
  a,b,c0,d0=row['nodes']
  for face in ((b,c0,d0),(a,d0,c0),(a,b,d0),(a,c0,b)):
   parity=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3));uses[tuple(sorted(face))].append(parity)
 assert all(len(s)==1 or len(s)==2 and sum(s)==0 for s in uses.values())
 expected={tuple(sorted(int(n)+span for n in face)) for face in faces};assert {f for f,s in uses.items() if len(s)==1}==expected
 old_tri=[[exact[int(n)] for n in face] for face in faces];outer=[[exact[int(n)+span] for n in face] for face in faces]
 missing=[]
 for i,a in enumerate(old_tri):
  for j,b in enumerate(outer):
   if not separated(a,b,F(band)):missing.append([i,j])
  m.budget()
 export(HERE/'annulus_source_envelope_probe.su2',nodes,all_cells)
 artifact=dict(nodes=[[n]+list(map(float,q)) for n,q in sorted(nodes.items())],collar_cells=records,original_cells=[list(map(int,row)) for row in g['cells']],directions={str(n):v.tolist() for n,v in directions.items()})
 (HERE/'annulus_source_envelope.json').write_text(json.dumps(artifact,indent=2)+'\n')
 report=dict(status='EXACT_SAVED_ANNULUS_ENVELOPE_CERTIFICATE' if not uncertified and not missing else 'UNCERTIFIED_ENVELOPE',
  requested_band=band,column_width_factor=2,source_cells=len(c),collar_cells=len(records),all_positive_exact=True,
  exact_conforming_pair_tests=pair_tests,uncertified_pairs=uncertified,outer_old_triangle_pairs=len(faces)**2,uncertified_separation_pairs=missing,
  oriented_augmented_skin_matches_outer_skin=True,source_node_displacements_max=float(max(np.linalg.norm(nodes[n+span]-p[n]) for n in directions)),
  interpretation='Exact positive/conforming augmented mesh and full old-to-outer-skin separation lower bound on the saved closed annulus; conditional on the original source being embedded. No full aerodynamic geometry/runtime admission.',
  scope='The source collar is integration/query geometry, not an adapted CFD mesh; there are no quality, full metric, flow, transfer, restart or MPI claims.')
 (HERE/'annulus_envelope_audit.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],pair_tests,'uncertified',len(uncertified),len(missing),flush=True)
 return report

def folded_m6():
 base=m.ROOT/'integration_evidence/native_3d_bl_v1/oneram6_frozen_field_v1';mesh=m.read_su2(base/'Coarse.su2');data=np.load(base/'frozen_trial.npz',allow_pickle=False)
 raw=data['sensor_owner'];tensors=np.tril(raw)+np.swapaxes(np.tril(raw,-1),-2,-1);faces=np.concatenate(list(mesh.M.values()));markers=[name for name,a in mesh.M.items() for _ in a]
 prior=json.loads((HERE/'saved_m6_owner_sensor.json').read_text());failed=prior['bands'][-1]['degenerate_or_folded_faces'];assert len(failed)==2
 g=m.prepare(mesh.P,mesh.E,faces,markers,tensors);required=sorted(set(int(n) for i in failed for n in faces[i]));directions={n:m.outward_direction(g,n)[0] for n in required}
 assert all(v is not None for v in directions.values());old_faces=g['faces'];old_normals=g['normals'];g['faces']=faces[failed];g['normals']=old_normals[failed]
 # The replay includes all globally incident normal constraints at these nodes, not only the two failed faces.
 nodes,records,values,span=make_cells(g,directions,1e-4)
 assert any(v<0 for v in values)
 export(HERE/'m6_wide_collar_rejected.su2',nodes,records)
 details=[]
 for i,face in enumerate(failed):
  ids=list(map(int,faces[face]));incident=sorted({j for n in ids for j in g['stars'][n]})
  details.append(dict(face=face,marker=markers[face],nodes=ids,source_parent=int(g['parents'][face]),source_parent_vertices=mesh.E[g['parents'][face]].tolist(),
   unit_outward_normal=list(map(float,old_normals[face])),normal_constraints=[dict(face=j,normal=list(map(float,old_normals[j])),marker=markers[j]) for j in incident],
   column_norms=[float(np.linalg.norm(directions[n])) for n in ids],
   determinants=[records[k]['determinant'] for k in range(3*i,3*i+3)],negative_slots=[k for k in range(3) if values[3*i+k]<0]))
 artifact=dict(source_nodes=[[n]+mesh.P[n].tolist() for n in required],generated_nodes=[[n]+list(map(float,p)) for n,p in sorted(nodes.items()) if n>=span],
  original_nodal_sensor={str(n):tensors[n].tolist() for n in required},directions={str(n):v.tolist() for n,v in directions.items()},cells=records,faces=details,
  interpretation='Exact binary64 prism folds at the saved wider band. Outward normal-cone constraints at individual vertices do not ensure a positive prism map. No fallback or enlarged acceptance tolerance is introduced.')
 (HERE/'m6_folded_collar_exact.json').write_text(json.dumps(artifact,indent=2)+'\n');print('EXACT_M6_COLLAR_FOLDS',[(r['face'],r['negative_slots'],r['column_norms']) for r in details],flush=True)
 return dict(status='EXACT_M6_WIDE_COLLAR_REJECTION',faces=failed,negative_cells=sum(v<0 for v in values),cells=len(records),same_captured_failed_faces=True)

def main():
 selfcheck();a=annulus();b=folded_m6()
 paths=[Path(__file__),HERE/'run.py',HERE/'numerical_results_manifest.json',HERE/'annulus_source_envelope.json',HERE/'annulus_source_envelope_probe.su2',HERE/'m6_folded_collar_exact.json',HERE/'m6_wide_collar_rejected.su2']
 result=dict(status='COMPLETE_SOURCE_ENVELOPE_DIAGNOSIS',annulus=a,m6=b,elapsed_seconds=time.monotonic()-START,
  process_maxrss_KiB=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,source_and_artifact_sha256={str(p.resolve().relative_to(m.ROOT)):m.sha(p) for p in paths},
  decision='A bounded conforming collar is viable for the saved numerical mapping band; neither nearest-face projection nor unrestricted shared normal columns solves the full physical approximation envelope. Preserve qualified analytic coverage and require exact global admission/actual-envelope coverage for new joint collars.')
 (HERE/'envelope_result.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],result['elapsed_seconds'],result['process_maxrss_KiB'],flush=True)
if __name__=='__main__':main()
