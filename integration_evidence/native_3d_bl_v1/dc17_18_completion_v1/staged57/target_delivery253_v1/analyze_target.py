"""Same-seed target diagnosis; no remeshing or production decisions."""
from pathlib import Path
import csv, hashlib, json, sys
import numpy as np
HERE=Path(__file__).resolve().parent;R=HERE.parent;REPO=R.parents[3]
sys.path.insert(0,str(REPO/'TestCases/adaptation/capability'))
import capcheck
source=R/'metric_target253_hessian_trace_v1/L1';mesh=capcheck.read_su2(source/'m6.su2')
P,F,_=capcheck.read_restart(source/'restart_flow.dat');assert np.array_equal(P,mesh.P)
E=mesh.E;T=mesh.M['WING3D'];n=len(P);V=P[E]
vol=np.abs(np.einsum('ni,ni->n',np.cross(V[:,1]-V[:,0],V[:,2]-V[:,0]),V[:,3]-V[:,0]))/6
assert (vol>0).all();dual=np.bincount(E.ravel(),weights=np.repeat(vol/4,4),minlength=n)
def matrix(prefix):
 m=np.zeros((n,3,3))
 for a,b,c in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:m[:,a,b]=m[:,b,a]=F[prefix+c]
 return m
def stats(v):
 v=np.asarray(v);assert v.size and np.isfinite(v).all()
 return dict(zip(('min','p10','median','p90','max'),map(float,np.quantile(v,[0,.1,.5,.9,1]))))
H=matrix('Hessian_MACH_');M=matrix('Metric_');he,Q=np.linalg.eigh(H);a=np.abs(he);a=np.maximum(a,1e-14*a.max(axis=1)[:,None]);present=a.max(axis=1)>0
logs=np.zeros_like(a);logs[present]=np.log(a[present]);ld=logs.sum(axis=1)
terms=ld[present]/5+np.log(dual[present]);ref=terms.max();log_integral=ref+np.log(np.exp(terms-ref).sum())
u=np.zeros_like(a);u[present]=np.exp(logs[present]+(2/3*(np.log(50000)-log_integral)-ld[present]/5)[:,None])
assert abs((dual*np.sqrt(u.prod(axis=1))).sum()/50000-1)<1e-10
# M_final dominates bounded input; major-eigenvector Rayleigh ratios locate unchanged directions.
i=np.argmax(a,axis=1);direction=Q[np.arange(n),:,i];ray=np.einsum('ni,nij,nj->n',direction,M,direction)
ratio=ray[present]/u[present].max(axis=1);scale=float(ratio.min());bounded=np.clip(scale*u,1e-4,1e12);bounded=np.maximum(bounded,bounded.max(axis=1)[:,None]/1e12)
pre=dual*np.sqrt(bounded.prod(axis=1));raw=dual*np.sqrt(u.prod(axis=1));final=dual*np.sqrt(np.linalg.det(M));assert abs(final.sum()/50000-1)<1e-6
report={'scope':'Independent reconstruction from saved production raw Hessian and final metric; inferred common scale is checked against rounded production pre-gradation complexity, not claimed as logged exact stage state. No target change.', 'points':n,'cells':len(E),'volume':float(vol.sum()),'raw_normalized_complexity':float(raw.sum()),'final_complexity':float(final.sum()),'inferred_common_scale':scale,'inferred_pre_gradation_complexity':float(pre.sum()),'production_pre_gradation_complexity_rounded':11961.4,'unchanged_major_directions_at_inferred_scale':int(np.count_nonzero(np.abs(ratio/scale-1)<1e-8)), 'shells':[]}
assert abs(pre.sum()/11961.4-1)<1e-4,(pre.sum(),scale)
radius=np.linalg.norm(P,axis=1)
for lo,hi in [(0,2),(2,5),(5,20),(20,50),(50,200)]:
 mask=(radius>=lo)&(radius<hi);eig=np.linalg.eigvalsh(M[mask]);report['shells'].append({'radius':[lo,hi],'points':int(mask.sum()),'volume_fraction':float(dual[mask].sum()/dual.sum()),'raw_complexity':float(raw[mask].sum()),'pre_gradation_complexity':float(pre[mask].sum()),'final_complexity':float(final[mask].sum()),'Mach':stats(F['Mach'][mask]),'raw_H_frobenius':stats(np.linalg.norm(H[mask],axis=(1,2))),'final_shortest_spacing':stats(1/np.sqrt(eig[:,-1])),'final_longest_spacing':stats(1/np.sqrt(eig[:,0]))})
