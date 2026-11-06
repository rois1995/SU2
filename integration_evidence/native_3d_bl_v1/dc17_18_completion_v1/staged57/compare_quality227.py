import json,time,hashlib
from pathlib import Path
root=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs')
records={}
for label,name in [('build225','euler_build225_entity_shell_real_cfd50k'),('build227','euler_build227_witness_real_cfd50k')]:
 a=root/name;progress=json.loads((a/'construction_progress_assessment.json').read_text());entry=next(e for e in progress['events'] if e['stage']=='quality_entry');service={}
 for rank in range(6):
  path=a/f'rank_output/1/rank.{rank}/stdout';receipt=progress['source_prefixes'][str(path)];raw=path.read_bytes()[:receipt['bytes']];assert hashlib.sha256(raw).hexdigest()==receipt['sha256']
  for line in raw.decode(errors='replace').splitlines():
   if not line.startswith('NATIVE3D_QUALITY '):continue
   row=dict(item.split('=',1) for item in line.split()[1:] if '=' in item)
   if row.get('stage')!='window':continue
   assert int(row['rank'])==rank and row['complete']=='1';key=(int(row['round']),rank);assert key not in service;service[key]=row
 windows={}
 for window in progress['events']:
  if window['stage']!='window' or window['phase']!='improvement':continue
  round_=window['round']
  if any((round_,rank) not in service for rank in range(6)):continue
  elapsed=window['maximum_native_elapsed']-entry['maximum_native_elapsed'];assert elapsed>0
  cumulative=[v for (rnd,rank),v in service.items() if rnd<=round_]
  sums={k:sum(float(v[k]) for v in cumulative) for k in ['commits','q_retired','q_introduced','j_retired','j_introduced','busy_seconds','join_call_seconds','tracked_collective_call_seconds']}
  removed={k:entry['totals'][k]-window['totals'][k] for k in ['q_bad','j_bad','geometry_bad']}
  assert sums['q_retired']-sums['q_introduced']==removed['q_bad'],(label,round_,'q churn')
  assert sums['j_retired']-sums['j_introduced']==removed['j_bad'],(label,round_,'J churn')
  work={k:sum(float(window['rank_rows'][str(rank)][k])-float(entry['rank_rows'][str(rank)][k]) for rank in range(6)) for k in ['upper_calls','upper_work','traversal_work','scope_build_work','facet_build_work','piece_selection_work','smooth_boundary_candidates','smooth_boundary_moves','smooth_boundary_seconds','smooth_boundary_debt_reduction','smooth_interior_candidates','smooth_interior_moves','smooth_interior_seconds','smooth_interior_debt_reduction']}
  debt=[sum(float(window['rank_rows'][str(rank)][k]) for k in ['q_debt','j_debt']) for rank in range(6)]
  windows[round_]={'quality_seconds':elapsed,'maximum_native_seconds':window['maximum_native_elapsed'],'totals':window['totals'],'complete_flags':window['complete_flags'],'net_bad_removed':removed,'net_bad_removed_per_quality_second':{k:v/elapsed for k,v in removed.items()},'cumulative_service':sums,'rank_window_service':{str(rank):service[(round_,rank)] for rank in range(6)},'quality_work':work,'q_removed_per_commit':removed['q_bad']/sums['commits'] if sums['commits'] else None,'join_call_wall_share':sums['join_call_seconds']/(6*elapsed),'rank_q_bad':[int(window['rank_rows'][str(rank)]['q_bad']) for rank in range(6)],'rank_q_J_debt':debt,'debt_max_over_mean':max(debt)/(sum(debt)/6) if sum(debt)>0 else 0}
 records[label]={'entry':entry['totals'],'windows':windows,'source_prefixes':progress['source_prefixes']}
matching=[];closest=[]
old,new=records['build225']['windows'],records['build227']['windows']
for rnd,b in new.items():
 if rnd in old:
  a=old[rnd]
  matching.append({'round':rnd,'counts_equal':a['totals']==b['totals'],'flags_equal':a['complete_flags']==b['complete_flags'],'build225':a,'build227':b,'net_q_rate_ratio227_over225':b['net_bad_removed_per_quality_second']['q_bad']/a['net_bad_removed_per_quality_second']['q_bad'] if a['net_bad_removed_per_quality_second']['q_bad'] else None,'traversal_work_ratio227_over225':b['quality_work']['traversal_work']/a['quality_work']['traversal_work'] if a['quality_work']['traversal_work'] else None})
 prior_round=min(old,key=lambda k:abs(old[k]['quality_seconds']-b['quality_seconds']));a=old[prior_round]
 closest.append({'round227':rnd,'round225':prior_round,'quality_seconds227':b['quality_seconds'],'quality_seconds225':a['quality_seconds'],'quality_time_gap_seconds227_minus225':b['quality_seconds']-a['quality_seconds'],'bad_counts227':{k:b['totals'][k] for k in ['q_bad','j_bad','geometry_bad']},'bad_counts225':{k:a['totals'][k] for k in ['q_bad','j_bad','geometry_bad']},'note':'Nearest measured COMPLETE census; times differ as stated, no interpolated counts or same-time causal claim.'})
assert matching
out={'time':time.time(),'analysis_helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'scope':'Existing complete first-event censuses only. Quality time normalized from each entry. Work counters/timers overlap; join-call wall share is not CPU idle. Net whole-field counts, not private debt, judge useful progress.','matching_rounds':matching,'nearest_measured_quality_times':closest,'source_prefixes':{k:v['source_prefixes'] for k,v in records.items()},'workflow_acceptance_assessed':False}
(root/'euler_build227_witness_real_cfd50k/quality_comparison225.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps([{'round':x['round'],'counts_equal':x['counts_equal'],'q_rate225':x['build225']['net_bad_removed_per_quality_second']['q_bad'],'q_rate227':x['build227']['net_bad_removed_per_quality_second']['q_bad'],'q_rate_ratio':x['net_q_rate_ratio227_over225'],'traversal_ratio':x['traversal_work_ratio227_over225'],'q_bad227':x['build227']['totals']['q_bad'],'J_bad227':x['build227']['totals']['j_bad'],'geometry_bad227':x['build227']['totals']['geometry_bad']} for x in matching[-3:]],indent=2))
