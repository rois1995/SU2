#!/usr/bin/env python3
"""Account for the accepted retry-lifecycle workflow and retain its earlier failures."""
import importlib.util,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.with_name('sphere_rans_lifecycle_v1')
spec=importlib.util.spec_from_file_location('query_outcomes',HERE.with_name('sphere_query_reuse_v1')/'report.py');query=importlib.util.module_from_spec(spec);spec.loader.exec_module(query)
def read(path):return json.loads(path.read_text())
def main():
 previous=query.event('n4_m2_reuse_v1');folder=BASE/'n4_m2_dependencies_v1/update'
 receipt=read(folder/'result.json');proof=read(folder/'provenance.json');assert receipt['exit_code']==0
 prior=read(BASE/'n4_m2_reuse_v1/update/provenance.json')
 for k in ['stage','ranks','workers','checkpoint_producer']:assert proof[k]==prior[k],k
 assert {Path(k).name:v for k,v in proof['inputs'].items()}=={Path(k).name:v for k,v in prior['inputs'].items()}
 statuses=[read(folder/('status_rank%d.json'%rank)) for rank in range(4)]
 assert all(s['complete'] and s['unchanged_input'] and s['work']==4511 for s in statuses)
 log=re.sub(r'\x1b\[[0-9;]*m','',(folder/'run.log').read_text())
 ranks=[query.previous.tokens(l) for l in log.splitlines() if l.startswith('NATIVE3D rank=')]
 assert sorted(r['rank'] for r in ranks)==[0,1,2,3] and all(r['complete']==1 for r in ranks)
 stages=[query.previous.tokens(l) for l in log.splitlines() if l.startswith('NATIVE3D_DETAIL stage=') and ' rank=0 ' in l]
 coarsen=next(r for r in stages if r['stage']=='coarsen')
 assert coarsen['blocked_queue_skips']>0 and coarsen['blocked_unmeasured']>0
 assert coarsen['metric_support_edges']+coarsen['metric_support_skipped']+coarsen['metric_support_duplicates']==6*coarsen['metric_support_cells']
 edits=sum(r['coarsen_accepted'] for r in ranks);assert edits==previous['global_private_edits']==344
 audit=read(folder/'independent.json');assert audit['status']=='PASS_ACTUAL_RANS_STATE_TRANSFER_RESTART_AND_OUTPUT' and audit['cells']==3768
 assert audit['maximum_actual_integral_defect']<2e-10 and audit['transported_sensor_error']<2e-14
 controls={mode:read(HERE/'controls_v3'/mode/'result.json') for mode in ['serial','mpi2','mpi4','rans_mpi4']};assert all(r['exit_code']==0 for r in controls.values())
 failures={name:read(HERE/name/'serial/result.json') for name in ['controls','controls_v2']};assert all(r['exit_code']==1 for r in failures.values())
 strict=read(HERE/'strict_independent_adapter/independent_mpi4_m2.json');assert strict['status']=='PASS_LIMITED_SCOPE' and len(strict['frames'])==2
 enabled=read(HERE/'controls_v3/rans_mpi4/independent_enabled.json');assert {r['budget'] for r in enabled}=={3,20}
 assert all(r['accepted'] and r['frozen'] and r['SU2_and_CGNS_reload'] and r['SA_restart_fields'] and r['flow_BDF_integral_defect']<1e-10 and r['transported_sensor_residual']<2e-12 for r in enabled)
 out={'status':'ACCEPTED_ACTUAL_REPEATED_BL_WORKFLOW_RETRY_LIFECYCLE_QUALIFIED','goal':'ACTIVE_INCOMPLETE','previous_rejection':previous,
  'current':{'folder':str(folder.relative_to(ROOT)),'status':statuses,'launcher_seconds':receipt['seconds'],'per_rank_native':ranks,'rank0_details':stages,'global_private_edits':edits,'independent':audit},
  'same_inputs_config_workers_caps':True,'current_controls':controls,'preserved_failed_controls':failures,'strict_and_opt_in_independent':True,
  'clock_correction':'Support imports nested inside seed/update are now charged once to donor and excluded from the reported parent. Earlier donor work observations are inclusive; earlier phase sums double-counted those imports.',
  'scope':'Accepted initialized N4/M2 CGNS/current+BDF/SA continuation of historical cold producer, prescribed identity original sensors and strict spherical BL; no CFD advance. Fresh independent exact geometry/height/state audit; spherical composed shape/edges retain native scope. Same344edits, less repeated search, complete survivors. Different rejected work prevents a clean same-work kernel or affordability/scaling comparison. No general reconstruction-failure memoization, enabled curved/interacting budget policy,2D policy or calibrated global-error allocator qualification.'}
 (HERE/'summary.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'],flush=True)
if __name__=='__main__':main()
