#!/usr/bin/env python3
"""Derive scoped query reuse results from preserved complete or atomically rejected runs."""
import importlib.util,json,math,re
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.with_name('sphere_rans_lifecycle_v1')
spec=importlib.util.spec_from_file_location('previous_query_report',HERE.with_name('sphere_private_queries_v1')/'report.py')
previous=importlib.util.module_from_spec(spec);spec.loader.exec_module(previous)
def read(path):return json.loads(path.read_text())
def event(name):
 folder=BASE/name/'update';result=read(folder/'result.json');log=re.sub(r'\x1b\[[0-9;]*m','',(folder/'run.log').read_text())
 statuses=[read(folder/('status_rank%d.json'%rank)) for rank in range(4)]
 assert result['exit_code']==1 and all(not s['complete'] and s['unchanged_input'] for s in statuses)
 assert log.count('Failed 1 test case, failed 1 assertion.')==4,'Unexpected state or transfer failure'
 ranks=[previous.tokens(line) for line in log.splitlines() if line.startswith('NATIVE3D rank=')]
 assert sorted(row['rank'] for row in ranks)==[0,1,2,3]
 stages=[previous.tokens(line) for line in log.splitlines() if line.startswith('NATIVE3D_DETAIL stage=') and ' rank=0 ' in line]
 coarsen=next(row for row in stages if row['stage']=='coarsen')
 if 'metric_support_cells' in coarsen:
  assert sum(coarsen[k] for k in ['metric_support_edges','metric_support_skipped','metric_support_duplicates'])==6*coarsen['metric_support_cells']
 return {'folder':str(folder.relative_to(ROOT)),'reason':next(line for line in log.splitlines() if line.startswith('Native3D rejected event:')),
  'status':statuses,'launcher_seconds':result['seconds'],'rank0_coarsen':coarsen,'rank0_details':stages,'per_rank_native':ranks,
  'global_private_edits':sum(row['coarsen_accepted'] for row in ranks)}
def main():
 before=event('n4_m2_query_v4');after=event('n4_m2_reuse_v1')
 a=read(BASE/'n4_m2_query_v4/update/provenance.json');b=read(BASE/'n4_m2_reuse_v1/update/provenance.json')
 for key in ['stage','ranks','workers','checkpoint_producer']:assert a[key]==b[key],key
 assert {Path(k).name:v for k,v in a['inputs'].items()}=={Path(k).name:v for k,v in b['inputs'].items()}
 assert before['global_private_edits']==after['global_private_edits']==344
 assert after['status'][0]['work']==6144 and 'global proposal budget exhausted' in after['reason']
 controls={mode:read(HERE/'controls'/mode/'result.json') for mode in ['serial','mpi2','mpi4','rans_mpi4']};assert all(r['exit_code']==0 for r in controls.values())
 strict=read(HERE/'strict_independent_adapter/independent_mpi4_m2.json');assert strict['status']=='PASS_LIMITED_SCOPE' and len(strict['frames'])==2
 enabled=read(HERE/'controls/rans_mpi4/independent_enabled.json');assert {r['budget'] for r in enabled}=={3,20}
 assert all(r['accepted'] and r['frozen'] and r['SU2_and_CGNS_reload'] and r['SA_restart_fields'] and r['flow_BDF_integral_defect']<1e-10 and r['transported_sensor_residual']<2e-12 for r in enabled)
 old=before['rank0_coarsen'];new=after['rank0_coarsen']
 out={'status':'PARTIAL_QUERY_REUSE_QUALIFIED_ACTUAL_REPEAT_WORK_REJECTED','goal':'ACTIVE_INCOMPLETE','before':before,'after':after,
  'same_inputs_config_workers_caps':True,'controls':controls,'strict_and_opt_in_independent':True,
  'observed_rank0_donor_seconds_ratio':new['donor']/old['donor'],'rank0_donor_candidates_ratio':new['donor_candidates']/old['donor_candidates'],
  'scope':'Different work ceilings reached: 5499/time versus6144/work. These are rejected events, not accepted-event speedup or affordability. Per-rank tracked collectives may count the same communication and must not be summed as unique network traffic. Native process RSS and GNUtime launcher RSS do not establish aggregate job peak. No full campaign donor-index cache, enabled curved/interacting policy lifecycle,2D policy, physical accuracy or global absolute-error allocator qualification.'}
 (HERE/'summary.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'],flush=True)
if __name__=='__main__':main()
