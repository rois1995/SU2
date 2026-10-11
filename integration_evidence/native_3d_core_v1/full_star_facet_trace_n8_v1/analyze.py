"""Compare the isolated current-source trace with the actual failed campaign and classify complete-star retries."""
from collections import Counter,defaultdict
from pathlib import Path
import hashlib,json,sys
folder=Path(__file__).resolve().parent;base=folder.parent;reference=base/'full_star_facet_n8_n1_m1_v1'
sys.path.insert(0,str(base))
from audit_lifecycle import parse,nested_stage,transport_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((folder/'validation.json').read_text())
assert receipt['status']=='PASS' and receipt['adaptation_status']=='FAIL'
assert not receipt['source_drift'] and not receipt['object_drift'] and not receipt['library_drift']
for name in ('parameters.json','case_environment.json','envelope.cfg','envelope_input.su2','envelope_s0_initial.json',
             'envelope_s0.json','envelope_s0_states.json','envelope_s1_initial.json'):
 assert (folder/name).read_bytes()==(reference/name).read_bytes(),name
work_keys=set('rank ranks workers complete detailed initial final refine_accepted coarsen_accepted tracked_bytes tracked_calls tracked_import_bytes tracked_import_calls tracked_parent_bytes tracked_parent_calls tracked_worker_bytes tracked_worker_calls native_bound requests hits misses invalid_queries failures evictions compositions batches retries candidates payloads maximum_patch moved arcs cells_before cells_after work_before work_after directory_elections rounds attempted accepted rejected conflicts edge_evaluations collapse_geometry_checks collapse_filtered collapse_orientation_checks collapse_orientation_filtered collapse_reactivated changed_star_scans changed_vertices_sent changed_vertices_received edge_reused reconnected inserted maximum_donor_patch maximum_owned donor_reused edge_separated donor_batches donor_candidates donor_filtered donor_payloads batch_retries query_retries private_query_retries final_owned_cells cached_measured_cells cached_invalid_cells remaining_edges remaining_repairs'.split())
def rows(where):return [(line.split()[0],parse(line)) for line in (where/'case.stdout').read_text().splitlines() if line.startswith(('NATIVE3D ','NATIVE3D_DETAIL '))]
old,new=rows(reference),rows(folder);assert len(old)==len(new)==16
equal=0
for (ot,o),(nt,n) in zip(old,new):
 assert ot==nt and o.get('stage')==n.get('stage')
 for key in sorted(set(o)&set(n)&work_keys):assert o[key]==n[key] and int(o[key])==o[key],(n.get('stage'),key);equal+=1
 if nt=='NATIVE3D_DETAIL':nested_stage(n)
 else:assert transport_counters(n)
assert 'assertions: 5049 | 5048 passed | 1 failed' in (folder/'case.stdout').read_text()
events=[json.loads(line) for line in (folder/'attempts_rank0.jsonl').read_text().splitlines()]
assert len(events)==940
per_epoch={}
def fingerprint(rows):return tuple(sorted((r['id'],r['version'],tuple(tuple(n) for n in r['nodes']),tuple(tuple(f) for f in r['physical'])) for r in rows))
for epoch,count,accepted in [(0,428,272),(1,512,437)]:
 group=[row for row in events if row['epoch']==epoch]
 assert len(group)==count and sum(row['approved'] for row in group)==accepted
 assert [row['round'] for row in group]==list(range(count))
 last={};identical=[];changed=[];reasons=Counter();rejected=[]
 for row in group:
  assert not row['repairing'];edge=tuple(row['edge']);support=fingerprint(row['old'])
  assert row['old'] and all(any(n[0]==edge[1] for n in cell['nodes']) for cell in row['old'])
  if not row['approved']:
   rejected.append(row);reasons[row['reason'].split(':',1)[0]]+=1
   if edge in last:
    (identical if last[edge]['support']==support else changed).append(dict(round=row['round'],edge=edge,previous=last[edge]['round']))
   last[edge]=dict(support=support,round=row['round'])
 per_epoch[epoch]=dict(attempts=count,accepted=accepted,rejected=len(rejected),unique_rejected_directions=len(last),
                       identical_full_star_repeat_rejections=identical,changed_full_star_repeat_rejections=changed,reason_counts=dict(reasons))
private=[json.loads(line) for line in (folder/'private_mesh_status.jsonl').read_text().splitlines()]
assert [(row['epoch'],row['cells'],row['valid_cached_exit']) for row in private]==[(0,3552,True),(0,2489,True),(1,2489,True),(1,517,False)]
result=dict(status='PASS',adaptation_status='FAIL',equal_integer_work_counters=equal,epochs=per_epoch,private_meshes=private,
            validation_sha256=sha(folder/'validation.json'),script_sha256=sha(Path(__file__).resolve()),
            scope='Source-matched isolated trace of the actual current-source failed n8 campaign, with eight identical input/accepted/state snapshots and all selected integer work/communication/queue counts. Classifies exact complete-star rejection repeats per immutable-target event and exposes private mesh progression. Diagnostic grids are not accepted CFD outputs or independently certified second-cycle fields; no production change, budget relaxation or timing/scaling/fullGoal1 claim.')
result['artifact_sha256']={p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('test_driver_trace','engine_trace.o','analysis.json')}
(folder/'analysis.json').write_text(json.dumps(result,indent=2)+'\n')
for epoch,data in per_epoch.items():print('epoch',epoch,'attempts',data['attempts'],'accepted',data['accepted'],'reasons',data['reason_counts'],'identical repeats',len(data['identical_full_star_repeat_rejections']),'changed repeats',len(data['changed_full_star_repeat_rejections']))
print('PASS',equal,'equal integer counters')
