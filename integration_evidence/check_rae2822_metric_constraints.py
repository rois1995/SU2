"""Audit the frozen RAE2822 metric comparison (requires NumPy).

An optional CASE_FOLDER selects a later replay (default: rae-native-metric-v2).
BUILD_DIRECTORY contains rae-hessian/inputs/{mesh.su2,solution.dat},
rae-native-metric-v2/{weighted,quadratic}_least_squares-mpi{1,2,4}/{fields.csv,solver.log},
and SU2_CFD/src/SU2_CFD. The fixture uses AIRFOIL, h0=1e-5, g=1.2, T=.02,
hgrad=1.3 and target complexity 60000. No remeshing or flow iterations are audited.
Writes rae-native-metric-v2/validation.json; assertions fail on invalid tensors,
BL spacing/coupling, complexity, MPI disagreement or changed restart fields.
"""
from pathlib import Path
import csv,json,re,struct,hashlib,sys
import numpy as np
if len(sys.argv) not in (2,3): raise SystemExit('Usage: check_rae2822_metric_constraints.py BUILD_DIRECTORY [CASE_FOLDER]')
foldername=sys.argv[2] if len(sys.argv)==3 else 'rae-native-metric-v2'
root=Path(sys.argv[1]).resolve()
inputs=root/'rae-hessian/inputs'
def load(case):
 with (case/'fields.csv').open() as f: names=next(csv.reader(f))
 a=np.loadtxt(case/'fields.csv',delimiter=',',skiprows=1);a=a[np.argsort(a[:,0])]
 return {name:a[:,i] for i,name in enumerate(names)}
def tensor(d,p):
 A=np.zeros((len(d['x']),2,2));A[:,0,0]=d[p+'_XX'];A[:,0,1]=A[:,1,0]=d[p+'_XY'];A[:,1,1]=d[p+'_YY'];return A
first=load(root/foldername/'weighted_least_squares-mpi1');xy=np.column_stack([first['x'],first['y']])
lines=(inputs/'mesh.su2').read_text().splitlines();start=lines.index('MARKER_TAG= AIRFOIL');count=int(lines[start+1].split('=')[1]);edges=np.array([list(map(int,line.split()[1:3])) for line in lines[start+2:start+2+count]])
a=xy[edges[:,0]];e=xy[edges[:,1]]-a;length=np.linalg.norm(e,axis=1);en=np.column_stack([-e[:,1],e[:,0]])/length[:,None]
vn=np.zeros_like(xy)
np.add.at(vn,edges[:,0],en);np.add.at(vn,edges[:,1],en);wall=np.unique(edges);vn[wall]/=np.linalg.norm(vn[wall],axis=1)[:,None]
distance=np.empty(len(xy));normal=np.empty_like(xy)
for begin in range(0,len(xy),256):
 X=xy[begin:begin+256];delta=X[:,None,:]-a;along=np.clip(np.einsum('bsi,si->bs',delta,e)/(length**2),0,1);r=delta-along[:,:,None]*e;d=np.linalg.norm(r,axis=2);index=np.argmin(d,axis=1);row=np.arange(len(X));dist=d[row,index];t=along[row,index];n=en[index].copy();endpoint=(t==0)|(t==1);radial=endpoint&(dist>1e-12*length[index]);n[radial]=r[row,index][radial]/dist[radial,None];atvertex=endpoint&~radial;vertex=edges[index,(t>.5).astype(int)];n[atvertex]=vn[vertex[atvertex]];distance[begin:begin+len(X)]=dist;normal[begin:begin+len(X)]=n
ntri=int(lines[1].split('=')[1]);tri=np.array([list(map(int,line.split()[1:4])) for line in lines[2:2+ntri]]);v=xy[tri];area=abs((v[:,1,0]-v[:,0,0])*(v[:,2,1]-v[:,0,1])-(v[:,1,1]-v[:,0,1])*(v[:,2,0]-v[:,0,0]))/2;volume=np.zeros(len(xy));np.add.at(volume,tri.ravel(),np.repeat(area/3,3))
full=distance<=.018;hn=np.maximum(1e-5,2*(1e-5+.2*distance)/2.2);tangent=np.column_stack([-normal[:,1],normal[:,0]])
binary=inputs/'solution.dat'
with binary.open('rb') as f:
 header=struct.unpack('5i',f.read(20));names=[f.read(33).split(b'\0')[0].decode() for _ in range(header[1])];original=np.fromfile(f,dtype=np.float64,count=header[1]*header[2]).reshape(header[2],header[1])
