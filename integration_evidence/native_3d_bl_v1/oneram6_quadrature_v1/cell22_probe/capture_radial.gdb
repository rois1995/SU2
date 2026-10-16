set pagination off
set confirm off
set print elements 3
catch throw
run
up
info locals
python
import gdb,json
from pathlib import Path
r={}
for name in ['left','right','depth','low','high','higher','totalValue','integral','volume','cellVolume','radialError']:
 try:r[name]=float(gdb.parse_and_eval(name))
 except gdb.error:r[name]=None
for name in ['lowPoints','highPoints','probePoints']:
 v=gdb.parse_and_eval(name);p=v['_M_impl']['_M_start'];n=int(v['_M_impl']['_M_finish']-p);r[name]=[]
 for k in range(n):
  point=p[k];samples=point['samples'];sp=samples['_M_impl']['_M_start'];sn=int(samples['_M_impl']['_M_finish']-sp)
  row={'barycentric':[float(point['barycentric']['_M_elems'][i]) for i in range(4)],'weight':float(point['weight']),'samples':[]}
  for j in range(sn):
   sample=sp[j]['sample'];row['samples'].append({'distance':float(sample['distance']),'hn':float(sample['hn']),'fade':float(sample['weight']),'normal':[float(sample['normal'][i]) for i in range(3)],'full':[[float(sample['full']['m'][a][b]) for b in range(3)] for a in range(3)],'metric':[[float(sample['metric']['m'][a][b]) for b in range(3)] for a in range(3)]})
  r[name].append(row)
Path('radial_failure_values.json').write_text(json.dumps(r,indent=2)+'\n');print('Saved actual rejected radial rules and wall samples.')
end
