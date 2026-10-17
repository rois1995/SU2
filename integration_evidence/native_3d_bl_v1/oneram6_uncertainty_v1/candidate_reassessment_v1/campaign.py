"""Candidate-partition/original-field reassessment; bounded, sequential, resumable checks."""
import csv,json,math,struct,sys,importlib.util
from pathlib import Path
import numpy as np
BASE=Path(__file__).resolve().parent
sys.path.insert(0,str(BASE.parent))
import run
from support_correction import DOMAIN,SHARED
from verify_support_correction import density,exact_weights
ROOT=run.ROOT
original_pins=run.pins
SOURCES=['Common/include/adaptation/'+n for n in ('CNativeCellSampling3D.hpp','CNativeDonor3D.hpp','CNativeImport3D.hpp','CNativeMesh3D.hpp','CNativeRemesher3D.hpp','CRemesher.hpp','CNativeDistributed3D.hpp')]
SOURCES+=['Common/src/adaptation/'+n for n in ('CNativeDonor3D.cpp','CNativeImport3D.cpp','CNativePredicates3D.cpp','CNativeRemesher3D.cpp')]
SOURCES+=['UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp','UnitTests/SU2_CFD/adaptation/fixtures/native_curved_rans_runtime.inc']
SOURCES+=['integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py']
SOURCES+=[str(Path(__file__).resolve().relative_to(ROOT)),str((BASE.parent/'verify_support_correction.py').relative_to(ROOT)),str((BASE.parent/'support_correction.py').relative_to(ROOT))]
def pins():return dict(original_pins(),**{p:run.sha(ROOT/p) for p in SOURCES})
run.pins=pins
fixture=run.OLD/'cell329/replay.dat'
def save(path,obj):path.write_text(json.dumps(obj,indent=2,allow_nan=False)+'\n')
def inputs(folder,paths):
 folder.mkdir(parents=True,exist_ok=True);path=folder/'inputs.json';identity={str(p.relative_to(ROOT)):run.sha(p) for p in paths}
 if path.exists():assert json.loads(path.read_text())==identity,'Changed inputs: '+str(folder)
 else:save(path,identity)
def execute(name,rank,filters,env=None):
 folder=BASE/name;inputs(folder,[fixture]);run.invoke(folder,env or {},fixture,rank,test_filter=filters);return folder

def controls(rank):
 folder=execute('controls_np%d'%rank,rank,'[NativeDensity3D],[NativeCellSampling3D],[NativeReassessment3D]',{'SU2_NATIVE_CELL_SAMPLER_TRACE':'1'})
 report=json.loads((folder/'original_field_reassessment.json').read_text());assert report['original_source_unchanged'] and report['source_builds']==1
 assert report['fine_indicator']<.7*report['coarse_indicator'];assert abs(report['fine']-report['reference'])<=5*report['fine_indicator']/3
 return report

def records(folder):
 rows=[]
 for path in folder.glob('correction_rank*.csv'):rows.extend(csv.DictReader(path.open()))
 rows.sort(key=lambda r:int(r['row']));assert [int(r['row']) for r in rows]==list(range(8192));return rows