summary={'scope':'Frozen adapted RAE2822 RANS/SA restart, zero flow iterations; metric validation only. Closest segment normals independently reconstructed from the wall mesh. No exact Hessian or remeshing accuracy claim.','full_band_points':int(full.sum()),'wall_vertices':len(wall),'cases':{}}
for folder,target in [(foldername,60000)]:
 out=summary['cases'].setdefault(str(target),{})
 methods=['weighted_least_squares','quadratic_least_squares']
 if (root/folder/'quadratic_least_squares_noise-mpi1').exists(): methods.append('quadratic_least_squares_noise')
 for method in methods:
  base=load(root/folder/(method+'-mpi1'));M=tensor(base,'Metric');val=np.linalg.eigvalsh(M);projection=np.einsum('ni,nij,nj->n',normal,M,normal);cross=np.einsum('ni,nij,nj->n',tangent,M,normal);size=1/np.sqrt(projection);log=(root/folder/(method+'-mpi1')/'solver.log').read_text();complexity=float(re.search(r'Mesh complexity with the boundary-layer metric: ([\d.e+\-]+)',log)[1]);entry={'reported_complexity':complexity,'independently_integrated_complexity':float(np.sum(np.sqrt(val[:,0]*val[:,1])*volume)),'minimum_metric_eigenvalue':float(val.min()),'maximum_metric_aspect_ratio':float(np.sqrt(val[:,1]/val[:,0]).max()),'full_band_max_relative_normal_size_error':float(np.max(abs(size[full]/hn[full]-1))),'full_band_max_relative_normal_tangent_coupling':float(np.max(abs(cross[full])*hn[full]**2)),'wall_min_normal_size_over_first_height':float((size[wall]/1e-5).min()),'mpi':{}}
  raw_edges=np.concatenate([tri[:,[0,1]],tri[:,[1,2]],tri[:,[2,0]]]);raw_edges=np.unique(np.sort(raw_edges,axis=1),axis=0)
  directed=np.concatenate([raw_edges,raw_edges[:,::-1]])
  dst,src=directed.T;dx=xy[dst]-xy[src]
  distance2=np.einsum('ni,nij,nj->n',dx,M[src],dx)
  factor=1/(1+np.log(1.3)*np.sqrt(distance2))**2
  dv,Q=np.linalg.eigh(M[dst]);inverse=Q/np.sqrt(dv)[:,None,:]
  white=np.einsum('nai,nab,nbj->nij',inverse,M[src]*factor[:,None,None],inverse)
  ratios=np.linalg.eigvalsh(white)[:,-1]
  entry['gradation_max_full_band_ratio']=float(ratios[full[dst]].max())
  entry['gradation_max_outer_ratio']=float(ratios[~full[dst]].max())
  entry['gradation_edges_above_tolerance']=int((ratios>1+1e-5).sum())
  entry['gradation_report']=re.search(r'Native metric gradation: (.*)',log)[1]
  entry['reference_conflict_report']=re.search(r'WARNING: Native reference chord.*',log)[0]
  assert np.isfinite(M).all() and val.min()>0
  assert entry['full_band_max_relative_normal_size_error']<1e-8
  assert entry['full_band_max_relative_normal_tangent_coupling']<1e-8
  for field in ['Density','Momentum_x','Momentum_y','Energy','Nu_Tilde']:
   assert np.allclose(base[field],original[:,names.index(field)],rtol=1e-14,atol=0)
  for rank in [2,4]:
   d=load(root/folder/(method+f'-mpi{rank}'));B=tensor(d,'Metric')
   Bval=np.linalg.eigvalsh(B)
   assert np.isfinite(B).all() and Bval.min()>0
   Bnormal=np.einsum('ni,nij,nj->n',normal,B,normal)
   Bcross=np.einsum('ni,nij,nj->n',tangent,B,normal)
   normal_error=float(np.max(abs(1/np.sqrt(Bnormal[full])/hn[full]-1)))
   coupling_error=float(np.max(abs(Bcross[full])*hn[full]**2))
   assert normal_error<1e-8 and coupling_error<1e-8
   for field in ['Density','Momentum_x','Momentum_y','Energy','Nu_Tilde']:
    assert np.allclose(d[field],original[:,names.index(field)],rtol=1e-14,atol=0)
   error=float((np.linalg.norm(M-B,axis=(1,2))/np.linalg.norm(M,axis=(1,2))).max());Herror=max(float(np.max(abs(tensor(base,'Hessian_'+s)-tensor(d,'Hessian_'+s)))) for s in ['MACH','PRESSURE']);Hrelative={}
   for sensor in ['MACH','PRESSURE']:
    A=tensor(base,'Hessian_'+sensor);D=tensor(d,'Hessian_'+sensor);norm=np.linalg.norm(A,axis=(1,2))
    Hrelative[sensor]=float(np.max(np.linalg.norm(A-D,axis=(1,2))/np.maximum(norm,1e-14*norm.max())))
   entry['mpi'][str(rank)]={'max_relative_metric_difference':error,'max_absolute_hessian_difference':Herror,'max_relative_hessian_difference_with_global_floor':Hrelative,'minimum_metric_eigenvalue':float(Bval.min()),'full_band_max_relative_normal_size_error':normal_error,'full_band_max_relative_normal_tangent_coupling':coupling_error};assert error<1e-9
  if target==60000: assert abs(entry['independently_integrated_complexity']/target-1)<2e-6
  else: assert 'Minimum attainable complexity' in log and complexity>target
  out[method]=entry
summary['binary_sha256']=hashlib.sha256((root/'SU2_CFD/src/SU2_CFD').read_bytes()).hexdigest()
(root/foldername/'validation.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
