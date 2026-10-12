"""Compare reservation work in two independently audited changed trajectories."""
from pathlib import Path
import hashlib,json,sys
folder=Path(__file__).resolve().parent;base=folder.parent;root=base.parents[1]
sys.path[:0]=[str(base),str(root/'integration_evidence/native_cluster_3d_core_v1')]
from audit_lifecycle import parse
from profile_summary import profiles
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
control=base/'box_filter_n4_n4_m2_v1'
records=[]
for path in (control,folder):
 v=json.loads((path/'validation.json').read_text());assert v['status']=='PASS'
 for name,h in v['artifact_sha256'].items():assert sha(path/name)==h,(path.name,name)
 a=json.loads((path/'independent_audit.json').read_text());assert a['status']=='PASS'
 lines=(path/'case.stdout').read_text().splitlines()
 n=[parse(l) for l in lines if l.startswith('NATIVE3D ')];s=[parse(l) for l in lines if l.startswith('NATIVE3D_STAGE ')];d=[parse(l) for l in lines if l.startswith('NATIVE3D_DETAIL ')]
 summary=profiles(n,s,d)
 events=[r for r in summary['nested_detail_events'] if r['stage'] in ('refine','coarsen')]
 work=[]
 for event in events:
  p=event['profile'];work.append(dict(stage=event['stage'],event_index=event['event_index'],proposal_outcomes=p['proposal_outcomes'],
      counts={k:p['measures'][k]['rank_values'] for k in ('rounds','attempted','accepted','rejected','conflicts','donor_candidates','donor_box_filtered','donor_payloads','tracked_bytes','tracked_calls')}))
 frames=[r for r in a['frames'] if r['file'] in {'envelope_s%d.json'%i for i in range(3)}]
 assert len(frames)==3 and all(r['minimum_mean_ratio']>=.20 and r['minimum_scaled_jacobian']>=.05 and r['maximum_length']<=1.8 for r in frames)
 records.append(dict(case=path.name,validation_sha256=sha(path/'validation.json'),audit_sha256=sha(path/'independent_audit.json'),log_sha256=sha(path/'case.stdout'),frames=frames,states=a['states'],events=work))
assert json.loads((control/'parameters.json').read_text())==json.loads((folder/'parameters.json').read_text())
initial={name:sha(folder/name) for name in ('envelope.cfg','envelope_input.su2','envelope_s0_initial.json')}
assert all(sha(control/name)==h for name,h in initial.items())
result=dict(status='PASS',initial_inputs_sha256=initial,cases=records,accepted_mesh_matches={f'envelope_s{i}.json':sha(folder/f'envelope_s{i}.json')==sha(control/f'envelope_s{i}.json') for i in range(3)},
    checker_sha256=sha(Path(__file__).resolve()),profile_summary_sha256=sha(root/'integration_evidence/native_cluster_3d_core_v1/profile_summary.py'),
    scope='One independently audited N4M2 rotating-priority directory prototype versus earlier source-pinned standard case. Initial geometry/frozen target/config match exactly; later meshes and per-event P1 references change. Operation/communication counts are descriptive work, not fixed-work speed/scaling, reduced-rank or general safety qualification. No production scheduling change.')
(folder/'comparison.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS:initial inputs exact; both trajectories independently audited; proposal/work counts retained without timing ratio')
