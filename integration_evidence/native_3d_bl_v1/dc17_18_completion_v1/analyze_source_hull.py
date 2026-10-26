"""Numerical diagnosis only; production admission uses outward-rounded bounds."""
import hashlib,itertools,json,sys
from pathlib import Path
import numpy as np
import scipy
from scipy.optimize import linprog

def separate(a,b):
    axes=[]
    for i,j,k in itertools.combinations(range(len(a)),3):
        axes.append(np.cross(a[j]-a[i],a[k]-a[i]))
    for i,j in itertools.combinations(range(len(a)),2):
        for k in range(3):axes.append(np.cross(a[j]-a[i],b[(k+1)%3]-b[k]))
    axes.extend([np.cross(b[1]-b[0],b[2]-b[0]),*np.eye(3)])
    axes=np.array(axes);axes=axes[np.linalg.norm(axes,axis=1)>1e-25]
    axes/=np.linalg.norm(axes,axis=1)[:,None]
    x=a@axes.T;y=b@axes.T
    gaps=np.maximum(y.min(axis=0)-x.max(axis=0),x.min(axis=0)-y.max(axis=0));i=np.argmax(gaps)
    return float(gaps[i]),axes[i].tolist()

triangle=np.array([[0.,0,0],[1,0,0],[0,1,0]])
assert abs(separate(triangle,triangle+[0,0,2])[0]-2)<1e-12
assert abs(separate(triangle,triangle)[0])<1e-12
source,destination=map(Path,sys.argv[1:]);w=json.loads(source.read_text())
a=np.array(w['controls']+w['source']);b=np.array([r['point'] for r in w['columns']])
origin=b[0].copy();a-=origin;b-=origin
gap,axis=separate(a,b)
scale=max(np.max(np.abs(a)),np.max(np.abs(b)))
eq=np.zeros((5,len(a)+len(b)));eq[:3,:len(a)]=a.T/scale;eq[:3,len(a):]=-b.T/scale
eq[3,:len(a)]=1;eq[4,len(a):]=1
lp=linprog(np.zeros(eq.shape[1]),A_eq=eq,b_eq=[0,0,0,1,1],bounds=(0,None),method='highs')
result=dict(maximum_separating_gap=gap,axis=axis,margin=w['margin'],
            intersection_lp_status=lp.status,intersection_lp_message=lp.message,
            scipy_version=scipy.__version__,scope=__doc__,
            pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (source,Path(__file__))})
if lp.success:
    result.update(intersection_a=(lp.x[:len(a)]@a+origin).tolist(),intersection_b=(lp.x[len(a):]@b+origin).tolist())
destination.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
