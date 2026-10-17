"""Bounded whole-original-field holdout calibration; existing native replay, no CFD or policy change."""
import csv,json,math,struct,sys,time
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent.parent))
import run
from support_correction import DOMAIN,SHARED
from correction_support import Proposal
from verify_support_correction import density,exact_weights
from normalize_support_correction import planar_integral
ROOT=run.ROOT;BASE=Path(__file__).resolve().parent;PILOT='--pilot' in sys.argv
old_pins=run.pins
SOURCES=['Common/include/adaptation/CNativeSupport3D.hpp','Common/include/adaptation/CNativeDonor3D.hpp',
 'Common/src/adaptation/CNativeDonor3D.cpp','Common/include/adaptation/CNativeDistributed2D.hpp',
 'UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp','SU2_CFD/src/adaptation/CNativeComplexity3D.cpp',
 'SU2_CFD/include/adaptation/CNativeComplexity3D.hpp']
def pins():return dict(old_pins(),**{p:run.sha(ROOT/p) for p in SOURCES})
run.pins=pins
COUNT=32768

def save(path,data):path.write_text(json.dumps(data,indent=2,allow_nan=False)+'\n')
def identity(folder,paths):
 folder.mkdir(parents=True,exist_ok=True);p=folder/'inputs.json'
 values={str(t.relative_to(ROOT)):run.sha(t) for t in paths}
 if p.exists():assert json.loads(p.read_text())==values,'Inputs changed: '+str(folder)
 else:save(p,values)

def prepare(label,fixture,seed):
 folder=BASE/label/'prepare';identity(folder,[fixture])
 run.invoke(folder,{'SU2_NATIVE_BL_PREPARE_CORRECTION':str(COUNT),'SU2_NATIVE_BL_PROPOSAL_SEED':str(seed)},fixture)
 data=np.fromfile(folder/'native_queries.bin',dtype='<f8',offset=16).reshape(-1,4)
 with (folder/'native_queries.bin').open('rb') as f:magic,n=struct.unpack('<QQ',f.read(16))
 meta=json.loads((folder/'native_proposal.json').read_text())
 assert magic==0x4e41545155455259 and n==COUNT==meta['samples'] and meta['seed']==seed
 assert np.all(np.isfinite(data)) and np.all(data[:,3]>0)
 assert meta['preparation_work']+meta['pdf_candidates']<=80000000
 # Existing full-stream independent PDF qualifications remain valid; audit a declared subset of the new stream.
 detail=np.atleast_1d(np.genfromtxt(folder/'native_queries.csv',delimiter=',',names=True))
 assert np.array_equal(data[:,:3],np.column_stack([detail[k] for k in ('x','y','z')]))
 assert np.array_equal(data[:,3],detail['inverse_pdf'])
 selected=list(np.linspace(0,COUNT-1,1024,dtype=int))+list(np.argsort(data[:,3])[:32])+list(np.argsort(data[:,3])[-32:])
 for kind in (0,1,2):
  ids=np.flatnonzero(detail['kind']==kind);selected.extend(ids[:32]);selected.extend(ids[-32:])
 selected=np.unique(selected);start=time.monotonic();proposal=Proposal(fixture);pdf=proposal.pdf(data[selected,:3])
 error=float(np.max(abs(pdf*data[selected,3]-1)));assert error<1e-8
 save(folder/'pdf_audit.json',{'status':'PASS_SUBSET_PDF','points':len(selected),'total_points':COUNT,
  'indices':list(map(int,selected)),'maximum_relative_difference':error,'seconds':time.monotonic()-start,
  'kind_counts':{str(k):int(np.count_nonzero(detail['kind']==k)) for k in (0,1,2)},
  'query_sha256':run.sha(folder/'native_queries.bin'),'independent_helper_sha256':run.sha(run.HERE/'correction_support.py'),
  'scope':'KD-tree independent PDF checks on declared regular/extreme/family samples, not every new point or a coverage proof.'})
 return folder/'native_queries.bin',data

