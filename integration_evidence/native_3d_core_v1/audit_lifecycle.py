"""Strict small actual-driver lifecycle audit. Per-rank phase closure precedes rank aggregation."""
from collections import Counter,defaultdict
from pathlib import Path
import copy,json,math,statistics,sys
from audit_coupled import frame
from audit_geometry_bridge import signature,su2_signature,reject

STAGES={
 'cfd_geometry_initial':('partition','migration','preprocess'),
 'cfd_geometry_return':('partition','migration','preprocess'),
 'metric':('setup','sensors','hessian','construction'),
 'metric_build':('sensor_frames','geometry_rule','complexity_gradation_audit','publication','halo'),
 'replace':('geometry','geometry_checks','solver','transfer','finalize'),
 'cycle':('metric_remesh','replace'),
 'mesh_output':('mesh','reference'),
}
ENGINE=('initialization','seed','selection','dependency','reservation','donor','reconstruction','metric','routing','publication','update','completion')

def parse(line):
 row={k:v if k in ('stage','stop') else float(v) for k,v in (item.split('=') for item in line.split()[1:])}
 assert all(math.isfinite(v) for k,v in row.items() if k not in ('stage','stop'))
 if 'stop' in row:
  assert row['stop'] in ('none','complete','round_budget','work_budget','time_budget','operator_stagnation','failure')
  assert row['global_work'] <= row['work_limit']
  assert bool(row['complete']) == (row['stop']=='complete')
 return row

def closes(row,phases):
 assert row['total']>=0 and row['other']>=-1e-10
 assert all(row[k]>=0 for k in phases)
 assert abs(sum(row[k] for k in phases)+row['other']-row['total'])<=1e-10*max(1,row['total'])

def field_counters(row):
 keys=('requests','hits','misses','invalid_queries','failures','evictions','compositions')
 assert all(k in row for k in keys),'Incomplete native3D field counters'
 assert all(math.isfinite(row[k]) and row[k]>=0 and int(row[k])==row[k] for k in keys)
 assert row['requests']==row['hits']+row['misses']+row['invalid_queries'],'Native3D cache request closure'
 assert row['invalid_queries']<=row['failures']<=row['requests']
 assert row['evictions']<=row['misses']
 assert row['compositions']<=row['requests']-row['invalid_queries']

def transport_counters(row):
 keys=('tracked_bytes','tracked_calls','nested_tracked_collectives')
 scopes=('import','parent','worker')
 parts=tuple(k for scope in scopes for k in ('tracked_'+scope+'_bytes','tracked_'+scope+'_calls','nested_tracked_'+scope))
 elections=('directory_elections','nested_directory_election')
 present=set(row)&set(keys[1:]+parts+elections)
 if not present:return False  # Historical bytes-only logs do not certify current transport accounting.
 assert all(k in row for k in keys),'Incomplete native3D transport counters'
 runtime='refine_accepted' in row or bool(set(row)&set(parts))
 checked=keys+parts if runtime else keys
 assert all(k in row for k in checked),'Incomplete native3D transport scopes'
 assert all(math.isfinite(row[k]) and row[k]>=0 for k in checked)
 assert all(int(row[k])==row[k] for k in checked if k.endswith(('_bytes','_calls')))
 if runtime:
  for suffix,key in (('bytes','tracked_bytes'),('calls','tracked_calls'),('seconds','nested_tracked_collectives')):
   total=sum(row['nested_tracked_'+scope if suffix=='seconds' else 'tracked_'+scope+'_'+suffix] for scope in scopes)
   assert abs(total-row[key]) <= (1e-10*max(1,total) if suffix=='seconds' else 0),'Native3D transport scope closure'
 if set(row)&set(elections):
  assert all(k in row and math.isfinite(row[k]) and row[k]>=0 for k in elections)
  assert int(row[elections[0]])==row[elections[0]]
 return True

def termination_counters(row):
 keys=('final_owned_cells','cached_measured_cells','cached_invalid_cells','remaining_edges','remaining_repairs')
 if not set(row)&set(keys):return False  # Historical records contain no termination-state evidence.
 assert all(k in row for k in keys),'Incomplete native3D termination counters'
 assert all(math.isfinite(row[k]) and row[k]>=0 and int(row[k])==row[k] for k in keys)
 assert row['cached_invalid_cells']<=row['cached_measured_cells']
 if row['complete']:
  assert row['cached_invalid_cells']==0 and row['cached_measured_cells']==row['final_owned_cells']
  assert row['remaining_edges']==0 and row['remaining_repairs']==0
 return True

def coarsening_counters(row):
 keys=('collapse_orientation_checks','collapse_orientation_filtered','collapse_reactivated',
       'changed_star_scans','changed_vertices_sent','changed_vertices_received')
 timers=('nested_change_notifications','nested_collapse_orientation')
 if not set(row)&set(keys+timers):return False  # Historical logs do not certify changed-star accounting.
 assert all(k in row for k in keys+timers),'Incomplete native3D coarsening counters'
 assert all(math.isfinite(row[k]) and row[k]>=0 and int(row[k])==row[k] for k in keys)
 assert row['collapse_orientation_filtered']<=row['collapse_orientation_checks']
 assert row['changed_vertices_sent']<=row['changed_star_scans']
 for timer,parent in zip(timers,('publication','selection')):
  assert math.isfinite(row[timer]) and 0<=row[timer]<=row[parent]+1e-10*max(1,row[parent])
 return True

def nested_stage(row):
 transport_counters(row)
 if row['stage'] in ('refine','coarsen'):closes(row,ENGINE);field_counters(row);termination_counters(row);coarsening_counters(row)
 elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'));field_counters(row)
 elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
 else:assert row['stage']=='source'

