#!/usr/bin/env python3
"""Exact original-domain and imported-support coverage of one saved binary64 edge failure."""
import hashlib,json,re,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2

def determinant(t):
 a,b,c,d=t;x,y,z=[tuple(p[j]-a[j] for j in range(3)) for p in (b,c,d)]
 return x[0]*(y[1]*z[2]-y[2]*z[1])-x[1]*(y[0]*z[2]-y[2]*z[0])+x[2]*(y[0]*z[1]-y[1]*z[0])
def weights(t,p):
 den=determinant(t);assert den
 result=[]
 for j in range(4):
  s=list(t);s[j]=p;result.append(determinant(s)/den)
 assert sum(result)==1;return result

def interval(first,last):
 low,high=F(0),F(1)
 for a,b in zip(first,last):
  if a<0 and b<0:return None
  if a<0:low=max(low,a/(a-b))
  elif b<0:high=min(high,a/(a-b))
 return (low,high) if low<=high else None

def gaps(intervals):
 cursor=F(0);result=[]
 for low,high in sorted(intervals):
  if low>cursor:result.append((cursor,low))
  cursor=max(cursor,high)
 if cursor<1:result.append((cursor,F(1)))
 return result

def main():
 # Independent self-check: crossing, reverse, excluded, and a genuine gap.
 assert interval([F(-1),F(2)],[F(1),F(0)])==(F(1,2),F(1))
 assert interval([F(1),F(0)],[F(-1),F(2)])==(F(0),F(1,2))
 assert interval([F(-1)],[F(-2)]) is None
 assert gaps([(F(0),F(1,3)),(F(1,2),F(1))])==[(F(1,3),F(1,2))]
 text=(HERE/'run.log').read_text();line=next(s for s in text.splitlines() if 'Native3D rejected event:' in s and 'edge=(' in s)
 m=re.search(r'edge=\(([^)]+)\)\.\.\(([^)]+)\) admitted=([^;]+?) candidates=([\d,]*)',line);assert m,line
 endpoints=[tuple(float(v) for v in m.group(i).split(',')) for i in (1,2)];assert all(len(p)==3 and all(np.isfinite(p)) for p in endpoints)
 admitted={int(cell):tuple(sorted(int(v) for v in ids.split(',') if v)) for cell,ids in re.findall(r'(\d+)\[([\d,]+)\]',m.group(3))}
 selected={int(v) for v in m.group(4).split(',') if v};assert selected.issubset(admitted)
 selected_keys={admitted[k] for k in selected};admitted_keys=set(admitted.values())
 mesh=read_su2(HERE/'Coarse.su2');assert mesh.dim==3
 geometry=mesh.P[mesh.E];low=geometry.min(axis=1);high=geometry.max(axis=1);del geometry
 lo=np.min(endpoints,axis=0);hi=np.max(endpoints,axis=0)
 candidates=np.flatnonzero(np.all((low<=hi)&(high>=lo),axis=1));assert len(candidates)<=8192,'Bounded exact replay work exceeded'
 exact=[tuple(F(v) for v in p) for p in endpoints];rows=[];coverage=[[],[],[]];contained=[[],[]];nearest=[None,None]
 for i in candidates:
  ids=mesh.E[i];key=tuple(sorted(map(int,ids)));tet=[tuple(F(float(v)) for v in mesh.P[n]) for n in ids]
  first,last=[weights(tet,p) for p in exact]
  for j,w in enumerate((first,last)):
   worst=min(w)
   if nearest[j] is None or worst>nearest[j][0]:nearest[j]=(worst,int(i),key)
   if worst>=0:contained[j].append({'file_cell':int(i),'nodes':key,'imported':key in admitted_keys,'ADT_candidate':key in selected_keys,'weights':[str(x) for x in w]})
  cut=interval(first,last)
  if cut is None:continue
  coverage[0].append(cut)
  if key in admitted_keys:coverage[1].append(cut)
  if key in selected_keys:coverage[2].append(cut)
  rows.append({'file_cell':int(i),'nodes':key,'begin':str(cut[0]),'end':str(cut[1]),'imported':key in admitted_keys,'ADT_candidate':key in selected_keys})
 missing=[gaps(c) for c in coverage]
 classification='OUTSIDE_ORIGINAL_PHYSICAL_DOMAIN' if missing[0] else 'MISSING_IMPORTED_SEGMENT_SUPPORT' if missing[1] else 'IMPORTED_SUPPORT_MISSED_BY_ADT' if missing[2] else 'TRACE_KERNEL_REQUIRES_AUDIT'
 def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
 result={'edge':endpoints,'admitted_keys':len(admitted),'ADT_candidates':len(selected),'global_AABB_candidates':len(candidates),
  'classification':classification,'exact_original_coverage_gaps':[[str(a),str(b)] for a,b in missing[0]],
  'exact_imported_coverage_gaps':[[str(a),str(b)] for a,b in missing[1]],'exact_ADT_coverage_gaps':[[str(a),str(b)] for a,b in missing[2]],
  'endpoint_containing_cells':contained,'endpoint_best_minimum_weight':[None if v is None else {'minimum_weight':str(v[0]),'file_cell':v[1],'nodes':v[2]} for v in nearest],
  'intersections':rows,'scope':'Exact Fraction clipping of the actual binary64 segment against every original cell whose closed AABB meets the segment box. No snapping, extrapolation, tolerance or new CFD run.',
  'sha256':{str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),HERE/'Coarse.su2',HERE/'run.log',HERE/'provenance.json',ROOT/'TestCases/adaptation/capability/capcheck.py')}}
 (HERE/'exact_segment_audit.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:v for k,v in result.items() if k not in ('intersections','sha256','endpoint_containing_cells')},indent=2))
if __name__=='__main__':main()
