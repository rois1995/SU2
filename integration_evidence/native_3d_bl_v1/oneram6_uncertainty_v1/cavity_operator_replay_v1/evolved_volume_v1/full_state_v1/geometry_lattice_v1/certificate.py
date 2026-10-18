#!/usr/bin/env python3
"""Exact binary64 lattice obstruction plus separate native fixed-accuracy length audit."""
import hashlib,json,math,sys
from fractions import Fraction as F
from functools import reduce
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;BASE=HERE.parents[2];sys.path.insert(0,str(BASE));from diagnose_contexts import patch

def sub(a,b):return [x-y for x,y in zip(a,b)]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def gcd(v):return reduce(math.gcd,map(abs,v))
def lattice(points):
 # Every binary64 in the coordinate box lies on these grids, even across binades.
 xyz=np.array(points);spacing=[];vectors=[]
 for j in range(3):
  low,high=float(xyz[:,j].min()),float(xyz[:,j].max());m=min(abs(low),abs(high)) if low*high>0 else 0.
  h=float(np.nextafter(m,np.inf)-m) if m else float(np.nextafter(0.,1.));assert math.isfinite(h) and h>0
  row=[(F(float(x))-F(float(xyz[0,j])))/F(h) for x in xyz[:,j]];assert all(x.denominator==1 for x in row)
  spacing.append(h);vectors.append([int(x) for x in row])
 return spacing,[[row[k] for row in vectors] for k in range(len(points))]
def interval(a,b):
 h,v=lattice([a,b]);g=gcd(v[1]);assert g>0
 return {'finest_axis_spacings':h,'integer_chord':[str(x) for x in v[1]],'parameter_denominator_gcd':g,'maximum_interior_binary64_points':g-1}
def selfcheck():
 x=.5;y=float(np.nextafter(x,np.inf));z=float(np.nextafter(y,np.inf))
 assert interval((x,x,x),(y,x,x))['parameter_denominator_gcd']==1
 assert interval((x,x,x),(z,x,x))['parameter_denominator_gcd']==2
 _,v=lattice([(x,x,x),(z,x,x),(x,z,x)]);u,w=v[1:];assert gcd(cross(u,w))==4
 assert (gcd(cross(u,w))-gcd(u)-gcd(w)-gcd(sub(w,u))+2)//2==0

def main():
 selfcheck();cells,nodes,faces=patch(HERE/'working.dat');a,b=nodes[211],nodes[3481];edge=interval(a,b);assert edge['parameter_denominator_gcd']==2
 exact_mid=[(F(x)+F(y))/2 for x,y in zip(a,b)];mid=[float(x) for x in exact_mid];assert all(F(x)==q for x,q in zip(mid,exact_mid))
 left=interval(a,mid);right=interval(mid,b);assert left['parameter_denominator_gcd']==right['parameter_denominator_gcd']==1
 p,q,r,s=[[F(x) for x in nodes[n]] for n in [211,3481,3544,212]];orientation=sum(x*y for x,y in zip(sub(s,p),cross(sub(q,p),sub(r,p))));assert orientation!=0
 triangles=[]
 for face in faces:
  h,v=lattice([nodes[n] for n in face[:3]]);u,w=v[1:];area=gcd(cross(u,w));boundary=gcd(u)+gcd(w)+gcd(sub(w,u));assert (area-boundary+2)%2==0
  triangles.append({'facet':face[-1],'nodes':face[:3],'finest_axis_spacings':h,'primitive_area_multiple':area,'boundary_lattice_points':boundary,'maximum_interior_binary64_points':(area-boundary+2)//2})
 native=[json.loads(x) for x in (HERE/'trials.jsonl').read_text().splitlines()];m=native[1];assert m['kind']=='crease'
 assert len(set(m['canonical_facets']))==2 and m['analytic_authorities']==0 and m['midpoint']==mid
 assert abs(m['length']-m['left']-m['right'])<1e-9
 assert m['length']>m['edge_limit'] and m['left']>6*m['edge_limit'] and m['right']<=m['edge_limit']
 source=BASE.parent/'concurrent_composed_proposals_v1/frozen_m6_np4';dtype=np.dtype([('id','<u8'),('x','<f8',(3,)),('m','<f8',(3,3))]);data=np.fromfile(source/'selected_sensor.bin',dtype=dtype,offset=8)
 assert tuple(data['x'][211])==a and tuple(data['x'][3481])==b
 d=np.array(b,dtype=np.longdouble)-np.array(a,dtype=np.longdouble);ma=data['m'][211].astype(np.longdouble);mb=data['m'][3481].astype(np.longdouble)
 A=d@ma@d;B=d@mb@d
 def integral(A,B):return float(np.longdouble(2)/3*(A+B+np.sqrt(A*B))/(np.sqrt(A)+np.sqrt(B)))
 result={'scope':'Exact finite-coordinate/facet obstruction; metric incompatibility uses the native fixed-accuracy measurement, not a proven interval quadrature bound',
 'physical_edge':[211,3481],'exact_edge_lattice':edge,'unique_interior_point':mid,'left_no_interior_point':left,'right_no_interior_point':right,
 'facet_planes_exactly_noncoplanar':True,'exact_four_point_orientation':{'numerator':str(orientation.numerator),'denominator':str(orientation.denominator),'approximate':float(orientation)},
 'facet_lattice_upper_bounds':triangles,'native_frozen_field_measurement':m,
 'sensor_only_original_edge_diagnostic':{'whole':integral(A,B),'left':integral(A/4,(A+B)/8),'right':integral((A+B)/8,B/4)},
 'interpretation':'Any conforming binary64 mesh preserving these exact distinct facet planes must cover the crease interval. There is no binary64 vertex inside the left subinterval; its full composed length is11.57>1.8. Larger cavities or different connectivity cannot remove that constraint. This concerns the archived frozen target/exact-facet authority, not every geometry representation, profile or native mesh.',
 'not_claimed':['A proven interval bound for metric quadrature','Infeasibility of the budget alone','Infeasibility after a separately declared geometry/reference change','Native failure on feasible declared analytic geometry'],
 'hashes':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path(__file__),HERE/'working.dat',HERE/'trials.jsonl',HERE/'receipt.json']}}
 (HERE/'certificate.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
