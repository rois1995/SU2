"""Associate qualified per-rank stages with fixed-work cases; never sum rank maxima."""
from collections import defaultdict
from pathlib import Path
import csv,json,statistics,sys

def main(folder):
 samples=[];summaries=[];overheads=[]
 for n in (1,2,4):
  data=json.loads((folder/f'lifecycle_mpi{n}/lifecycle_audit.json').read_text());assert data['status']=='PASS'
  cases=defaultdict(list);stages=defaultdict(list);detail=defaultdict(list)
  for r in data['outer_case_rows']:
   if r['detailed']==1:cases[int(r['rank'])].append(r)
  for r in data['rank_stages']:stages[int(r['rank']),r['stage']].append(r)
  for r in data['nested_rank_details']:detail[int(r['rank']),r['stage']].append(r)
  for (rank,label),rows in stages.items():
   assert len(rows)==len(cases[rank])
   for i,row in enumerate(rows):
    meta=cases[rank][i];copy=dict(row,workers=int(meta['workers']),repeat=int(meta['repeat']));samples.append(copy)
  for m in sorted({int(r['workers']) for r in cases[0]}):
   for repeat in range(3):
    indices={r:next(i for i,c in enumerate(rows) if c['workers']==m and c['repeat']==repeat) for r,rows in cases.items()}
    labels=sorted({label for r,label in stages})
    totals={label:max(stages[r,label][i]['total'] for r,i in indices.items()) for label in labels}
    # The parent case maximum is the envelope; the separate output maximum is diagnostic, not added to it.
    parent=max(cases[r][i]['wall'] for r,i in indices.items());accepted=0
    for r,i in indices.items():
     assert detail[r,'refine'][i]['workers']==m
     accepted+=detail[r,'refine'][i]['accepted']+detail[r,'coarsen'][i]['accepted']
    summaries.append(dict(ranks=n,workers=m,repeat=repeat,parent_case_seconds=parent,accepted_edits=int(accepted),accepted_edits_per_parent_second=accepted/parent,nested_stage_maxima_seconds=totals,
      stage_max_over_mean={label:totals[label]/statistics.mean(stages[r,label][i]['total'] for r,i in indices.items()) for label in labels}))
  overheads.extend(dict(ranks=n,**r) for r in data['fixed_work_timings'])
 keys=['ranks','workers','repeat','rank','stage','time_iter','total','other','cpu','cpu_other','process_peak_rss']
 keys+=sorted({k for row in samples for k in row}-set(keys))
 with (folder/'lifecycle_rank_phases.csv').open('w',newline='') as f:
  out=csv.DictWriter(f,fieldnames=keys);out.writeheader();out.writerows(samples)
 result=dict(status='PASS',rank_stage_count=len(samples),cases=summaries,fixed_work_off_on=overheads,
  scope='Per-rank exclusive stages and closure, then max/mean diagnostics. Nested stage maxima are not additive; parent_case_seconds includes test metric assertions/observation and nested reporting, excludes requested mesh output. Accepted edits/s is a small fixture diagnostic, not CFD affordability or matching work across different N/M trajectories. Contended host; three fresh fixed-work repeats per N/M/detail.')
 (folder/'lifecycle_phase_summary.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS',len(samples),'rank-stage CSV rows;',len(summaries),'ON case summaries')
if __name__=='__main__':main(Path(sys.argv[1]).resolve())
