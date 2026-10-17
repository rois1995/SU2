#!/usr/bin/env python3
"""Resumable bounded-estimate and original-field subdivision experiments; never run CFD."""
import hashlib,json,os,subprocess,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=ROOT/'integration_evidence/build-integrated-v2'
BINARY=BUILD/'UnitTests/test_driver'
OLD=HERE.parent/'oneram6_quadrature_v1/captured_cells_v1'
FROZEN=HERE.parent/'oneram6_frozen_field_v1'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def pins():
 paths=[Path(__file__).resolve(),BINARY,BUILD/'build.ninja',BUILD/'meson-info/intro-buildoptions.json',
        ROOT/'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',ROOT/'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',
        ROOT/'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp',
        ROOT/'Common/include/adaptation/CComplexityNormalization.hpp',ROOT/'SU2_CFD/src/solvers/CSolver.cpp']
 return {str(p.relative_to(ROOT)):sha(p) for p in paths}
def invoke(folder,env,fixture,ranks=1,test_filter='[NativeFacetedVolumeReplay3D]',expected_failure=False):
 assert ranks in (1,2,4)
 folder.mkdir(parents=True,exist_ok=True)
 identity={'pins':pins(),'fixture':sha(fixture),'parameters':env,'ranks':ranks,'test_filter':test_filter,'expected_failure':expected_failure}
 proof=folder/'provenance.json'
 if proof.exists():assert json.loads(proof.read_text())==identity,'Provenance changed: '+str(folder)
 else:proof.write_text(json.dumps(identity,indent=2)+'\n')
 result=folder/'result.json'
 if result.exists():
  saved=json.loads(result.read_text())
  assert saved['provenance_sha256']==sha(proof) and saved['log_sha256']==sha(folder/'run.log')
  assert (0<saved['exit_code']<124) if expected_failure else saved['exit_code']==0
  if 'normalization_sha256' in saved:assert saved['normalization_sha256']==sha(folder/'normalization.json')
  print('REUSE',folder,flush=True);return saved
 environment=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
                  TMPDIR=str(BUILD/'compile-tmp'),SU2_NATIVE_BL_REPLAY=str(fixture),**env)
 base=[str(BINARY),test_filter,'--abort','--reporter','compact']
 if ranks>1:base=['mpirun','-np',str(ranks),'--bind-to','none']+base
 command=['taskset','-c','0' if ranks==1 else '0-'+str(ranks-1),'nice','-n','19','timeout','--kill-after=5s','180s']+base
 start=time.monotonic()
 with (folder/'run.log').open('w') as log:
  run=subprocess.run(command,cwd=str(folder),env=environment,stdout=log,stderr=subprocess.STDOUT)
 report={'exit_code':run.returncode,'elapsed_seconds':time.monotonic()-start,'command':command,
         'provenance_sha256':sha(proof),'log_sha256':sha(folder/'run.log')}
 if (folder/'complexity_estimate.json').exists():report['estimate']=json.loads((folder/'complexity_estimate.json').read_text())
 if (folder/'normalization.json').exists():
  report['normalization']=json.loads((folder/'normalization.json').read_text());report['normalization_sha256']=sha(folder/'normalization.json')
 (folder/'result.json').write_text(json.dumps(report,indent=2)+'\n')
 print(folder.name,json.dumps(report.get('estimate',report)),flush=True)
 if not ((0<run.returncode<124) if expected_failure else run.returncode==0):raise RuntimeError('Failed '+str(folder))
 return report
def pilot():
 summary=[]
 for cell in (88,157,329):
  refs=HERE.parent/'oneram6_density_reference_v1'/('cell%d'%cell)
  order=72 if cell==157 else 128
  values=[json.loads((refs/('order%d_apex%d'%(order,a))/'result.json').read_text())['integral'] for a in (0,3)]
  ref=float(np.mean(np.array(values,dtype=np.longdouble)))
  for depth in (0,2,4):
   folder=HERE/'pilot'/('cell%d_depth%d'%(cell,depth))
   r=invoke(folder,{'SU2_NATIVE_BL_COMPLEXITY':'8192','SU2_NATIVE_BL_SUBDIVISIONS':str(depth)},OLD/('cell%d'%cell)/'replay.dat')
   e=r['estimate'];e.update(cell=cell,depth=depth,reference=ref,reference_chart_spread=float(abs(np.longdouble(values[0])-np.longdouble(values[1]))),
       actual_error=abs(e['value']-ref),relative_error=abs(e['value']/ref-1),indicator_covers_observed_error=e['indicator'] is not None and e['indicator']>=abs(e['value']-ref))
   summary.append(e)
 (HERE/'pilot_summary.json').write_text(json.dumps(summary,indent=2)+'\n')