# Actual plane/triangle cuts: no nearest-face association; piecewise-linear flow values.
cut_rows=[];cuts=[]
for y in [.2,.4,.6,.8,1.0]:
 rows=[]
 for face in T[(P[T,1].min(axis=1)<=y)&(P[T,1].max(axis=1)>y)]:
  intersections=[]
  for aa,bb in [(0,1),(1,2),(2,0)]:
   ia,ib=face[aa],face[bb];ya,yb=P[ia,1],P[ib,1]
   if (ya<=y<yb) or (yb<=y<ya):
    w=(y-ya)/(yb-ya);pos=(1-w)*P[ia]+w*P[ib]
    intersections.append([float(pos[0]),float(pos[2]),float((1-w)*F['Mach'][ia]+w*F['Mach'][ib]),float((1-w)*F['Pressure_Coefficient'][ia]+w*F['Pressure_Coefficient'][ib])])
  assert len(intersections)==2
  rows.extend(intersections)
 rows=np.unique(np.round(rows,13),axis=0);xmin,xmax=rows[:,0].min(),rows[:,0].max();chord=xmax-xmin
 summary={'y':y,'chord':float(chord),'branches':{}}
 for label,mask in [('upper',rows[:,1]>=0),('lower',rows[:,1]<0)]:
  arr=rows[mask];arr=arr[np.argsort(arr[:,0])];x=(arr[:,0]-xmin)/chord
  for xx,row in zip(x,arr):cut_rows.append([y,label,float(xx),*map(float,row)])
  dx=np.diff(x);valid=(dx>1e-8)&(x[:-1]>.15)&(x[1:]<.9);dM=np.diff(arr[:,2]);dCp=np.diff(arr[:,3]);compression=np.flatnonzero(valid&(dM<0)&(dCp>0));peaks=sorted(compression,key=lambda j:dM[j]/dx[j])[:3]
  summary['branches'][label]={'points':len(arr),'Mach_range':[float(arr[:,2].min()),float(arr[:,2].max())],'strongest_compression_segments':[{'x_over_c':[float(x[j]),float(x[j+1])],'resolved_width_over_c':float(dx[j]),'Mach_drop':float(-dM[j]),'Cp_rise':float(dCp[j])} for j in peaks]}
 cuts.append(summary)
with (HERE/'surface_cuts.csv').open('w') as f:
 w=csv.writer(f,lineterminator="\n");w.writerow(['y','side','x_over_c','x','z','Mach','Cp']);w.writerows(cut_rows)
