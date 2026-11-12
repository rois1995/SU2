"""Frozen-centroid tensor, planar wall-footprint control only; not curved-field feasibility or acceptance."""
from pathlib import Path
from itertools import combinations
import hashlib,json,sys
import numpy as np
import assess_saved_row369 as previous
import assess_saved_row399 as plane
R=Path(__file__).resolve().parent;LD=np.longdouble
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def optimum(base,matrix,h0,tolerance):
    base=np.asarray(base,dtype=LD);base=base-base.mean(axis=0);m=np.asarray(matrix,dtype=LD);m=m/max(abs(m).flat)
    n=np.cross(base[1]-base[0],base[2]-base[0]);area2=np.sqrt(n@n);n/=area2
    l=plane.factor(m);det=l[0,0]*l[1,1]*l[2,2];S=sum((base[j]-base[i])@m@(base[j]-base[i]) for i,j in ((0,1),(0,2),(1,2)))
    best=None
    for count in (1,2,3):
      for subset in combinations(range(3),count):
        anchor=base[subset[0]];D=(base[list(subset[1:])]-anchor).T
        if count>1:
          G=D.T@m@D;rhs=np.stack((-D.T@m@anchor,-D.T@m@n),axis=1)
          if count==2:w=rhs/G[0,0]
          else:
            determinant=G[0,0]*G[1,1]-G[0,1]*G[1,0];assert determinant>0
            w=np.array([[G[1,1]*rhs[0,k]-G[0,1]*rhs[1,k] for k in range(2)],[G[0,0]*rhs[1,k]-G[1,0]*rhs[0,k] for k in range(2)]])/determinant
          offset=anchor+D@w[:,0];direction=n+D@w[:,1]
          bary=np.concatenate((np.array([[1-w[:,0].sum(),-w[:,1].sum()]]),w))
        else:offset=anchor;direction=n;bary=np.array([[1,0]],dtype=LD)
        lo=LD(h0)*(1-LD(tolerance));hi=LD(h0)*(1+LD(tolerance))
        for c0,c1 in bary:
          if c1>0:lo=max(lo,-c0/c1)
          elif c1<0:hi=min(hi,-c0/c1)
          elif c0<0:hi=-1
        if hi<lo or lo<=0:continue
        a=LD(4)/3*S+3*offset@m@offset;b=6*offset@m@direction;c=3*direction@m@direction;assert a>0 and c>0
        stationary=4*a/(b+np.sqrt(b*b+32*a*c))
        h=min(hi,max(lo,stationary));p=offset+h*direction
        denominator=S+sum((p-v)@m@(p-v) for v in base)
        q=12*np.cbrt(area2*h*det/2)**2/denominator
        weights=np.zeros(3,dtype=LD)
        for identity,coeff in zip(subset,bary):weights[identity]=coeff[0]+coeff[1]*h
        assert min(weights)>-1e-10 and abs(weights.sum()-1)<1e-10
        if best is None or q>best['q_max']:best=dict(q_max=float(q),plane_height=float(h),foot_barycentric=weights.astype(float).tolist())
    assert best is not None
    return best

def main():
    triangle=np.array([[0,0,0],[1,0,0],[.5,np.sqrt(LD(3))/2,0]],dtype=LD)
    assert abs(optimum(triangle,np.eye(3),np.sqrt(LD(2)/3),0)['q_max']-1)<1e-14
    case=R/'cases/curved399_checkpoint_target_patches_20261111T231039_3485346';command=case/'command.json';env=json.loads(command.read_text())['environment'];patch=Path(env['SU2_NATIVE_BL_SAVED_PATCH']);frames=case/'saved_prepared_repair_before_frames.jsonl'
    cells,faces=previous.read_patch(patch);rows=[json.loads(x) for x in frames.read_text().splitlines()];results=[]
    for row in rows:
      if not row['wall_child']:continue
      ids,xyz=cells[row['cell']];face=next(f for f in faces if set(f[:3]).issubset(ids));base=np.asarray([xyz[ids.index(i)] for i in face[:3]],dtype=LD)
      xx,xy,xz,yy,yz,zz=row['metric'];M=np.array([[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]],dtype=LD)
      constrained=optimum(base,M,LD('2e-6'),LD('.1'));assert row['q']<=constrained['q_max']+1e-10
      # Independent double solve only reports where the unrestricted plane optimum would put its foot.
      n=np.cross(base[1]-base[0],base[2]-base[0]);n/=np.sqrt(n@n);apex=next(xyz[k] for k,i in enumerate(ids) if i not in face[:3]);n*=np.sign((apex-base[0])@n)
      z=np.linalg.solve(np.asarray(M,float),np.asarray(n,float));p=np.asarray(base.mean(axis=0),float)+2e-6*z/(np.asarray(n,float)@z);foot=p-2e-6*np.asarray(n,float)
      uv=np.linalg.lstsq(np.asarray((base[1:]-base[0]).T,float),foot-np.asarray(base[0],float),rcond=None)[0]
      results.append(dict(cell=row['cell'],baseline_q=row['q'],plane_footprint_control=constrained,unrestricted_plane_optimal_foot_barycentric=[float(1-uv.sum()),*uv.tolist()]))
    assert len(results)==5
    result=dict(scope=__doc__,results=results,input_sha256={str(p):sha(p) for p in (command,patch,frames,Path(plane.__file__),Path(previous.__file__))},helper_sha256=sha(__file__),limits=['Optimization is exact active-face algebra in a constant centroid tensor with a flat triangular footprint and plane-height interval.','Curved nearest-foot support, varying target, neighboring cells and global clearance are not modeled; not a full-field impossibility proof.','No production metric, operator, position or gate changed.'])
    output=R/'row_footprint402_assessment.json';assert not output.exists();output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n');print(json.dumps(results))
if __name__=='__main__':main()
