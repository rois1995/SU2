"""Recover completed exact geometry stages and finish the correct rotating-sensor/state check; retain v1 FAIL."""
from pathlib import Path
import hashlib,json,math,sys
folder=Path(__file__).resolve().parent;where=folder.parent
sys.path.insert(0,str(where))
from audit_geometry_bridge import states,signature,su2_signature
from audit_envelope import WEDGE_MARKERS,wedge_domain
from audit_lifecycle import parse,nested_stage,transport_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
failed=json.loads((folder/'first_cycle_audit.json').read_text())
assert failed['status']=='FAIL' and failed['campaign_status']=='FAIL'
assert sha(folder/'audit_first_cycle.py')==failed['script_sha256']
assert all(sha(folder/n)==h for n,h in failed['input_sha256'].items())
assert all(sha(where/n)==h for n,h in failed['tools_sha256'].items())
assert (folder/'first_cycle_audit.stdout').read_text().splitlines()==[
 'Exact original mesh geometry/field audit','Exact accepted mesh geometry/field audit',
 'Original-P1/current+BDF transfer and actual SU2 output audit','FAIL first accepted cycle; full campaign FAIL']
stderr=(folder/'first_cycle_audit.stderr').read_text()
assert 'audit_geometry_bridge.py", line 109, in states' in stderr
assert 'assert max(abs(a-b) for a,b in zip(expected,v[4]))<1e-13' in stderr
# Reaching that pinned progress marker proves both complete frame() calls and reference comparisons finished.
initial=json.loads((folder/'envelope_s0_initial.json').read_text());data=json.loads((folder/'envelope_s0.json').read_text())
assert initial['facets']==data['facets'] and initial['facet_groups']==data['facet_groups'] and initial['marker_names']==data['marker_names']
wedge_domain(initial,1);wedge_domain(data,1)
def sensor_at(point,scale):
 x,y,z=point;q=scale*(1.+.125*x);phi=math.pi*(.25+.25*x);theta=math.pi*y/6.
 u,v,w=math.cos(phi)*math.cos(theta),math.sin(phi)*math.cos(theta),math.sin(theta);delta=1.5**2-1.
 return q*(1.+delta*u*u),q*delta*u*v,q*delta*u*w,q*(1.+delta*v*v),q*delta*v*w,q*(1.+delta*w*w)
transfer=states(data,json.loads((folder/'envelope_s0_states.json').read_text()),sensor_at)
assert su2_signature(folder/'envelope_mesh_00001.su2',WEDGE_MARKERS)==signature(data,WEDGE_MARKERS)
lines=(folder/'case.stdout').read_text().splitlines()
native=[parse(line) for line in lines if line.startswith('NATIVE3D ')]
assert len(native)==2 and [row['complete'] for row in native]==[1,0]
assert all(transport_counters(row) for row in native)
details=[parse(line) for line in lines if line.startswith('NATIVE3D_DETAIL ')]
for row in details:nested_stage(row)
assert all(sha(folder/n)==h for n,h in failed['input_sha256'].items())
result=dict(status='PASS',campaign_status='FAIL',cycles_audited=[0],cells=len(data['cells']),points=len(data['nodes']),
            transport=transfer,recovered_geometry_receipt_sha256=sha(folder/'first_cycle_audit.json'),
            recovered_geometry_stage_sha256=sha(folder/'first_cycle_audit_stage.json'),
            recovered_geometry_stdout_sha256=sha(folder/'first_cycle_audit.stdout'),
            recovered_geometry_stderr_sha256=sha(folder/'first_cycle_audit.stderr'),
            input_sha256=failed['input_sha256'],tools_sha256=failed['tools_sha256'],script_sha256=sha(Path(__file__).resolve()),
            scope='Recovered completed exact initial/accepted geometry/original-P1 q/J/edge frame audits from pinned v1 execution before its wrong-default-sensor metadata assertion. Correct rotating sensor callback, transported original-P1/current+BDF states, unchanged reference, actual SU2 and phase/notification/transport checks now complete. Original v1 failure retained. First cycle only; whole campaign remains FAIL at cycle1. No relaxed checks, numerical rerun, private cycle1 mesh certificate or performance/scaling/fullGoal1 claim.')
(folder/'first_cycle_audit_v2.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS first accepted cycle only; full campaign FAIL',transfer)
