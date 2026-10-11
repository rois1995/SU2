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
 row={k:v if k=='stage' else float(v) for k,v in (item.split('=') for item in line.split()[1:])}
 assert all(math.isfinite(v) for k,v in row.items() if k!='stage')
 return row

def closes(row,phases):
 assert row['total']>=0 and row['other']>=-1e-10
 assert all(row[k]>=0 for k in phases)
 assert abs(sum(row[k] for k in phases)+row['other']-row['total'])<=1e-10*max(1,row['total'])

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
  if row['stage'] in ('refine','coarsen'):closes(row,ENGINE);assert row['hits']<=row['requests']
  elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'));assert row['hits']<=row['requests']
  elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
  else:assert row['stage']=='source'
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
 result=dict(status='PASS',frames=records,fixed_work_controls=pairs,rank_stages=rows,nested_rank_details=details,outer_case_rows=cases,fixed_work_timings=summaries,adversaries_rejected=3,
  scope='Actual metric/Hessian construction, weighted remesh and CFD repartition/rebuild/BDF transfer/output on small forced-size faceted cubes. Closed per-rank exclusive wall/CPU scopes and nested query/partition/engine counters; process-lifetime RSS. Three fresh same-work repeats per N/M/OFFON. Outer testcase timers include its computed-metric assertions/observation; cycle excludes requested output, which has a separate stage. Each stage excludes its own diagnostic emission while parents include nested emissions. No CFD iterations, private CPU inference from elapsed-minus-CPU, complete MPI wait/traffic or allocator/reader/ParMETIS memory certificate, practical scaling or affordability claim.')
 (folder/'lifecycle_audit.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS',len(records),'strict lifecycle frames;',len(rows),'closed rank-stage records;',len(details),'nested rank records')

if __name__=='__main__':main(Path(sys.argv[1]).resolve(),int(sys.argv[2]),Path(sys.argv[3]).resolve())
