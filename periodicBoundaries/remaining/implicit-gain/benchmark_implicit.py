from pathlib import Path
import subprocess, os, csv, json, time, shutil, re, sys
r=Path(__file__).resolve().parent
out=r/'implicit_gain';out.mkdir(exist_ok=True)
bundle=r.parent/'pr_bundle/periodicBoundaries'
binary=r/'build_gain/SU2_CFD/src/SU2_CFD'
def configure(name,mode,limit):
 d=out/name/mode; d.mkdir(parents=True,exist_ok=True)
 if name.startswith('rotation45'):
  base=bundle/'rotation/periodic2d'
  cfg=(base/'pr_no_limiter_cfl100/run.cfg').read_text()
  shutil.copy2(str(base/'sector.su2'),str(d/'mesh.su2'))
  shutil.copy2(str(base/'inlet.dat'),str(d/'inlet.dat'))
 else:
  base=bundle/'axis/pipe_axis'
  cfg=(base/'pr_w30_converged/run.cfg').read_text()
  shutil.copy2(str(base/'pipe_w30.su2'),str(d/'mesh.su2'))
 changes={'MESH_FILENAME':'mesh.su2','ITER':str(limit),'INNER_ITER':str(limit),
  'CONV_RESIDUAL_MINVAL':'-10','CONV_STARTITER':'0','CONV_FIELD':'RMS_DENSITY',
  'OUTPUT_FILES':'(RESTART_ASCII)','OUTPUT_WRT_FREQ':'999999',
  'HISTORY_OUTPUT':'(ITER,RMS_RES,LINSOL)','SCREEN_WRT_FREQ_INNER':'200'}
 if name=='rotation45-linear50': changes['LINEAR_SOLVER_ITER']='50'
 for key,value in changes.items():
  cfg=re.sub(r'^\s*'+key+r'\s*=.*$',key+'= '+value,cfg,flags=re.M) if re.search(r'^\s*'+key+r'\s*=',cfg,re.M) else cfg+'\n'+key+'= '+value+'\n'
 (d/'run.cfg').write_text(cfg)
 return d
def history(d):
 with (d/'history.csv').open() as f:
  rd=csv.reader(f); keys=[s.strip().strip('"') for s in next(rd)]
  return [{k:float(v) for k,v in zip(keys,row)} for row in rd]
def run(name,mode,limit=4000,ranks=1,exe=binary):
 d=configure(name,mode,limit)
 while True:
  print(subprocess.check_output(['uptime'],text=True).strip(),flush=True)
  print('\n'.join(subprocess.check_output(['ps','-eo','pid,pcpu,comm','--sort=-pcpu'],text=True).splitlines()[:7]),flush=True)
  if float(Path('/proc/loadavg').read_text().split()[0])<=6:break
  time.sleep(10)
 for name in ('history.csv','restart.csv'):
  if (d/name).exists(): (d/name).unlink()
 env=dict(os.environ,OMP_NUM_THREADS='1',TMPDIR=str(r/'compiler_tmp'),SU2_PERIODIC_COUPLING_COMPARE='legacy' if mode.startswith('legacy') else 'fixed')
 cmd=['nice','-n','10','timeout','-k','5','300']+(['mpirun','-np',str(ranks)] if ranks>1 else [])+[str(exe),'run.cfg']
 start=time.monotonic()
 with (d/'run.log').open('w') as f:
  p=subprocess.run(cmd,cwd=str(d),env=env,stdout=f,stderr=subprocess.STDOUT)
 elapsed=time.monotonic()-start
 rows=history(d) if (d/'history.csv').exists() else []
 result={'case':name,'mode':mode,'ranks':ranks,'command':cmd,'environment':{'OMP_NUM_THREADS':'1','SU2_PERIODIC_COUPLING_COMPARE':env['SU2_PERIODIC_COUPLING_COMPARE']},'exit':p.returncode,'wall_seconds':elapsed,'iterations':len(rows),'linear_iterations':sum(v.get('Linear_Solver_Iterations',0) for v in rows),'final':rows[-1] if rows else None,'converged': bool(rows and rows[-1]['rms[Rho]']<=-10)}
 (d/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result),flush=True)
 if p.returncode:print('\n'.join((d/'run.log').read_text().splitlines()[-12:]),flush=True)
 return result
if __name__ == '__main__':
 if sys.argv[1]=='verify':
  run('rotation45','verify-private',30)
  run('rotation45','verify-published',30,exe=r/'build/SU2_CFD/src/SU2_CFD')
  a=history(out/'rotation45/verify-private');b=history(out/'rotation45/verify-published')
  assert a==b, 'Private fixed mode differs from validated binary'
  print('PASS: 30-iteration histories, including linear work, match exactly.')
 elif sys.argv[1]=='resolved':
  results=[run('rotation45-linear50',mode) for mode in ('legacy','fixed')]
  (out/'summary-resolved.json').write_text(json.dumps(results,indent=2)+'\n')
 else:
  results=[]
  for name in ('rotation45','axis30'):
   for mode in ('legacy','fixed'):
    results.append(run(name,mode))
  (out/'summary.json').write_text(json.dumps(results,indent=2)+'\n')
