"""Compare complete cylindrical wall samplings at fixed height in the frozen metric."""
import json,math,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from audit_full_metric import CylinderField
from construct import sha
start=time.monotonic();source=HERE.parent/'native_front_constant_v1';field=CylinderField(source,'core_construction_rejected.json');rows=[]
assert np.array_equal(field.axis,[0.,0.,1.])
def measures(triangles,apices):
 p=np.concatenate((apices[:,None,:],triangles),axis=1);m=field.batch(p.mean(axis=1));edges=p[:,None,:,:]-p[:,:,None,:]
 squared=np.einsum('nabi,nij,nabj->nab',edges,m,edges);weighted=np.abs(np.linalg.det(p[:,1:]-p[:,:1]))*np.sqrt(np.linalg.det(m));i,j=np.triu_indices(4,1)
 q=12*(weighted/2)**(2/3)/squared[:,i,j].sum(axis=1)
 products=np.array([np.prod(np.delete(squared[:,k,:],k,axis=1),axis=1) for k in range(4)]).T
 jac=np.min(math.sqrt(2)*weighted[:,None]/np.sqrt(products),axis=1)
 return q,jac
for count in range(36,43):
 triangles=[]
 for i in range(count):
  a=2*math.pi*i/count;b=2*math.pi*(i+1)/count
  p=np.array([[math.cos(a),math.sin(a),0],[math.cos(b),math.sin(b),0],[math.cos(b),math.sin(b),.2],[math.cos(a),math.sin(a),.2]])
  choices=np.array([[p[0],p[1],p[2]],[p[0],p[2],p[3]],[p[0],p[1],p[3]],[p[1],p[2],p[3]]])
  center=choices.mean(axis=1);normal=np.cross(choices[:,1]-choices[:,0],choices[:,2]-choices[:,0]);normal/=np.linalg.norm(normal,axis=1)[:,None]
  base=center-field.h*normal;metric=field.batch((3*center+base)/4)
  direction=np.linalg.solve(metric,normal[...,None])[...,0];opt=center-field.h*direction/np.sum(normal*direction,axis=1)[:,None]
  oldq,oldj=measures(choices,base);newq,newj=measures(choices,opt)
  old=min(min(oldq[:2].min()/.2,oldj[:2].min()/.05),min(oldq[2:].min()/.2,oldj[2:].min()/.05))
  best_old=max(min(oldq[:2].min()/.2,oldj[:2].min()/.05),min(oldq[2:].min()/.2,oldj[2:].min()/.05))
  best_new=max(min(newq[:2].min()/.2,newj[:2].min()/.05),min(newq[2:].min()/.2,newj[2:].min()/.05))
  selected=0 if min(newq[:2].min()/.2,newj[:2].min()/.05)>=min(newq[2:].min()/.2,newj[2:].min()/.05) else 2
  # Actual geometric normal feet remain in the face, not merely in its plane.
  feet=opt+field.h*normal;ab=choices[:,1]-choices[:,0];ac=choices[:,2]-choices[:,0];relative=feet-choices[:,0]
  gram=np.stack((np.stack((np.sum(ab*ab,axis=1),np.sum(ab*ac,axis=1)),axis=1),np.stack((np.sum(ab*ac,axis=1),np.sum(ac*ac,axis=1)),axis=1)),axis=1)
  uv=np.linalg.solve(gram,np.stack((np.sum(ab*relative,axis=1),np.sum(ac*relative,axis=1)),axis=1)[...,None])[...,0]
  weights=np.column_stack((1-uv.sum(axis=1),uv));assert np.all(weights>=-1e-12)
  assert np.max(np.abs(np.sum((center-opt)*normal,axis=1)-field.h))<1e-14
  triangles.append((best_old,best_new,newq[selected:selected+2].min(),newj[selected:selected+2].min(),weights[selected:selected+2].min()))
 rows.append({'angular_intervals':count,'wall_triangles':2*count,'minimum_old_shape_gate_ratio':min(r[0] for r in triangles),'minimum_metric_apex_shape_gate_ratio':min(r[1] for r in triangles),'minimum_new_q':min(r[2] for r in triangles),'minimum_new_j':min(r[3] for r in triangles),'minimum_normal_foot_barycentric':min(r[4] for r in triangles)})
report={'status':'WALL_SAMPLING_METRIC_APEX_DIAGNOSIS_ONLY','rows':rows,'helper_sha256':sha(Path(__file__)),'field_helper_sha256':sha(HERE.parent/'audit_full_metric.py'),'source_rejection_sha256':sha(source/'core_construction_rejected.json'),'elapsed_seconds':time.monotonic()-start,
        'scope':'Whole regular cylindrical-wall sampling with both diagonals, actual frozen sensor-plus-BL at actual tetra centroids, exact-height and contained-foot arithmetic checks. Metric-nearest apex uses M^-1*n/(n^T*M^-1*n) from the initial tetra centroid. Not a complete volume mesh, full surface-edge/embedding/work gate or native acceptance; no change to the target or constraints.'}
(HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
