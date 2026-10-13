exec(open('integration_evidence/native_3d_bl_v1/coupled_engine_v1/reconstruction_experiments/native_bl_flip_probe.py').read().split('pts[200000]=')[0])
orig=dict(pts);best=None;results=[]
for height in [.16+i*.002 for i in range(50)]:
 for z in [.08+i*.004 for i in range(111)]:
  pts=dict(orig);pts[31]=[height,0,height];pts[47]=[0,height,height];pts[200000]=[0,0,z];scores=[]
  for row in v['cells']:
   c=row['nodes'];cs=[c]
   if 0 in c and 4 in c:cs=[[200000 if x==0 else x for x in c],[200000 if x==4 else x for x in c]]
   for c in cs:scores.append(measure(c))
  score=min(min(q/.2,j/.05) for q,j,_ in scores)
  if best is None or score>best[0]:best=(score,height,z,min(x[0] for x in scores),min(x[1] for x in scores))
print('Best inherited-connectivity shape placement',best)
