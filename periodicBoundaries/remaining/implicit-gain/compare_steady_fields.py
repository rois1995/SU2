from pathlib import Path
import csv, json
base=Path(__file__).resolve().parent
r=base/'implicit_gain' if (base/'implicit_gain').is_dir() else base
results=[]
for case in ('rotation45-linear50','axis30'):
 def data(mode):
  with (r/case/mode/'restart.csv').open() as f:
   return [{k:float(v) for k,v in row.items()} for row in csv.DictReader(f)]
 a,b=data('legacy'),data('fixed')
 assert len(a)==len(b)
 assert all(x['PointID']==y['PointID'] and all(x[k]==y[k] for k in ('x','y')) for x,y in zip(a,b))
 for field in ('Density','Momentum_x','Momentum_y','Momentum_z','Energy','Pressure','Temperature'):
  if field not in a[0]:continue
  scale=max(1,max(abs(x[field]) for x in a))
  error=max(abs(x[field]-y[field]) for x,y in zip(a,b))
  results.append({'case':case,'field':field,'max_abs_difference':error,'relative_to_max_baseline_or_one':error/scale})
  assert error/scale < 1e-5, (case,field,error,scale)
(r/'steady-field-comparison.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS: both converged cases agree in primary and thermodynamic fields within 1e-5 on the stated scales.')