def records(folder):
 rows=[]
 for path in folder.glob('correction_rank*.csv'):rows.extend(csv.DictReader(path.open()))
 rows.sort(key=lambda x:int(x['row']));assert [int(x['row']) for x in rows]==list(range(COUNT))
 return rows

def moments(y):
 y=np.asarray(y,dtype=np.longdouble);assert len(y)>1 and np.all(np.isfinite(y))
 mean=y.mean();std=y.std(ddof=1);total=y.sum();top=np.sort(y)[-32:]
 return {'samples':len(y),'nonzero':int(np.count_nonzero(y)), 'mean':float(mean),'empirical_SE':float(std/np.sqrt(len(y))),
  'largest_positive_sample_share':float(max(0,y.max())/total),'top32_positive_share':float(np.maximum(top,0).sum()/total),
  'skewness':float(np.mean((y-mean)**3)/std**3) if std>0 else 0.,'indicator_is_proven_bound':False}

def integrate(label,source,fixture,query,data,ranks):
 folder=BASE/label/('np%d'%ranks);identity(folder,[source,fixture,query])
 run.invoke(folder,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(source),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(query)},fixture,ranks)
 native=json.loads((folder/'correction_global.json').read_text());rows=records(folder)
 assert native['samples']==COUNT and native['complete_original_domain_search'] and native['original_RAW9_preserved']
 weighted=np.array([float(t['weighted_correction']) for t in rows]);all_stats=moments(weighted)
 assert abs(all_stats['mean']/native['correction']-1)<1e-12
 assert abs(all_stats['empirical_SE']/native['empirical_standard_error']-1)<1e-12
 with source.open('rb') as f:magic,count,core=struct.unpack('<QQd',f.read(24))
 assert magic==0x4e41545352433344 and source.stat().st_size==24+count*DOMAIN.itemsize
 domain=np.memmap(source,dtype=DOMAIN,offset=24,mode='r',shape=(count,))
 present=np.array([i for i,t in enumerate(rows) if int(t['donor'])!=2**64-1]);ids=np.array([int(rows[i]['donor']) for i in present])
 raw=domain[np.zeros(len(ids),dtype=int) if count==1 else ids];assert np.array_equal(raw['id'],ids)
 x=raw['coordinates'];edges=np.swapaxes(x[:,1:]-x[:,0,None],1,2)
 beta=np.linalg.solve(edges,(data[present,:3]-x[:,0])[...,None])[...,0];beta=np.column_stack([1-beta.sum(axis=1),beta])
 independent=density(np.einsum('nk,nkab->nab',beta,raw['sensor']));actual=np.array([float(rows[i]['sensor']) for i in present])
 error=float(np.max(abs(independent/actual-1)));assert error<1e-9
 condition=np.linalg.cond(edges);check=np.unique(np.r_[np.arange(min(32,len(ids))),np.argsort(condition)[-32:]])
 exact=np.array([exact_weights(x[i],data[present[i],:3]) for i in check])
 precise=density(np.einsum('nk,nkab->nab',exact,raw['sensor'][check]));exact_error=float(np.max(abs(precise/actual[check]-1)));assert exact_error<1e-9
 blocks=[moments(block) for block in np.split(weighted,4)];prefix=blocks[0];holdout=moments(weighted[8192:])
 discrepancy=abs(prefix['mean']-holdout['mean']);comparison_SE=math.hypot(prefix['empirical_SE'],holdout['empirical_SE'])
 report={'status':'PASS_FROZEN_FIELD_REPLAY_NOT_ERROR_CERTIFICATION','native':native,'full_stream':all_stats,'blocks':blocks,
  'prefix8192':prefix,'held_out24576':holdout,'prefix_holdout_discrepancy':discrepancy,'combined_empirical_SE':comparison_SE,
  'prefix_holdout_standardized_difference':discrepancy/comparison_SE,'inside_source_points':len(ids),
  'all_inside_sensor_relative_error':error,'exact_barycentric_samples':len(check),'exact_sensor_relative_error':exact_error,
  'input_provenance_sha256':run.sha(folder/'inputs.json'),'query_sha256':run.sha(query),'source_sha256':run.sha(source),
  'rows_sha256':{p.name:run.sha(p) for p in folder.glob('correction_rank*.csv')},'CFD_steps':0,'accepted_mesh':False,
  'scope':'Whole original RAW9 P1 with actual geometric composition, including outside-domain zeros. Exact containment on selected inside samples only; no independent outside-domain oracle or full composed-field reference.'}
 if label.startswith('m6'):
  old=json.loads((run.HERE/'production_complexity_geometry_v1/m6_np2/dynamic_complexity.json').read_text())['trials'][0]
  lower,upper,numerical=old['sensor_lower'],old['sensor_upper'],old['numerical_indicator']
  report.update(sensor_lower=lower,sensor_upper=upper,numerical_indicator=numerical,
   total_estimate=all_stats['mean']+(lower+upper)/2,heuristic_total_indicator=3*all_stats['empirical_SE']+(upper-lower)/2+numerical,
   background_interval_halfwidth=(upper-lower)/2)
  if label=='m6_seed20261017':
   assert abs(prefix['mean']/old['correction']-1)<1e-11,'Prefix differs from saved production estimator field/stream.'
   report['prefix_matches_saved_production_estimate']=True
 else:
  prep=json.loads((run.HERE/'planar_support_correction_v2/preparation.json').read_text());ref=planar_integral(1.)
  estimate=all_stats['mean']+prep['sensor_integral'];err=abs(estimate-ref)
  report.update(independent_composed_reference=ref,total_estimate=estimate,observed_relative_error=err/ref,
   observed_absolute_error=err,heuristic_three_SE=3*all_stats['empirical_SE'],three_SE_covers_observed_error=err<=3*all_stats['empirical_SE'])
 previous=BASE/label/'np2/verification.json'
 if ranks==4 and previous.exists():
  prior=json.loads(previous.read_text())
  for name,digest in prior['rows_sha256'].items():assert run.sha(previous.parent/name)==digest
  assert rows==records(previous.parent),'MPI source/query/density rows differ.'
  report['all_MPI2_4_rows_equal']=True
 save(folder/'verification.json',report);print(label,'MPI',ranks,'mean',all_stats['mean'],'SE',all_stats['empirical_SE'],flush=True)
 return report