def coarse_m6(rank,prepare=True):
 source=SHARED/'original.bin';manifest=json.loads((SHARED/'manifest.json').read_text());assert run.sha(source)==manifest['original_sha256']
 folder=BASE/('m6_carrier_np%d'%rank);inputs(folder,[fixture,source])
 run.invoke(folder,{'SU2_NATIVE_BL_CELL_CARRIER':str(source),'SU2_NATIVE_BL_CARRIER_SAMPLES':'8192','SU2_NATIVE_BL_PROPOSAL_SEED':'20261017'},fixture,rank)
 query=folder/'native_queries.bin';queries=np.fromfile(query,dtype='<f8',offset=16).reshape(-1,4);assert queries.shape==(8192,4)
 integration=folder/'density';inputs(integration,[fixture,source,query])
 run.invoke(integration,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(source),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(query)},fixture,rank)
 rows=records(integration);assert all(int(row['donor'])!=2**64-1 for row in rows),'A candidate-domain point lost original support.'
 with source.open('rb') as stream:magic,count,core=struct.unpack('<QQd',stream.read(24))
 assert magic==0x4e41545352433344 and source.stat().st_size==24+count*DOMAIN.itemsize
 domain=np.memmap(source,dtype=DOMAIN,offset=24,mode='r',shape=(count,))
 ids=np.array([int(r['donor']) for r in rows]);data=domain[ids];assert np.array_equal(data['id'],ids)
 carrier=list(csv.DictReader((folder/'carrier_queries.csv').open()));assert len(carrier)==8192
 assert np.array_equal(ids,np.array([int(r['cell']) for r in carrier]))
 x=data['coordinates'];matrix=np.swapaxes(x[:,1:]-x[:,0,None],1,2)
 weights3=np.linalg.solve(matrix,(queries[:,:3]-x[:,0])[...,None])[...,0];weights=np.column_stack([1-weights3.sum(axis=1),weights3])
 native=np.array([float(r['sensor']) for r in rows]);oracle=density(np.einsum('nk,nkab->nab',weights,data['sensor']))
 selection=np.unique(np.r_[np.arange(32),np.argsort(np.linalg.cond(matrix))[-32:]])
 exact=np.array([exact_weights(x[i],queries[i,:3]) for i in selection]);precise=density(np.einsum('nk,nkab->nab',exact,data['sensor'][selection]))
 error=float(max(np.max(abs(oracle/native-1)),np.max(abs(precise/native[selection]-1))));assert error<1e-9
 y=np.array([np.longdouble(r['composed']) for r in rows])*queries[:,3].astype(np.longdouble)
 mean=float(y.mean());sem=float(y.std(ddof=1)/np.sqrt(len(y)));block=float(np.array([y[i::8].mean() for i in range(8)]).std(ddof=1)/np.sqrt(8))
 meta=json.loads((folder/'carrier_metadata.json').read_text());indicator=3*max(sem,block)
 nominal=np.array([float(r['importance']) for r in carrier])*queries[:,3]/meta['centroid_mass'];pdf_error=float(np.max(abs(nominal-1)));assert pdf_error<1e-9
 report=dict(value=mean,point_SE=sem,block_SE=block,indicator=indicator,relative_indicator=indicator/mean,
  centroid_estimate=meta['centroid_mass'],samples=len(y),sensor_density_max_relative_error=error,exact_weight_checks=len(selection),
  proposal_pdf_max_relative_error=pdf_error,original_source_sha256=run.sha(source),proven_bound=False,adapted_mesh=False,
  scope='Coarse candidate proposal negative control. Full composed density/inverse-PDF mean; legacy correction-only summary unused.')
 save(integration/'full_field_report.json',report);return report

def traces(rank):
 rows=[]
 for path in (BASE/('controls_np%d'%rank)).glob('cell_sampler_rank*.csv'):rows.extend(csv.DictReader(path.open()))
 return sorted(rows,key=lambda r:int(r['row']))

def main():
 summary={'serial':controls(1),'m6_coarse_np2':coarse_m6(2)}
 if '--pilot' not in sys.argv:
  for rank in (2,4):summary['controls_np%d'%rank]=controls(rank)
  assert traces(1)==traces(2)==traces(4),'Candidate proposal changes with rank count.'
  for rank in (2,4):
   for key in ('coarse','fine','coarse_indicator','fine_indicator'):
    assert math.isclose(summary['serial'][key],summary['controls_np%d'%rank][key],rel_tol=2e-12,abs_tol=1e-12)
  summary['m6_coarse_np4']=coarse_m6(4)
  assert (BASE/'m6_carrier_np2/native_queries.bin').read_bytes()==(BASE/'m6_carrier_np4/native_queries.bin').read_bytes()
  assert records(BASE/'m6_carrier_np2/density')==records(BASE/'m6_carrier_np4/density')
  execute('bridge_np4',4,'[NativeBridge3D],[NativeCurvedRuntime3D]')
  folder=execute('mpi4_m2',4,'[NativeBLPolicyRuntime3D]',{'SU2_NATIVE_POLICY_WORKERS':'2'})
  save(BASE/'mpi4_m2.json',{'status':'PASS','contract':{'selected_workers':2},'artifacts':{p.name:run.sha(p) for p in folder.iterdir() if p.is_file()}})
  spec=importlib.util.spec_from_file_location('policy_audit',ROOT/'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py')
  audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit);audit.HERE=BASE
  previous=sys.argv;sys.argv=['audit.py','4','2']
  try:audit.main()
  finally:sys.argv=previous
  original,_=audit.base.sources(json.loads((folder/'policy_m2_s0.json').read_text()))
  nodes,weights=np.polynomial.legendre.leggauss(48);reference=0.
  for a,b in zip((0.,.05,.27,.3),(.05,.27,.3,1.)):
   for t,w in zip(nodes,weights):
    z=(a+b)/2+(b-a)*t/2
    reference+=(b-a)*w/2*math.sqrt(np.linalg.det(audit.matrix(audit.target(original,(audit.F(0),audit.F(0),audit.F(float(z)))))))
  reference=float(reference)
  mass=json.loads((folder/'reassessment_policy_m2_stage0.json').read_text())
  summary['accepted_planar_RANS_mass']=dict(reference=reference,estimate=mass['value'],indicator=mass['indicator'],absolute_error=abs(mass['value']-reference),indicator_covers_observed_error=abs(mass['value']-reference)<=mass['indicator'],proven_bound=False)
  summary['MPI_cloud_and_field_rows_exact']=True
 save(BASE/('pilot_summary.json' if '--pilot' in sys.argv else 'summary.json'),summary)
 print(json.dumps(summary,indent=2),flush=True)
if __name__=='__main__':main()
