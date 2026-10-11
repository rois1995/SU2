"""Classify actual n8 attempted stars without treating diagnostic rejection as adaptation success."""
from collections import Counter
import hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
folder=Path(__file__).resolve().parent;base=folder.parent;root=base.parents[1]
sys.path.insert(0,str(base))
from audit_lifecycle import parse,nested_stage,transport_counters,termination_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((folder/'validation.json').read_text())
assert receipt['status']=='PASS' and receipt['adaptation_status']=='FAIL'
assert not receipt['source_drift'] and not receipt['object_drift'] and not receipt['library_drift']
assert all(sha(folder/n)==h for n,h in receipt['artifact_sha256'].items())
baseline=base/'transport_limit_n8_n1_m1_v2'
for n in ('parameters.json','case_environment.json','envelope.cfg','envelope_input.su2','envelope_s0_initial.json'):
 assert (folder/n).read_bytes()==(baseline/n).read_bytes(),n

def counters(where):
 rows=[]
 for line in (where/'case.stdout').read_text().splitlines():
  if line.startswith(('NATIVE3D ','NATIVE3D_DETAIL ')):rows.append((line.split()[0],parse(line)))
 return rows
old=counters(baseline);new=counters(folder);assert len(old)==len(new)==8
work_keys=set('rank ranks workers complete detailed initial final refine_accepted coarsen_accepted tracked_bytes tracked_calls tracked_import_bytes tracked_import_calls tracked_parent_bytes tracked_parent_calls tracked_worker_bytes tracked_worker_calls native_bound requests hits misses invalid_queries failures evictions compositions batches retries candidates payloads maximum_patch moved arcs cells_before cells_after work_before work_after directory_elections rounds attempted accepted rejected conflicts edge_evaluations collapse_geometry_checks collapse_filtered edge_reused reconnected inserted maximum_donor_patch maximum_owned donor_reused edge_separated donor_batches donor_candidates donor_filtered donor_payloads batch_retries query_retries private_query_retries final_owned_cells cached_measured_cells cached_invalid_cells remaining_edges remaining_repairs'.split())
equal=[]
for (ot,orr),(nt,nr) in zip(old,new):
 assert ot==nt and orr.get('stage')==nr.get('stage')
 keys=sorted(set(orr)&set(nr)&work_keys)
 for k in keys:assert orr[k]==nr[k] and int(orr[k])==orr[k],(nr.get('stage'),k)
 equal.extend((nr.get('stage','parent'),k) for k in keys)
 if nt=='NATIVE3D_DETAIL':nested_stage(nr)
 if nr.get('stage') in ('refine','coarsen'):assert termination_counters(nr)
 if nt=='NATIVE3D':assert transport_counters(nr)
assert len(equal)>=160
out=(folder/'case.stdout').read_text()
assert 'assertions: 5039 | 5038 passed | 1 failed' in out
assert 'REQUIRE( result.status==CRemeshResult::Status::COMPLETE )' in out
assert not (folder/'independent_audit.json').exists()
events=[json.loads(line) for line in (folder/'attempts_rank0.jsonl').read_text().splitlines()]
assert len(events)==512 and sum(e['approved'] for e in events)==272 and not any(e['repairing'] for e in events)
assert [e['round'] for e in events]==list(range(512))

def fingerprint(rows):return tuple(sorted((r['id'],r['version'],tuple(tuple(n) for n in r['nodes']),tuple(tuple(f) for f in r['physical'])) for r in rows))
def touched(rows,ids):return any(set(ids).issubset(n[0] for n in r['nodes']) for r in rows)
blocked={};last_rejected={};duplicates=[];misses=[];reactivated=[];changed_rejections=[];reasons=Counter()
for event in events:
 edge=tuple(event['edge']);support=fingerprint(event['old']);previous=last_rejected.get(edge)
 assert touched(event['old'],edge)
 assert all(any(n[0]==edge[1] for n in row['nodes']) for row in event['old'])
 blocked.pop(edge,None)
 if event['approved']:
  for candidate in list(blocked):
   vertices=[*event['old'],*event['fresh']]
   if touched(vertices,candidate):
    reactivated.append(dict(round=event['round'],edge=candidate,blocked_round=blocked[candidate]['round']))
    del blocked[candidate]
   elif touched(vertices,[candidate[1]]):
    misses.append(dict(round=event['round'],edge=candidate,blocked_round=blocked[candidate]['round'],changed_cells=[r['id'] for r in event['old'] if touched([r],[candidate[1]])],new_cells=[r['id'] for r in event['fresh'] if touched([r],[candidate[1]])]))
 else:
  reasons[event['reason'].split(':',1)[0]]+=1
  if previous is not None:
   (duplicates if previous['support']==support else changed_rejections).append(dict(round=event['round'],edge=edge,previous_round=previous['round']))
  last_rejected[edge]=dict(support=support,round=event['round'])
  blocked[edge]=dict(support=support,round=event['round'])
private=[json.loads(s) for s in (folder/'private_mesh_status.jsonl').read_text().splitlines()]
assert [(r['cells'],r['valid_cached_exit']) for r in private]==[(3552,True),(2489,False)]
assert (folder/'coarsen_rejected.su2').is_file() and (folder/'refine_private.su2').is_file()
result=dict(status='PASS',adaptation_status='FAIL',attempts=len(events),accepted=272,rejected=240,unique_rejected_directions=len(last_rejected),identical_full_star_repeat_rejections=duplicates,changed_full_star_repeat_rejections=changed_rejections,missed_full_star_reactivation_events=misses,missed_directions=len({tuple(r['edge']) for r in misses}),edge_touching_reactivation_events=len(reactivated),reason_counts=dict(reasons),identical_integer_work_counters=len(equal),private_meshes=private,receipt_sha256=sha(folder/'validation.json'),source_sha256={'analyze.py':sha(Path(__file__)),'../audit_lifecycle.py':sha(base/'audit_lifecycle.py')},artifact_sha256={n:sha(folder/n) for n in ('attempts_rank0.jsonl','private_mesh_status.jsonl','coarsen_rejected.su2','refine_private.su2','case.stdout')},scope='Diagnostic-source replay of the actual frozen n8 failure, preserving baseline input and integer workload counters. Classifies exact complete attempted star identities/versions/coordinates/bindings and missed invalidation events in the serial trace. Rejected private SU2 grids are for inspection, not accepted CFD or independently certified mesh/field outputs. No distributed fix, accepted n8 result, later-cycle or performance certificate.')
(folder/'analysis.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('identical_full_star_repeat_rejections','changed_full_star_repeat_rejections','missed_full_star_reactivation_events','artifact_sha256','source_sha256')},indent=2))
print('identical_repeat_rejections=',len(duplicates),'changed_star_repeat_rejections=',len(changed_rejections),'missed_reactivation_events=',len(misses))
