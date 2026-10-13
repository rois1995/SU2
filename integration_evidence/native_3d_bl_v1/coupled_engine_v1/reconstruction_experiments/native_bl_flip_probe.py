import json,math,itertools
from pathlib import Path
v=json.loads(Path('integration_evidence/native_3d_bl_v1/coupled_engine_v1/independent_coupled_BL_rejected_n1.json').read_text());pts={n[0]:n[1:] for n in v['nodes']}
def sub(a,b):return [x-y for x,y in zip(a,b)]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def measure(c):
 t=[pts[x] for x in c];det=dot(sub(t[1],t[0]),cross(sub(t[2],t[0]),sub(t[3],t[0])))
 if det==0:return (0,0,0)
 z=sum(x[2] for x in t)/4;u=max(0,min(1,(z-.27)/.03));w=1-u*u*(3-2*u);tang=1.5**w;h=max(.1,2*(.1+.2*z)/2.2);n=h**(-2*w)
 def qd(d):return (1+tang)/2*(d[0]**2+d[1]**2)+(1-tang)*d[0]*d[1]+n*d[2]**2
 sq={(i,j):qd(sub(t[j],t[i])) for i,j in itertools.combinations(range(4),2)};vol=abs(det)*math.sqrt(tang*n)
 q=12*(vol/2)**(2/3)/sum(sq.values());j=min(1,min(math.sqrt(2)*vol/math.prod(math.sqrt(sq[tuple(sorted((i,k)))]) for k in range(4) if k!=i) for i in range(4)))
 return q,j,abs(det)
pts[200000]=[0,0,.16873350527066577];cells=[]
for row in v['cells']:
 c=row['nodes']
 if 0 in c and 4 in c:cells.extend([[200000 if x==0 else x for x in c],[200000 if x==4 else x for x in c]])
 else:cells.append(c)
for step in range(12):
 scores=[measure(c) for c in cells];print('step',step,'minq',min(x[0] for x in scores),'minj',min(x[1] for x in scores),'cells',len(cells))
 bad={i for i,m in enumerate(scores) if m[0]<.2 or m[1]<.05};best=None
 faces={}
 for i,c in enumerate(cells):
  for f in itertools.combinations(c,3):faces.setdefault(tuple(sorted(f)),[]).append(i)
 for f,owners in faces.items():
  if len(owners)!=2 or not bad.intersection(owners):continue
  i,j=owners;a=next(x for x in cells[i] if x not in f);b=next(x for x in cells[j] if x not in f)
  nc=[[a,b,f[k],f[(k+1)%3]] for k in range(3)];ms=[measure(c) for c in nc]
  if abs(sum(m[2] for m in ms)-scores[i][2]-scores[j][2])>1e-13:continue
  before=min(scores[i][0]/.2,scores[j][0]/.2,scores[i][1]/.05,scores[j][1]/.05)
  after=min(min(m[0]/.2,m[1]/.05) for m in ms)
  if after>before+1e-6 and (best is None or after-before>best[0]):best=(after-before,owners,nc,after)
 edges={}
 for i,c in enumerate(cells):
  for e in itertools.combinations(c,2):edges.setdefault(tuple(sorted(e)),[]).append(i)
 for e,owners in edges.items():
  if len(owners)!=3 or not bad.intersection(owners):continue
  ring=sorted(set(x for i in owners for x in cells[i])-set(e))
  if len(ring)!=3:continue
  nc=[ring+[e[k]] for k in range(2)];ms=[measure(c) for c in nc]
  if abs(sum(m[2] for m in ms)-sum(scores[i][2] for i in owners))>1e-13:continue
  before=min(min(scores[i][0]/.2,scores[i][1]/.05) for i in owners)
  after=min(min(m[0]/.2,m[1]/.05) for m in ms)
  if after>before+1e-6 and (best is None or after-before>best[0]):best=(after-before,owners,nc,after)
 if best is None:break
 _,owners,nc,after=best;print('flip', [cells[x] for x in owners], 'to',nc,'score',after)
 cells=[c for i,c in enumerate(cells) if i not in owners]+nc
