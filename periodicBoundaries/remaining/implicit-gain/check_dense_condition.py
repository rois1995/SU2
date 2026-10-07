import numpy as np,json
from pathlib import Path
n=5;variables=5;size=n**3*variables
P=np.eye(size)
for z in range(n):
 for y in range(n):
  i=(z*n+y)*n;j=i+n-1
  for a in range(variables):
   ii=i*variables+a;jj=j*variables+a
   P[ii,ii]=P[jj,jj]=P[ii,jj]=P[jj,ii]=.5
results=[]
for diagonal in (3,20):
 A=np.zeros((size,size))
 for z in range(n):
  for y in range(n):
   for x in range(n):
    i=(z*n+y)*n+x
    coords=[(x,y,z)]+[(x+dx,y+dy,z+dz) for dx,dy,dz in ((-1,0,0),(1,0,0),(0,-1,0),(0,1,0),(0,0,-1),(0,0,1)) if 0<=x+dx<n and 0<=y+dy<n and 0<=z+dz<n]
    for xx,yy,zz in coords:
     j=(zz*n+yy)*n+xx
     for a in range(variables):
      for b in range(variables):A[i*variables+a,j*variables+b]=(diagonal if i==j and a==b else 0)+.001*(1+a+2*b)+(0 if i==j else .002*(1+i))
 K=P@A@P+np.eye(size)-P
 s=np.linalg.svd(K,compute_uv=False)
 results.append({'diagonal':diagonal,'condition_2':float(s[0]/s[-1]),'min_singular_value':float(s[-1]),'max_singular_value':float(s[0])})
r=Path(__file__).resolve().parent
(r/'dense-condition.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results,indent=2))
