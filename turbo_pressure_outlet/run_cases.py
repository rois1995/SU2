from pathlib import Path
import argparse,csv,hashlib,json,math,os,subprocess,time
root=Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('--binary',required=True)
parser.add_argument('--ranks',type=int,default=2)
parser.add_argument('--threads',type=int,default=1)
parser.add_argument('cases',nargs='+')
a=parser.parse_args()
for name in a.cases:
 d=root/name
 start=time.monotonic()
 with (d/'solver.log').open('w') as log:
  run=subprocess.run(['mpirun','-n',str(a.ranks),a.binary,'case.cfg'],cwd=d,stdout=log,stderr=subprocess.STDOUT,env=dict(os.environ,OMP_NUM_THREADS=str(a.threads)))
 target=next(float(line.split('=',1)[1]) for line in (d/'case.cfg').read_text().splitlines() if line.strip().startswith('CONV_RESIDUAL_MINVAL='))
 result=dict(density_residual_target=target,case=name,binary=a.binary,binary_sha256=hashlib.sha256(Path(a.binary).read_bytes()).hexdigest(),ranks=a.ranks,threads=a.threads,exit_code=run.returncode,seconds=time.monotonic()-start)
 def rows(path):
  with path.open()as f:return [{k.strip().strip('"'):float(v)for k,v in row.items()}for row in csv.DictReader(f)]
 if (d/'history.csv').exists():
  history=rows(d/'history.csv');result['last']=history[-1] if history else None;result['finite_history']=bool(history) and all(math.isfinite(v)for row in history for v in row.values());result['converged']=run.returncode==0 and bool(history) and result['last']['rms[Rho]']<=target
 if (d/'restart_flow.csv').exists():
  field=rows(d/'restart_flow.csv')
  result['finite_field']=all(math.isfinite(v)for row in field for v in row.values())
  outlet=sorted([row for row in field if abs(row['x']-1)<1e-12],key=lambda row:row['y'])
  if outlet:
   height=outlet[-1]['y']-outlet[0]['y']
   def average(key):return sum(.5*(left[key]+right[key])*(right['y']-left['y'])for left,right in zip(outlet,outlet[1:]))/height
   result['outlet_average']={key:average(key)for key in ['Density','Pressure','Mach','Momentum_x']}
   result['outlet_massflow']=result['outlet_average']['Momentum_x']*height
 (d/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result),flush=True)
