"""Fixed-budget face-reflection experiment; pair-aware Python assessment, no native normalization."""
import csv,json,math,struct,sys,time
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent.parent))
import run
from support_correction import SHARED,DOMAIN
from correction_support import Proposal
from verify_support_correction import density,exact_weights
from normalize_support_correction import planar_integral
ROOT=run.ROOT;BASE=Path(__file__).resolve().parent;OLD=BASE.parent/'global_uncertainty_calibration_v1';PILOT='--pilot' in sys.argv
old_pins=run.pins
SOURCES=['Common/include/adaptation/CNativeSupport3D.hpp','Common/include/adaptation/CNativeDonor3D.hpp',
 'Common/src/adaptation/CNativeDonor3D.cpp','Common/include/adaptation/CNativeDistributed2D.hpp',
 'UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp','UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp',
 'SU2_CFD/src/adaptation/CNativeComplexity3D.cpp','SU2_CFD/include/adaptation/CNativeComplexity3D.hpp']
def pins():return dict(old_pins(),**{p:run.sha(ROOT/p) for p in SOURCES})
run.pins=pins
COUNT=8192

def save(p,data):p.write_text(json.dumps(data,indent=2,allow_nan=False)+'\n')
def identity(folder,paths):
 folder.mkdir(parents=True,exist_ok=True);p=folder/'inputs.json';value={str(t.relative_to(ROOT)):run.sha(t) for t in paths}
 if p.exists():assert json.loads(p.read_text())==value,'Changed inputs: '+str(folder)
 else:save(p,value)

