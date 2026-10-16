#!/usr/bin/env python3
"""Resumable independent normal-density reference with radial and apex changes."""
from pathlib import Path
import argparse,hashlib,json,runpy,time
import numpy as np
from numpy.polynomial.legendre import leggauss
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--apex',type=int,choices=range(4),required=True)
parser.add_argument('--angular',type=int,required=True);parser.add_argument('--radial',type=int,required=True)
parser.add_argument('--bands',type=float,default=1.5);args=parser.parse_args()
assert 4<=args.angular<=256 and 4<=args.radial<=64 and 1<args.bands<=2
# Reuse the qualified exact-triangle oracle; its legacy acceptance/reference runs first.
old=runpy.run_path(str(HERE/'check_normal_integral.py'))
cell=old['cell'];distance=old['distance'];density=old['density'];volume=old['volume']
identity={'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          'oracle_sha256':hashlib.sha256((HERE/'check_normal_integral.py').read_bytes()).hexdigest(),
          'fixture_sha256':hashlib.sha256((HERE/'wing_cell13.dat').read_bytes()).hexdigest(),
          'numpy':np.__version__,'parameters':vars(args)}
key='apex%d_a%d_r%d_b%s'%(args.apex,args.angular,args.radial,str(args.bands).replace('.','p'))
path=HERE/('radial_chart_'+key+'.json')
if path.is_file():
 saved=json.loads(path.read_text())
 assert saved['identity']==identity, 'Changed reference dependencies: retain old result before recomputing.'
 if saved['complete']:
  print('REUSED_COMPLETE',path.name,saved['integral']);raise SystemExit(0)
else:saved={'identity':identity,'complete':False,'next_radial':0,'integral':0.,'mass':0.,'elapsed_seconds':0.}
z,w=leggauss(args.angular);z=.5*(1+z);w=.5*w
u,v=np.meshgrid(z,z,indexing='ij');angular=(w[:,None]*w[None,:]*u).ravel()
base_ids=[i for i in range(4) if i!=args.apex]
base=(1-u.ravel())[:,None]*cell[base_ids[0]]+(u*(1-v)).ravel()[:,None]*cell[base_ids[1]]+(u*v).ravel()[:,None]*cell[base_ids[2]]
diameter=np.linalg.norm(cell[:,None,:]-cell[None,:,:],axis=2).max()
scale=old['h0']/diameter
cuts=[0.,1.,.5*scale,1-.5*scale];level=scale
while level<1:
 cuts.extend([level,1-level]);level*=args.bands
cuts=sorted(set(cuts));rz,rw=leggauss(args.radial)
t=[];wt=[]
for left,right in zip(cuts[:-1],cuts[1:]):
 half=.5*(right-left);t.extend(.5*(left+right)+half*rz);wt.extend(half*rw)
assert len(t)==len(wt) and 0<=saved['next_radial']<=len(t)
start=time.monotonic();offset=saved['elapsed_seconds']
for k in range(saved['next_radial'],len(t)):
 position=(1-t[k])*base+t[k]*cell[args.apex];d=distance(position)
 factor=6*volume*(1-t[k])**2*wt[k]
 saved['integral']+=float(factor*(angular@density(d)));saved['mass']+=float(factor*angular.sum())
 saved['next_radial']=k+1;saved['elapsed_seconds']=offset+time.monotonic()-start
 if k%16==15:
  path.write_text(json.dumps(saved,indent=2)+'\n')
  print('REFERENCE_PROGRESS',key,k+1,len(t),flush=True)
assert abs(saved['mass']/volume-1)<2e-12
saved['complete']=True;saved['radial_nodes']=len(t);saved['volume_relative_error']=abs(saved['mass']/volume-1)
saved['scope']='Exact nearest-triangle normal-density oracle, independent product Gauss/Duffy; radial order/bands and all4 apex choices must converge separately. This result alone is not a full tensor-density or remeshing proof.'
path.write_text(json.dumps(saved,indent=2)+'\n');print('REFERENCE_COMPLETE',key,saved['integral'],saved['elapsed_seconds'],flush=True)
