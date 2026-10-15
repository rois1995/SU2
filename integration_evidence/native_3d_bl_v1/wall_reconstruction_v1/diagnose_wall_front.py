#!/usr/bin/env python3
"""Geometry-only active-wall-front experiment on the saved real cylinder residual."""
import collections,csv,hashlib,itertools,json,os
os.environ['OPENBLAS_NUM_THREADS']='1'
from pathlib import Path
import numpy as np
from diagnose_wall_planes import faces,volume
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def incidence(cells):
 out=collections.defaultdict(list)
 for i,c in enumerate(cells):
  for f in faces(c):out[tuple(sorted(f))].append((i,f))
 return out

def compliant(f,cell,nodes):
 a,b,c=[nodes[n] for n in f];n=np.cross(b-a,c-a);n/=np.linalg.norm(n)
 apex=next(v for v in cell if v not in f);p=nodes[apex];height=float(n@(a-p));inv=np.linalg.pinv(np.array([b-a,c-a]).T)
 uv=inv@(p+height*n-a)
 return abs(height-.05)<=1e-11 and min(1-sum(uv),*uv)>=-1e-11

def attempt(data,edge,protect_unchanged):
 allnodes={r[0]:np.array(r[1:],dtype=float) for r in data['nodes']}
 marker={f['id']:data['marker_names'][f['marker']] for f in data['facets']}
 wall=[f for f in data['surface'] if set(edge)<=set(f['nodes']) and marker[f['facet']]=='cylinder']
 apices={next(v for v in c['nodes'] if v not in f['nodes']) for f in wall for c in data['cells'] if set(f['nodes'])<=set(c['nodes'])}
 original=[c['nodes'] for c in data['cells'] if set(edge)<=set(c['nodes']) or apices&set(c['nodes'])]
 cells=[];nodes={n:allnodes[n] for c in original for n in c};initial=set(nodes);point=max(allnodes)+1
 nodes[point]=(nodes[edge[0]]+nodes[edge[1]])/2;nodes[point][:2]/=np.linalg.norm(nodes[point][:2])
 for c in original:
  if set(edge)<=set(c):cells.extend([[point if n==v else n for n in c] for v in edge])
  else:cells.append(c)
 if min(volume(c,nodes) for c in cells)<=0:return dict(status='INHERITED_SPLIT_INVERTED')
 inc=incidence(cells);skin={key for key,rows in inc.items() if len(rows)==1};physical={}
 for f in data['surface']:
  fs=[[point if n==v else n for n in f['nodes']] for v in edge] if set(edge)<=set(f['nodes']) else [f['nodes']]
  for child in fs:
   key=tuple(sorted(child))
   if key in skin:physical[key]=(child,f['facet'])
 walls={key:f for key,(f,facet) in physical.items() if marker[facet]=='cylinder'}
 protected={key for key,f in walls.items() if compliant(f,cells[inc[key][0][0]],nodes)} if protect_unchanged else set()
 active=[key for key in sorted(walls) if key not in protected];grew=0;built=0;reason='';needed=None
 for at,key in enumerate(active):
  f=walls[key];a,b,c=[nodes[n] for n in f];normal=np.cross(b-a,c-a);normal/=np.linalg.norm(normal)
  apex=point+1+at;nodes[apex]=(a+b+c)/3-.05*normal
  inc=incidence(cells);owners=inc[key]
  assert len(owners)==1
  region={owners[0][0]}
  while True:
   star=incidence([cells[i] for i in sorted(region)]);neighbor=None
   for fk,rows in sorted(star.items()):
    if len(rows)!=1:continue
    face=rows[0][1]
    if volume([apex,*face],nodes)>1e-18:continue
    outside=[i for i,_ in inc[fk] if i not in region]
    if not outside:reason='PHYSICAL_BLOCKAGE' if fk in physical else 'MISSING_INTERFACE';needed=list(face);break
    other=outside[0]
    if any(tuple(sorted(g)) in protected for g in faces(cells[other])):
     reason='COMPLETED_WALL_BLOCKAGE';needed=list(face);break
    neighbor=other;break
   if reason or neighbor is None:break
   region.add(neighbor);grew+=1
   if len(region)>64:reason='CAVITY_BOUND';break
  if reason:break
  if any(tuple(sorted(g)) in protected for i in region for g in faces(cells[i])):reason='COMPLETED_WALL_SEED';break
  star=incidence([cells[i] for i in sorted(region)])
  replacement=[[apex,*rows[0][1]] for rows in star.values() if len(rows)==1]
  assert min(volume(c,nodes) for c in replacement)>0
  cells=[c for i,c in enumerate(cells) if i not in region]+replacement;built+=1;protected.add(key)
  if len(cells)>128:reason='REPLACEMENT_BOUND';break
 inc=incidence(cells);invalid=[list(key) for key,f in walls.items() if not compliant(f,cells[inc[key][0][0]],nodes)]
 present={n for c in cells for n in c}
 return dict(status=reason or ('GEOMETRY_CANDIDATE' if not invalid else 'HEIGHT_FOOT_FAILURE'),old_cells=len(original),new_cells=len(cells),wall_faces=len(walls),active_faces=len(active),constructed_faces=built,visibility_growth=grew,obstruction=needed,invalid_wall_faces=invalid,retired_points=sorted(initial-present),minimum_volume=min(volume(c,nodes) for c in cells))

def main():
 parent=HERE/'curved_feature_residual';rp=HERE/'curved_feature_residual.json';r=json.loads(rp.read_text());assert r['status']=='PASS'
 for name in ['audited_grid.json','grid_edges.csv']:assert sha(parent/name)==r['artifacts'][name]
 data=json.loads((parent/'audited_grid.json').read_text());markers={f['id']:data['marker_names'][f['marker']] for f in data['facets']};physical=collections.defaultdict(set)
 for f in data['surface']:
  for edge in itertools.combinations(f['nodes'],2):physical[tuple(sorted(edge))].add(markers[f['facet']])
 candidates=[(tuple(sorted((int(row['a']),int(row['b'])))),float(row['length'])) for row in csv.DictReader((parent/'grid_edges.csv').open()) if float(row['length'])>1.8 and physical[tuple(sorted((int(row['a']),int(row['b']))))]=={'cylinder'}]
 rows=[]
 for edge,length in candidates:
  row=dict(edge=list(edge),native_length=length,all_rows=attempt(data,edge,False),active_rows=attempt(data,edge,True));rows.append(row)
 report=dict(status='GEOMETRY_ONLY_ACTIVE_WALL_FRONT_EXPERIMENT',rows=rows,receipt_sha256=sha(rp),helper_sha256=sha(Path(__file__)),geometry_helper_sha256=sha(HERE/'diagnose_wall_planes.py'),scope='All16 actual oversized pure-cylinder edges. Double-precision midpoint projection, first-height/normal-foot and positive visibility-cone candidates only, on complete original wall-apex stars. No native exact topology/clearance/overlap, actual frozen-field q/J/full lengths, MPI, CFD or performance claim. Compares rebuilding every row to preserving already admissible wall rows; failures diagnose these constructions, not global target infeasibility.')
 out=HERE/'curved_feature_residual_wall_front_probe';out.mkdir(exist_ok=True);(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 for row in sorted(rows,key=lambda x:-x['native_length']):print(row['edge'],row['all_rows']['status'],row['active_rows']['status'],row['active_rows'].get('active_faces'))
if __name__=='__main__':main()
