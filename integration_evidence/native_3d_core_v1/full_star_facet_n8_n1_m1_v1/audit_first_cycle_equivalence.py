"""Reuse exact first-event geometry certification only after verifying byte-identical actual artifacts."""
from pathlib import Path
import hashlib,json,math,sys
folder=Path(__file__).resolve().parent;where=folder.parent
prior=where/'coarsen_changes_n8_n1_m1_v1'
sys.path.insert(0,str(where))
from audit_geometry_bridge import states,signature,su2_signature
from audit_envelope import WEDGE_MARKERS,wedge_domain
from audit_lifecycle import parse,nested_stage,transport_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
certificate=json.loads((prior/'first_cycle_audit_v2.json').read_text())
assert certificate['status']=='PASS' and certificate['cycles_audited']==[0]
assert sha(prior/'audit_first_cycle_v2.py')==certificate['script_sha256']
assert all(sha(prior/n)==h for n,h in certificate['input_sha256'].items())
assert all(sha(where/n)==h for n,h in certificate['tools_sha256'].items())
for key,name in [('recovered_geometry_receipt_sha256','first_cycle_audit.json'),
                 ('recovered_geometry_stage_sha256','first_cycle_audit_stage.json'),
                 ('recovered_geometry_stdout_sha256','first_cycle_audit.stdout'),
                 ('recovered_geometry_stderr_sha256','first_cycle_audit.stderr')]:
 assert sha(prior/name)==certificate[key]
original=json.loads((prior/'validation.json').read_text())
current=json.loads((folder/'validation.json').read_text())
assert original['status']==current['status']=='FAIL'
assert not any(current[k] for k in ('source_drift','object_drift','archive_drift','tool_drift'))
names=['envelope_s0_initial.json','envelope_s0.json','envelope_s0_states.json',
       'envelope_mesh_00001.su2','envelope_mesh_00001.su2.native_ref','envelope_s1_initial.json']
identical={}
for name in names:
 assert sha(prior/name)==original['artifact_sha256'][name]
 assert sha(folder/name)==current['artifact_sha256'][name]==sha(prior/name),name
 identical[name]=sha(folder/name)
data=json.loads((folder/'envelope_s0.json').read_text());wedge_domain(data,1)
def sensor_at(point,scale):
 x,y,z=point;q=scale*(1.+.125*x);phi=math.pi*(.25+.25*x);theta=math.pi*y/6.
 u,v,w=math.cos(phi)*math.cos(theta),math.sin(phi)*math.cos(theta),math.sin(theta);delta=1.5**2-1.
 return q*(1.+delta*u*u),q*delta*u*v,q*delta*u*w,q*(1.+delta*v*v),q*delta*v*w,q*(1.+delta*w*w)
transfer=states(data,json.loads((folder/'envelope_s0_states.json').read_text()),sensor_at)
assert su2_signature(folder/'envelope_mesh_00001.su2',WEDGE_MARKERS)==signature(data,WEDGE_MARKERS)
lines=(folder/'case.stdout').read_text().splitlines();native=[parse(s) for s in lines if s.startswith('NATIVE3D ')]
assert [r['complete'] for r in native]==[1,0]
assert all(transport_counters(r) for r in native)
for line in lines:
 if line.startswith('NATIVE3D_DETAIL '):nested_stage(parse(line))
result=dict(status='PASS',campaign_status='FAIL',cycles_audited=[0],cells=len(data['cells']),points=len(data['nodes']),
 transport=transfer,identical_actual_artifacts_sha256=identical,prior_certificate_sha256=sha(prior/'first_cycle_audit_v2.json'),
 current_validation_sha256=sha(folder/'validation.json'),script_sha256=sha(Path(__file__).resolve()),
 tools_sha256=certificate['tools_sha256'],
 scope='Exact first-event geometry/original-P1 q/J/integrated-length certification reused from pinned prior execution solely for six byte-identical current actual mesh/state/reference/next-donor artifacts. Correct sensor, current+BDF transfer and actual SU2 signature rechecked here; current runtime nested accounting checked. No new full geometry audit run, second-event private-grid certificate, whole-campaign success, speedup or fullGoal1 claim.')
(folder/'first_cycle_equivalence_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS byte-identical independently certified first event; whole campaign FAIL',transfer)