def stage(row):
 phases=STAGES[row['stage']];closes(row,phases)
 assert row['total']>0 and row['process_peak_rss']>0
 assert row['cpu']>=0 and row['cpu_other']>=-1e-10
 assert all(row[k+'_cpu']>=0 for k in phases)
 assert abs(sum(row[k+'_cpu'] for k in phases)+row['cpu_other']-row['cpu'])<=1e-10*max(1,row['cpu'])

def main(folder,ranks,log):
 workers=[m for m in (1,2,4) if m<=ranks];records=[];pairs=[]
 for m in workers:
  reference=None
  for detail in (0,1):
   for repeat in range(3):
    stem=f'lifecycle_m{m}_d{detail}_r{repeat}'
    record,data=frame(folder/(stem+'.json'),True);records.append(record)
    expected=signature(data)
    assert su2_signature(folder/(stem+'_mesh_00001.su2'))==expected
    assert (folder/(stem+'_mesh_00001.su2.native_ref')).is_file()
    if reference is None:reference=data
    else:
     assert expected==signature(reference),'Fixed-work repeat/control changed geometry/connectivity'
     assert data['original']==reference['original'],'Fixed-work repeat/control changed computed sensor source'
     pairs.append(stem)
 lines=log.read_text().splitlines()
 rows=[parse(s) for s in lines if s.startswith('NATIVE3D_STAGE ')]
 details=[parse(s) for s in lines if s.startswith('NATIVE3D_DETAIL ')]
 native=[parse(s) for s in lines if s.startswith('NATIVE3D ')]
 cases=[parse(s) for s in lines if s.startswith('LIFECYCLECASE ')]
 expected=3*len(workers)*ranks
 assert Counter(r['stage'] for r in rows)==Counter({k:expected for k in STAGES}), 'Missing/unexpected detailed-stage records'
 assert len(cases)==2*expected and len(native)==2*expected
 grouped=defaultdict(list);nested=defaultdict(list)
 for row in rows:
  assert row['ranks']==ranks and 0<=row['rank']<ranks;stage(row);grouped[row['stage'],int(row['rank'])].append(row)
 for row in details:
  assert row['ranks']==ranks and row['complete']==1
  nested_stage(row)
  nested[row['stage'],int(row['rank'])].append(row)
 assert Counter(r['stage'] for r in details)==Counter({k:expected for k in ('import','weights','return','source','partition','refine','coarsen')})
 for rank in range(ranks):
  for index in range(3*len(workers)):
   get=lambda label:grouped[label,rank][index]
   assert get('metric_build')['total']<=get('metric')['construction']+1e-10
   assert get('cfd_geometry_return')['total']<=get('replace')['geometry']+1e-10
   assert get('replace')['total']<=get('cycle')['replace']+1e-10
   active=[r for r in native if r['rank']==rank and r['detailed']==1][index]
   assert get('metric')['total']+active['total']<=get('cycle')['metric_remesh']+1e-10
 for row in native:transport_counters(row)
 summaries=[]
 for m in workers:
  totals={}
  for detail in (0,1):
   samples=[]
   for repeat in range(3):
    block=[r for r in cases if r['workers']==m and r['detailed']==detail and r['repeat']==repeat]
    assert len(block)==ranks and {r['rank'] for r in block}==set(range(ranks))
    assert all(r['wall']>0 and r['cpu']>=0 for r in block)
    samples.append(max(r['wall'] for r in block))
   totals[detail]=samples
  summaries.append(dict(workers=m,off_seconds=totals[0],on_seconds=totals[1],off_median=statistics.median(totals[0]),on_median=statistics.median(totals[1]),on_off_median_ratio=statistics.median(totals[1])/statistics.median(totals[0])))
 # The verifier must detect corrupted closure and invalid cache arithmetic.
 bad=copy.deepcopy(rows[0]);bad[STAGES[bad['stage']][0]]+=1;reject(lambda:stage(bad))
 bad=copy.deepcopy(rows[0]);bad['cpu']+=1;reject(lambda:stage(bad))
 bad=copy.deepcopy(rows[0]);bad['total']=float('nan');reject(lambda:stage(bad))
 for key in ('misses','invalid_queries','failures','evictions','compositions'):
  bad=copy.deepcopy(next(r for r in details if r['stage']=='weights' and r['requests']>0))
  bad[key]=bad['requests']+1;reject(lambda:field_counters(bad))
 result=dict(status='PASS',field_counter_adversaries_rejected=5,frames=records,fixed_work_controls=pairs,rank_stages=rows,nested_rank_details=details,outer_case_rows=cases,fixed_work_timings=summaries,adversaries_rejected=3,
  scope='Actual metric/Hessian construction, weighted remesh and CFD repartition/rebuild/BDF transfer/output on small forced-size faceted cubes. Closed per-rank exclusive wall/CPU scopes and nested query/partition/engine counters; process-lifetime RSS. Three fresh same-work repeats per N/M/OFFON. Outer testcase timers include its computed-metric assertions/observation; cycle excludes requested output, which has a separate stage. Each stage excludes its own diagnostic emission while parents include nested emissions. No CFD iterations, private CPU inference from elapsed-minus-CPU, complete MPI wait/traffic or allocator/reader/ParMETIS memory certificate, practical scaling or affordability claim.')
 (folder/'lifecycle_audit.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS',len(records),'strict lifecycle frames;',len(rows),'closed rank-stage records;',len(details),'nested rank records')

if __name__=='__main__':main(Path(sys.argv[1]).resolve(),int(sys.argv[2]),Path(sys.argv[3]).resolve())
