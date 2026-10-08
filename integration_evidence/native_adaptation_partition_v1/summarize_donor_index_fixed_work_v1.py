from pathlib import Path
import csv,json,hashlib,re,statistics
r=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');e=r/'integration_evidence';destination=e/'native_adaptation_partition_v1';destination.mkdir(exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
report=dict(status='PASS',scope='Two MPI4 fixed-work repeats per unchanged frozen sensor/geometric-BL workload on a shared workstation; immutable pre-index and index executables; no full unsteady or rank-repartition performance claim',workloads={})
for fixture in ('rae_euler_to_bl','rae_bl_to_euler'):
 names=[f'native_frozen_{fixture}_'+suffix for suffix in ('pruning_cost_contention_control_v3','donor_index_v1','pruning_cost_contention_repeat_v3','donor_index_repeat_v1')]
 runs=[];meshes=[];work=[]
 audit_case=e/names[1]/'runtime_np4';audit=json.loads((audit_case/'independent_frozen_metric_audit.json').read_text());assert audit['status']=='PASS'
 for name in names:
  case=e/name/'runtime_np4';record=json.loads((case/'run_evidence.json').read_text());assert record['phase']=='terminal' and record['solver_exit']==0
  table=list(csv.DictReader((case/'native_frozen_timing_rank_0.csv').open()));assert len(table)==1 and int(table[0]['accepted'])==1
  text=(case/'solver.log').read_text();assert text.count('All tests passed')==4
  footprint=[]
  for field in ('Native operations (height/split/remove/redistribute/bulk remove/split/flip/move):','Native private target requests/evaluations/dynamic evictions:'):
   matches=[line for line in text.splitlines() if line.startswith(field)];assert len(matches)==1;footprint.append(matches[0])
  work.append(footprint);mesh=case/'native_frozen_adapted.su2';meshes.append(sha(mesh))
  samples=[s for s in record['machine_samples'] if 'elapsed_seconds' in s]
  timings={m[1]:list(map(float,m.group(2,3,4))) for m in re.finditer(r'Native rank cost (.*?) seconds min/mean/max: ([\d.eE+-]+) ([\d.eE+-]+) ([\d.eE+-]+)',text)}
  runs.append(dict(folder=str(case),kernel_seconds=float(table[0]['remesh_max_seconds']),elapsed_seconds=record['elapsed_seconds'],binary_sha256=record['binary_sha256'],inputs_sha256=record['inputs_sha256'],grid_sha256=meshes[-1],evidence_sha256=sha(case/'run_evidence.json'),cpu_busy_range=[min(s['busy_fraction'] for s in samples),max(s['busy_fraction'] for s in samples)],cpu_pressure_avg10_range=[min(s['cpu_pressure_avg10'] for s in samples),max(s['cpu_pressure_avg10'] for s in samples)],rank_costs_min_mean_max=timings))
 assert len(set(meshes))==1 and all(v==work[0] for v in work)
 assert all(v['inputs_sha256']==runs[0]['inputs_sha256'] for v in runs)
 baseline=[runs[i]['kernel_seconds'] for i in (0,2)];indexed=[runs[i]['kernel_seconds'] for i in (1,3)]
 text=(audit_case/'solver.log').read_text();m=re.search(r'Native donor search candidates/full-scan equivalent: (\d+) (\d+); local immutable index retained bytes \(max rank\): (\d+)',text);assert m
 candidate,full,index_bytes=map(int,m.groups())
 report['workloads'][fixture]=dict(runs=runs,baseline_kernel_seconds=baseline,index_kernel_seconds=indexed,mean_kernel_reduction_fraction=1-statistics.mean(indexed)/statistics.mean(baseline),donor_candidates=candidate,full_scan_equivalent=full,candidate_reduction_fraction=1-candidate/full,max_rank_retained_index_bytes=index_bytes,operations_and_target_query_counters_identical=True,all_grids_byte_identical=True,independent_audit=str(audit_case/'independent_frozen_metric_audit.json'),independent_audit_sha256=sha(audit_case/'independent_frozen_metric_audit.json'))
report['limitations']=['Shared host; two repeats per executable/workload, CPU/load/PSI recorded; gains are measured under these conditions, not universal hardware speedups.','Euler-to-BL private reconstruction remains strongly imbalanced.','ADT query scratch bound can require the original allocation-free scan on larger donor partitions or lower dependency ceilings.','Weighted adaptation partitions, M<N execution, final-source unsteady campaigns and practical scaling limits remain pending.']
report['core_validation']=dict(evidence=str(e/'native_donor_index_core_mpi_v1/evidence.json'),sha256=sha(e/'native_donor_index_core_mpi_v1/evidence.json'),cases_per_rank=64,ranks=[1,2,4])
(destination/'donor_index_fixed_work_comparison_v1.json').write_text(json.dumps(report,indent=2)+'\n')
for name,row in report['workloads'].items():print(name,row['baseline_kernel_seconds'],row['index_kernel_seconds'],100*row['mean_kernel_reduction_fraction'],flush=True)
