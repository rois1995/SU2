exec(open('integration_evidence/native_3d_bl_v1/coupled_engine_v1/reconstruction_experiments/native_bl_flip_probe.py').read().split('pts[200000]=')[0])
orig=dict(pts);best=None;results=[]
for height in [.2,.22,.24,.26,.28,.30,.32,.34,.36,.38]:
 for inner in [.2,.24,.28,.32,.36,.4,.44,.48]:
  for z in [.12,.16,.1687335,.2,.24,.28,.32,.36,.4]:
   pts=dict(orig);pts[31]=[height,0,height];pts[47]=[0,height,height];pts[15]=[inner,inner,inner];pts[200000]=[0,0,z];scores=[];invalid=False
   for row in v['cells']:
    c=row['nodes'];cs=[c]
    if 0 in c and 4 in c:cs=[[200000 if x==0 else x for x in c],[200000 if x==4 else x for x in c]]
    for c in cs:
     t=[pts[x] for x in c];det=dot(sub(t[1],t[0]),cross(sub(t[2],t[0]),sub(t[3],t[0])))
     if det<=0:invalid=True;break
     scores.append(measure(c))
    if invalid:break
   if invalid:continue
   score=min(min(q/.2,j/.05) for q,j,_ in scores)
   if best is None or score>best[0]:best=(score,height,inner,z,min(x[0] for x in scores),min(x[1] for x in scores))
print('Best inherited-connectivity shape placement',best)