def prepare(label,fixture,seed,paired=True):
 folder=BASE/label/('prepare' if paired else 'independent_stream_control');identity(folder,[fixture])
 count=COUNT if paired else 32768
 env={'SU2_NATIVE_BL_PREPARE_CORRECTION':str(count),'SU2_NATIVE_BL_PROPOSAL_SEED':str(seed)}
 if paired:env['SU2_NATIVE_BL_PAIR_FACES']='YES'
 run.invoke(folder,env,fixture)
 meta=json.loads((folder/'native_proposal.json').read_text());assert meta['samples']==count and meta['face_pairs']==paired
 assert meta['independent_samples']==(count//2 if paired else count)
 assert meta['preparation_work']+meta['pdf_candidates']<=80000000
 query=folder/'native_queries.bin';data=np.fromfile(query,dtype='<f8',offset=16).reshape(-1,4)
 with query.open('rb') as f:magic,n=struct.unpack('<QQ',f.read(16))
 assert magic==0x4e41545155455259 and n==count and np.all(np.isfinite(data)) and np.all(data[:,3]>0)
 detail=np.atleast_1d(np.genfromtxt(folder/'native_queries.csv',delimiter=',',names=True));assert len(detail)==count
 assert np.array_equal(data[:,:3],np.column_stack([detail[k] for k in ('x','y','z')])) and np.array_equal(data[:,3],detail['inverse_pdf'])
 previous=OLD/label/'prepare/native_queries.bin'
 if not paired:
  assert query.read_bytes()==previous.read_bytes(),'Default Draw changed the saved whole stream.'
  save(folder/'verification.json',{'status':'PASS_DEFAULT_STREAM_BIT_IDENTICAL','previous_path':str(previous.relative_to(ROOT)),
       'previous_sha256':run.sha(previous),'current_sha256':run.sha(query),'points':count});return query,data
 old=np.fromfile(previous,dtype='<f8',offset=16).reshape(-1,4);assert np.array_equal(data[::2],old[:COUNT//2]),'Pair anchors changed.'
 assert np.array_equal(detail['kind'][::2],detail['kind'][1::2])
 for k in (1,2):
  mask=detail['kind'][::2]==k;assert np.array_equal(data[::2][mask],data[1::2][mask]),'Nonface fallback changed its marginal.'
 audit=folder/'pdf_audit.json'
 if audit.exists():
  verified=json.loads(audit.read_text())
  assert verified['status']=='PASS_ALL_PAIRED_PDFS_AND_ANCHORS' and verified['points']==count
  assert verified['query_sha256']==run.sha(query) and verified['previous_anchor_file_sha256']==run.sha(previous)
  assert verified['independent_pdf_helper_sha256']==run.sha(run.HERE/'correction_support.py') and verified['maximum_relative_difference']<1e-8
  print('REUSE_CHECKED_FULL_PDF_AUDIT',label,flush=True);return query,data
 start=time.monotonic();proposal=Proposal(fixture);pdf=proposal.pdf(data[:,:3]);error=float(np.max(abs(pdf*data[:,3]-1)))
 assert error<1e-8
 save(folder/'pdf_audit.json',{'status':'PASS_ALL_PAIRED_PDFS_AND_ANCHORS','points':count,'maximum_relative_difference':error,
  'seconds':time.monotonic()-start,'query_sha256':run.sha(query),'previous_anchor_file_sha256':run.sha(previous),
  'independent_pdf_helper_sha256':run.sha(run.HERE/'correction_support.py'),
  'kind_pairs':{str(k):int(np.count_nonzero(detail['kind'][::2]==k)) for k in (0,1,2)}})
 return query,data

def records(folder,count):
 rows=[]
 for p in folder.glob('correction_rank*.csv'):rows.extend(csv.DictReader(p.open()))
 rows.sort(key=lambda r:int(r['row']));assert [int(r['row']) for r in rows]==list(range(count));return rows

def integrate(label,fixture,source,query,data,ranks):
 folder=BASE/label/('np%d'%ranks);identity(folder,[fixture,source,query])
 run.invoke(folder,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(source),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(query)},fixture,ranks)
 rows=records(folder,COUNT);y=np.array([float(r['weighted_correction']) for r in rows],dtype=np.longdouble)
 pairs=y.reshape(-1,2);averages=pairs.mean(axis=1);mean=float(averages.mean());sem=float(averages.std(ddof=1)/np.sqrt(len(averages)))
 naive=float(y.std(ddof=1)/np.sqrt(COUNT));native=json.loads((folder/'correction_global.json').read_text())
 assert abs(mean/native['correction']-1)<1e-12 and abs(naive/native['empirical_standard_error']-1)<1e-12
 previous=OLD/label/('np1' if label.startswith('planar') else 'np2');old=records(previous,32768)
 drift=0.
 for a,b in zip(rows[::2],old):
  assert a['donor']==b['donor']
  for name in ('sensor','composed','correction','weighted_correction'):
   drift=max(drift,abs(float(a[name])-float(b[name]))/max(1.,abs(float(b[name]))))
 assert drift<1e-11
 with source.open('rb') as f:magic,count,core=struct.unpack('<QQd',f.read(24))
 assert magic==0x4e41545352433344 and source.stat().st_size==24+count*DOMAIN.itemsize
 domain=np.memmap(source,dtype=DOMAIN,offset=24,mode='r',shape=(count,))
 present=np.array([i for i,r in enumerate(rows) if int(r['donor'])!=2**64-1]);ids=np.array([int(rows[i]['donor']) for i in present])
 raw=domain[np.zeros(len(ids),dtype=int) if count==1 else ids];assert np.array_equal(raw['id'],ids)
 x=raw['coordinates'];matrix=np.swapaxes(x[:,1:]-x[:,0,None],1,2)
 beta=np.linalg.solve(matrix,(data[present,:3]-x[:,0])[...,None])[...,0];beta=np.column_stack([1-beta.sum(axis=1),beta])
 actual=np.array([float(rows[i]['sensor']) for i in present]);independent=density(np.einsum('nk,nkab->nab',beta,raw['sensor']))
 error=float(np.max(abs(independent/actual-1)));assert error<1e-9
 condition=np.linalg.cond(matrix);check=np.unique(np.r_[np.arange(min(32,len(ids))),np.argsort(condition)[-32:]])
 exact=np.array([exact_weights(x[i],data[present[i],:3]) for i in check])
 precise=density(np.einsum('nk,nkab->nab',exact,raw['sensor'][check]));exact_error=float(np.max(abs(precise/actual[check]-1)));assert exact_error<1e-9
 old_report=json.loads((previous/'verification.json').read_text());baseline=old_report['prefix8192'];holdout=old_report['held_out24576']
 delta=abs(mean-holdout['mean']);independentSE=math.hypot(sem,holdout['empirical_SE'])
 covariance=float(np.cov(np.asarray(pairs,dtype=float),rowvar=False,ddof=1)[0,1])
 record={'status':'PASS_PAIR_ARITHMETIC_NOT_ACCURACY_CERTIFICATION','native':native,'points':COUNT,'independent_pairs':len(averages),
  'correction':mean,'pair_empirical_SE':sem,'native_pointwise_SE_INVALID_FOR_PAIRS':naive,'pair_covariance':covariance,
  'original_IID8192_SE':baseline['empirical_SE'],'pair_to_saved_IID_SE':sem/baseline['empirical_SE'],
  'disjoint_holdout_correction':holdout['mean'],'holdout_standardized_discrepancy':delta/independentSE,
  'maximum_anchor_scalar_scaled_drift':drift,'all_inside_sensor_relative_error':error,'exact_sensor_relative_error':exact_error,
  'inside_points':len(present),'exact_containment_samples':len(check),'query_sha256':run.sha(query),
  'rows_sha256':{p.name:run.sha(p) for p in folder.glob('correction_rank*.csv')},'original_field_source_sha256':run.sha(source),
  'CFD_steps':0,'production_sampling_changed':False,'native_IID_error_not_used_for_any_acceptance':True,
  'scope':'Pair means are computed from complete owner-routed rows, retaining outside zeros. Empirical SEM is heuristic; no proven tail/accuracy bound.'}
 if label.startswith('planar'):
  background=json.loads((run.HERE/'planar_support_correction_v2/preparation.json').read_text())['sensor_integral'];reference=planar_integral(1.)
  record.update(total_estimate=mean+background,independent_reference=reference,observed_relative_error=abs(mean+background-reference)/reference,
   three_pair_SE_covers_reference=abs(mean+background-reference)<=3*sem)
 else:
  background=(old_report['sensor_lower']+old_report['sensor_upper'])/2;half=old_report['background_interval_halfwidth']
  record.update(total_estimate=mean+background,heuristic_pair_indicator=3*sem+half+old_report['numerical_indicator'])
 if ranks==4:
  old_np2=BASE/label/'np2';proof=json.loads((old_np2/'verification.json').read_text())
  for name,h in proof['rows_sha256'].items():assert run.sha(old_np2/name)==h
  assert records(old_np2,COUNT)==rows,'MPI paired rows differ.'
  assert record['pair_empirical_SE']==proof['pair_empirical_SE'];record['MPI2_4_rows_and_pair_assessment_equal']=True
 save(folder/'verification.json',record);print(label,'MPI',ranks,'mean',mean,'pair_SE',sem,'IID_SE',baseline['empirical_SE'],flush=True)
 return record

fixture=run.OLD/'cell329/replay.dat';source=SHARED/'original.bin'
assert run.sha(source)==json.loads((SHARED/'manifest.json').read_text())['original_sha256']
controls=BASE/'serial_controls';identity(controls,[])
run.invoke(controls,{},fixture,1,test_filter='[NativeSupport3D],[NativeDynamicComplexity3D],[NativeIntegrationPolicy3D],[NativeComposite2D],[NativeComposite3D],[NativeComplexityNormalization],Native3D complexity integrates the geometric BL with sensor-only donors')
reports=[]
for label,seed in [('m6_seed20261017',20261017)]+([] if PILOT else [('m6_seed9919',9919)]):
 if not PILOT:prepare(label,fixture,seed,False)
 query,data=prepare(label,fixture,seed);reports.append(integrate(label,fixture,source,query,data,2))
 if not PILOT and seed==20261017:integrate(label,fixture,source,query,data,4)
if PILOT:
 print('PASS_PAIR_PRIMITIVES_AND_ORIGINAL_M6_PILOT',flush=True);raise SystemExit(0)
planar=run.HERE/'planar_support_correction_v2';query,data=prepare('planar_seed449',planar/'wall.dat',449)
planar_report=integrate('planar_seed449',planar/'wall.dat',planar/'original.bin',query,data,1)
mpi=BASE/'mpi_controls';identity(mpi,[]);run.invoke(mpi,{},fixture,4,test_filter='[NativeDynamicComplexity3D]')
save(BASE/'summary.json',{'status':'COMPLETE_FIXED_POINT_BUDGET_PAIR_EXPERIMENT','production_changed':False,'full_goal_complete':False,
 'source_pins':pins(),'verification_helper_sha256':run.sha(Path(__file__)),'M6':reports,'planar':planar_report,
 'limits':['No native pair-aware normalization: old IID SEM is explicitly invalid here.','No default tolerance or accepted M6 mesh.','Positive covariance can worsen pairing; decide from measurements before adding production policy.']})
print('PASS_CONNECTED_PAIR_EXPERIMENT_NOT_PRODUCTION_POLICY',flush=True)