def export():
 sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
 from capcheck import read_su2
 proof=json.loads((FROZEN/'result.json').read_text())
 assert sha(FROZEN/'frozen_trial.npz')==proof['sha256']['frozen_trial.npz']
 data=np.load(FROZEN/'frozen_trial.npz',allow_pickle=False);mesh=read_su2(FROZEN/'Coarse.su2');cells=np.sort(mesh.E,axis=1)
 host=data['cell_owner'];lookup=[]
 for rank in range(4):
  a=np.full(len(mesh.P),-1,dtype=np.int64);ids=data['rank%d_global'%rank];a[ids]=np.arange(len(ids));lookup.append(a)
 folder=HERE/'field_inputs';folder.mkdir(exist_ok=True)
 dtype=np.dtype([('id','<u8'),('coordinates','<f8',(4,3)),('sensor','<f8',(4,3,3))])
 records=[]
 for begin in range(0,len(cells),256):
  end=min(len(cells),begin+256);indices=np.arange(begin,end);chunk=cells[begin:end]
  rows=np.empty(len(chunk),dtype=dtype);rows['id']=indices;rows['coordinates']=mesh.P[chunk]
  for rank in range(4):
   selected=host[begin:end]==rank
   if np.any(selected):rows['sensor'][selected]=data['rank%d_sensor'%rank][lookup[rank][chunk[selected]]]
  path=folder/('block%06d.bin'%begin)
  payload=np.array([len(rows)],dtype='<u8').tobytes()+rows.tobytes()
  if path.exists():assert hashlib.sha256(path.read_bytes()).digest()==hashlib.sha256(payload).digest()
  else:path.write_bytes(payload)
  records.append({'begin':begin,'end':end,'path':str(path.relative_to(HERE)),'sha256':sha(path)})
 (folder/'manifest.json').write_text(json.dumps({'count':len(cells),'core':float(data['core']),
    'frozen_sha256':sha(FROZEN/'frozen_trial.npz'),'blocks':records},indent=2)+'\n')
 print('EXPORTED',len(cells),len(records),flush=True)
def field():
 manifest=json.loads((HERE/'field_inputs/manifest.json').read_text())
 start=int(sys.argv[2]) if len(sys.argv)>2 else 0
 stop=int(sys.argv[3]) if len(sys.argv)>3 else len(manifest['blocks'])
 campaign=HERE/'field';campaign.mkdir(exist_ok=True)
 begin_time=time.monotonic()
 for item in manifest['blocks'][start:stop]:
  source=HERE/item['path'];assert sha(source)==item['sha256']
  folder=campaign/('block%06d'%item['begin'])
  result=invoke(folder,{'SU2_NATIVE_BL_COMPLEXITY':'8192','SU2_NATIVE_BL_FIELD':str(source)},OLD/'cell329/replay.dat')
  output=folder/'field_estimates.csv'
  if not output.exists():output=folder/'field_estimates_rank0.csv'
  data=np.atleast_1d(np.genfromtxt(output,delimiter=',',names=True))
  assert len(data)==item['end']-item['begin']
  assert np.array_equal(data['cell'],np.arange(item['begin'],item['end']))
  assert np.all(data['value']>0) and np.all(np.isfinite(data['value']))
  assert np.all(data['evaluations']<=8192) and np.all(data['indicator']>=0)
  summary={'complete':True,'input_sha256':sha(source),'output_sha256':sha(output),'cells':len(data),
   'value':float(np.sum(data['value'],dtype=np.longdouble)),
   'indicator':float(np.sum(data['indicator'],dtype=np.longdouble)) if np.all(np.isfinite(data['indicator'])) else None,
   'evaluations':int(data['evaluations'].sum()),'limited_cells':int(data['limited'].sum()),
   'unresolved_cells':int(data['unresolved_support'].sum()),'maximum_cell_seconds':float(max(data['seconds']))}
  (folder/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
  print('FIELD',item['begin'],json.dumps(summary),flush=True)
  if time.monotonic()-begin_time>7200:
   print('CAMPAIGN_TIME_CEILING reached; completed blocks retained',flush=True);break
if __name__=='__main__':
 {'pilot':pilot,'export':export,'field':field}[sys.argv[1]]()
