"""Verify the accepted local parameter witness independently; never claims whole-mesh acceptance."""
import contextlib,io,json,runpy,sys
from pathlib import Path
import numpy as np
from analyze import Reader
from run import ROOT,sha,save

folder=ROOT/'offline7'
status=json.loads((folder/'status.json').read_text());report=json.loads((folder/'repair_replay.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
assert not status['rss_cap_reached'] and not status['branch_wall_cap_reached']
for key in ['prescribed_interface','geometry_progress','transaction_metric','final_patch_metric','external_contact','accepted_local_repair']:
    assert report[key],key
assert not report['full_mesh_accepted'] and report['placement_trials']==15
with contextlib.redirect_stdout(io.StringIO()):
    env=runpy.run_path(str(ROOT/'rank_candidates.py'))
r=Reader((folder/'repair_candidate.bin').read_bytes());cells=[r.cell() for _ in range(r.count(128))]
surface=[r.take('QQQQ') for _ in range(r.count(512))];parameters=[]
for _ in range(r.count(512)):parameters.append(dict(node=r.take('Qddd'),binding=r.take('QQQdd')))
r.end();shapes=[]
for _,vertices in cells:
    p=np.asarray([n[1:] for n in vertices],dtype=np.longdouble)
    assert env['module'].exact_volume(p)>0
    shapes.append(env['module'].shape(p,env['field'](tuple(np.asarray(p.mean(axis=0),dtype=float)))))
q=float(min(m[0] for m in shapes));j=float(min(m[1] for m in shapes));assert q>=.2 and j>=.05
assert abs(q-report['q'])<1e-12 and abs(j-report['J'])<1e-12
seed=json.loads((ROOT/'selected_trials.json').read_text())['events'][1]['fresh']
old_nodes={n[0]:list(n[1:]) for _,v in seed['cells'] for n in v};new_nodes={n[0]:list(n[1:]) for _,v in cells for n in v}
assert old_nodes.keys()==new_nodes.keys();moved=[n for n in old_nodes if old_nodes[n]!=new_nodes[n]]
assert moved==[119505];assert [list(f) for f in surface]==seed['surface']
result=dict(independent_q=q,independent_J=j,positive_cells=len(cells),moved_nodes=moved,
            native_full_edge_certificate=report['final_patch_metric'],native_contact_certificate=report['external_contact'],
            full_mesh_accepted=False,pins={str(f):sha(f) for f in [Path(__file__),ROOT/'rank_candidates.py',ROOT/'selected_donors.json',folder/'repair_candidate.bin',folder/'repair_replay.json',folder/'status.json']})
save(folder/'independent_check.json',result);save(folder/'repair_candidate.json',dict(cells=cells,surface=surface,parameters=parameters))
print(json.dumps(result))
