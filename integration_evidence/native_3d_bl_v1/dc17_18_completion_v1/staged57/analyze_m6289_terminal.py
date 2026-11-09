"""Seal comparable terminal counters and exact raw-payload identity, not mesh acceptance."""
from pathlib import Path
import json,time,hashlib
R=Path(__file__).resolve().parent
A=R/'m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k'
B=R/'m6_runs/euler_build287_exact_face_coverage_mach_only_real_cfd50k'
assert json.loads((A/'terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((A/'status.json').read_text())['pins_verified']
def window(a):
 d=json.loads((a/'construction_progress_assessment.json').read_text());start=[e for e in d['events'] if e['stage']=='quality_entry' and e['round']==25203][-1];end=[e for e in d['events'] if e['stage']=='exit' and e['round']==25210][0]
 assert all(end['complete_flags'].values())
 assert end['totals']['cells']==736323 and all(end['totals'][k]==0 for k in ('q_bad','j_bad','geometry_bad','size_bad'))
 fields=['full_calls','full_seconds','full_work','traversal_work','traversal_seconds','scope_build_work','scope_build_seconds','scope_attempts','scope_builds','facet_build_work','facet_build_seconds','facet_hits','facet_misses','piece_selection_work','piece_selection_seconds','selected_pieces','support_pieces','upper_calls','upper_work','upper_seconds','quality_progress','accepted']
 ranks={}
 for rank in start['rank_rows']:
  before=start['rank_rows'][rank];after=end['rank_rows'][rank];delta={k:float(after[k])-float(before[k]) for k in fields if k in before and k in after}
  delta['facet_visits']=delta.get('facet_hits',0)+delta.get('facet_misses',0);ranks[rank]=delta
 return {'before':{k:start[k] for k in ('stage','round','maximum_native_elapsed','totals','complete_flags')},'exit':{k:end[k] for k in ('stage','round','maximum_native_elapsed','totals','complete_flags')},'per_rank_deltas':ranks,'source_prefixes':d['source_prefixes']}
x,y=window(A),window(B)
raw=[]
for a in (A,B):
 ready=json.loads((a/'capture_ready.json').read_text());assert ready['raw_capture_complete'] and ready['metric_census_complete'];raw.append({(v['rank'],v['role']):v for v in ready['files'].values()})
roles={role:all(raw[0][(rank,role)]['sha256']==raw[1][(rank,role)]['sha256'] for rank in range(6)) for role in ('owned','metrics','reference','rejections')};assert all(roles.values())
log=(A/'run.log').read_bytes();at=log.find(b'Native3D rejected event:');end=log.find(b'\n',at);assert at>=0 and end>at;reason=log[at:end].decode();assert 'storage exhausted: requested 13482375068, ceiling 12884901888' in reason
peak=0
for line in (A/'resources.jsonl').open():peak=max(peak,json.loads(line)['sampled_process_tree_rss_bytes'])
out={'schema':'native_terminal_geometry_storage_diagnosis_v1','time':time.time(),'build289':x,'build287':y,'same_raw_roles_all_six_ranks':roles,'refusal':reason,'log_prefix':{'bytes':end+1,'sha256':hashlib.sha256(log[:end+1]).hexdigest()},'requested_bytes':13482375068,'ceiling_bytes':12884901888,'caller_bytes':5381336204,'scratch_bytes':8101038864,'largest_sampled_process_tree_rss_bytes':peak,'memory_caveat':'Sampled summed RSS, not true physical peak; host had ample available RAM and negligible PSI. This is per-query admission exhaustion, not host OOM.','next_target':'Final certificate memory/proof-work reuse; not construction, shape repair or widened cleanup.','workflow_accepted':False,'global_geometry_certified':False,'scope':'Both final local fields all-known zero failures. Three outer full calls per rank; nested direction/refinement count is not directly emitted. Control flow plus extra facet visits establishes additional reverse-target preparation; do not infer exact direction wall share from overlapping timers.'}
(A/'terminal_certification_diagnosis.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({'raw_identical':roles,'sampled_tree_RSS_GB':peak/1e9,'clean_rank_facet_visit_differences':{r:x['per_rank_deltas'][r]['facet_visits']-y['per_rank_deltas'][r]['facet_visits'] for r in ('0','1','2','3','4')},'refusal':reason}))