report['surface_cuts']=cuts
# Independent bounded stencil estimate: complete graph one/two-rings; fixed sample cap.
pairs=np.concatenate([E[:,[a,b]] for a in range(4) for b in range(4) if a!=b]);keys=np.unique(pairs[:,0]*n+pairs[:,1]);neighbors=keys%n;starts=np.r_[0,np.cumsum(np.bincount(keys//n,minlength=n))]
surface=np.zeros(n,bool);surface[np.unique(T)]=True
C=P[T].mean(axis=1);U=P[T[:,1]]-P[T[:,0]];W=P[T[:,2]]-P[T[:,0]];G=np.stack([U,W],axis=1)
grad=np.einsum('ni,nij->nj',np.linalg.solve(G@G.transpose(0,2,1),(F['Mach'][T[:,1:]]-F['Mach'][T[:,0,None]])[...,None])[...,0],G)
# Directional restrictions are diagnostic, not production edge quadrature.
Mraw=(Q*u[:,None,:])@Q.transpose(0,2,1);Mpre=(Q*bounded[:,None,:])@Q.transpose(0,2,1)
normal=np.cross(U,W);normal/=np.linalg.norm(normal,axis=1)[:,None]
gnorm=np.linalg.norm(grad,axis=1);gdir=grad/np.maximum(gnorm[:,None],1e-300)
wing=P[np.unique(T)];report['directional_compression_strips']=[]
for y in np.linspace(.15,1.15,11):
 nodes=wing[abs(wing[:,1]-y)<.025];xi=nodes[:,0]-nodes[:,1]/np.sqrt(3);chord=np.ptp(xi);xc=(C[:,0]-C[:,1]/np.sqrt(3)-xi.min())/chord
 eligible=(abs(C[:,1]-y)<.025)&(C[:,2]>.002)&(xc>.15)&(xc<.8)&(grad[:,0]<0);cut=np.quantile(-grad[eligible,0],.8);mask=eligible&(-grad[:,0]>=cut)
 section={'y':float(y),'triangles':int(mask.sum()),'chord':float(chord)}
 for name,field in [('raw_normalized',Mraw),('bounded_pre_gradation',Mpre),('final',M)]:
  avg=field[T].mean(axis=1);h=1/np.sqrt(np.einsum('ni,nij,nj->n',gdir[mask],avg[mask],gdir[mask]));section[name+'_spacing_over_chord']=stats(h/chord)
 report['directional_compression_strips'].append(section)
# Highest upper compression in each station, LE, nearby interior, and farfield. Not full shock-curve detection.
samples=[]
for y in [.2,.4,.6,.8,1.0]:
 eligible=np.flatnonzero((abs(C[:,1]-y)<.03)&(C[:,2]>.002)&(C[:,0]-C[:,1]/np.sqrt(3)>.1)&(C[:,0]-C[:,1]/np.sqrt(3)<.6))
 shock=eligible[np.argmin(grad[eligible,0])];ids=list(T[shock]);samples.extend((int(j),'compression_surface') for j in ids)
 near=np.linalg.norm(P-C[shock],axis=1);near[surface]=np.inf;samples.extend((int(j),'compression_interior') for j in np.argsort(near)[:3])
 band=np.flatnonzero(surface&(abs(P[:,1]-y)<.03));xi=P[band,0]-P[band,1]/np.sqrt(3);samples.extend((int(j),'leading_edge') for j in band[np.argsort(xi)[:3]])
ff=np.flatnonzero(radius>20);samples.extend((int(j),'farfield') for j in ff[np.argsort(final[ff])[-12:]])
fit_rows=[]
for point,region in dict.fromkeys(samples):
 one=neighbors[starts[point]:starts[point+1]];two=np.unique(np.concatenate([one,*[neighbors[starts[j]:starts[j+1]] for j in one]]));two=two[two!=point]
 for ring,cloud in [(1,one),(2,two)]:
  dx=P[cloud]-P[point];_,sing,Vt=np.linalg.svd(dx,full_matrices=False);assert sing[-1]>0
  whitening=Vt.T@np.diag(np.sqrt(len(cloud))/sing);z=dx@whitening;weight=1/np.maximum(np.linalg.norm(z,axis=1),1e-12)
  D=np.column_stack([z,*[z[:,a]*z[:,b]*(.5 if a==b else 1) for a in range(3) for b in range(a,3)]]);D*=weight[:,None];rhs=(F['SENSOR_MACH'][cloud]-F['SENSOR_MACH'][point])*weight
  coef,_,rank,s=np.linalg.lstsq(D,rhs,rcond=1e-10);HH=np.zeros((3,3));k=3
  for a in range(3):
   for b in range(a,3):HH[a,b]=HH[b,a]=coef[k];k+=1
  HH=whitening@HH@whitening.T;de,dq=np.linalg.eigh(HH);order=np.argmax(abs(de));pe,pq=np.linalg.eigh(H[point]);po=np.argmax(abs(pe));norm=max(np.linalg.norm(H[point]),1e-300)
  fit_rows.append({'point':point,'region':region,'ring':ring,'samples':len(cloud),'rank':int(rank),'design_condition':float(s[0]/s[-1]),'displacement_condition':float(sing[0]/sing[-1]),'relative_residual':float(np.linalg.norm(D@coef-rhs)/max(np.linalg.norm(rhs),1e-300)),'H_relative_difference':float(np.linalg.norm(HH-H[point])/norm),'independent_H_norm':float(np.linalg.norm(HH)),'production_H_norm':float(norm),'dominant_axis_abs_dot':float(abs(dq[:,order]@pq[:,po])),'coord':P[point].tolist()})
report['independent_quadratic_samples']=fit_rows
report['fit_scope']='At most 57 representative points/two rings; SVD least squares independent solver. Pure quadratic only, NOT production thin-wall cubic/quartic selection or per-node warning localization; nonsmooth truncation and stencil sensitivity remain.'
files=[source/'m6.su2',source/'baseline_euler_seed.dat',source/'restart_flow.dat',source/'run.cfg',Path(__file__)]
report['sha256']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
(HERE/'target_diagnosis.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:report[k] for k in ['raw_normalized_complexity','inferred_common_scale','inferred_pre_gradation_complexity','final_complexity','unchanged_major_directions_at_inferred_scale']}));print('sample fits',len(fit_rows))
