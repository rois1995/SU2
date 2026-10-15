#!/usr/bin/env python3
"""Closed radial-integral probe of positive projective angular quadrature, not SU2."""
import json,math
from pathlib import Path
C=(.35,.35,.02);V=((0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(0.,0.,1.));K=500.
HIGH=((1/3,1/3,1/3),(.059715871789770,.470142064105115,.470142064105115),(.470142064105115,.059715871789770,.470142064105115),(.470142064105115,.470142064105115,.059715871789770),(.797426985353087,.101286507323456,.101286507323456),(.101286507323456,.797426985353087,.101286507323456),(.101286507323456,.101286507323456,.797426985353087))
HW=(.225,.132394152788506,.132394152788506,.132394152788506,.125939180544827,.125939180544827,.125939180544827)
LOW=((2/3,1/6,1/6),(1/6,2/3,1/6),(1/6,1/6,2/3))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def norm(a):return math.sqrt(dot(a,a))
def mean(points,w):return tuple(sum(a[k]*b for a,b in zip(points,w)) for k in range(3))
def integrate(panel,mode,high):
 rays=[sub(p,C) for p in panel];lengths=list(map(norm,rays));volume=abs(dot(rays[0],cross(rays[1],rays[2])))/6
 mass=value=0.
 for alpha,w in zip(HIGH if high else LOW,HW if high else (1/3,)*3):
  beta=list(alpha);jac=1.
  if mode=='projective':
   inverse=[alpha[i]/lengths[i] for i in range(3)];total=sum(inverse);beta=[a/total for a in inverse];jac=1/(math.prod(lengths)*total**3)
  end=mean(panel,beta);r=norm(sub(end,C));mass+=w*jac;value+=w*jac*(8*volume+3*volume*K/r**3)
 return value/mass,volume

def run(mode):
 count=0;retained=0;total=mass=0.;max_depth=0
 def refine(panel,depth):
  nonlocal count,retained,total,mass,max_depth
  count+=1;assert count<=16384 and depth<32;max_depth=max(max_depth,depth)
  low,_=integrate(panel,mode,False);high,volume=integrate(panel,mode,True)
  if abs(high-low)<=1e-4*high:
   retained+=1;total+=high;mass+=volume;return
  rays=[sub(p,C) for p in panel];lengths=list(map(norm,rays))
  if mode=='projective':rays=[tuple(x/r for x in p) for p,r in zip(rays,lengths)]
  a,b=max(((i,j) for i in range(3) for j in range(i+1,3)),key=lambda ij:norm(sub(rays[ij[0]],rays[ij[1]])))
  weights=[0.]*3
  if mode=='projective':weights[a]=1/lengths[a];weights[b]=1/lengths[b];weights=[x/sum(weights) for x in weights]
  else:weights[a]=weights[b]=.5
  middle=mean(panel,weights)
  for i in (a,b):child=list(panel);child[i]=middle;refine(child,depth+1)
 for skip in range(4):
  face=[V[i] for i in range(4) if i!=skip]
  normal=cross(sub(face[1],face[0]),sub(face[2],face[0]));distance=dot(sub(C,face[0]),normal)/dot(normal,normal)
  foot=tuple(C[k]-distance*normal[k] for k in range(3))
  for i in range(3):refine((foot,face[i],face[(i+1)%3]),0)
 expected=8/6+4*math.pi*K
 assert abs(mass-1/6)<1e-13 and abs(total/expected-1)<8e-4
 return dict(panels_visited=count,panels_retained=retained,maximum_depth=max_depth,volume=mass,integral=total,expected=expected,relative_error=total/expected-1)
if __name__=='__main__':
 report=dict(scope='Analytic closed radial integral and positive volume-normalized angular rules only; no tensor, wall query, sensor, MPI or SU2 qualification.',physical=run('physical'),projective=run('projective'))
 Path(__file__).with_name('report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