fixture=run.OLD/'cell329/replay.dat';source=SHARED/'original.bin'
assert run.sha(source)==json.loads((SHARED/'manifest.json').read_text())['original_sha256']
reports=[]
for label,seed in [('m6_seed20261017',20261017)]+([] if PILOT else [('m6_seed9919',9919)]):
 query,data=prepare(label,fixture,seed);reports.append(integrate(label,source,fixture,query,data,2))
 if not PILOT and seed==20261017:integrate(label,source,fixture,query,data,4)
if PILOT:
 print('PASS_FIRST_COMPLETE_FIELD_HOLDOUT_PILOT',flush=True);raise SystemExit(0)
planar=run.HERE/'planar_support_correction_v2';query,data=prepare('planar_seed449',planar/'wall.dat',449)
planar_report=integrate('planar_seed449',planar/'original.bin',planar/'wall.dat',query,data,1)
summary={'status':'PASS_CALIBRATION_OBSERVATIONS_NOT_CALIBRATED_ERROR_BOUND','full_goal_complete':False,'production_changed':False,
 'source_pins':pins(),'helper_sha256':run.sha(Path(__file__)),'M6':reports,'planar':planar_report,
 'limits':['Independent whole-M6 composed reference unavailable.','Empirical SE and finite-seed/block agreement cannot certify rare-tail coverage.',
 'No default uncertainty tolerance, adapted-grid reassessment, correction epoch or accepted M6 mesh from this experiment.']}
save(BASE/'summary.json',summary);print('PASS_COMPLETE_FIELD_CALIBRATION_BATCH',flush=True)
