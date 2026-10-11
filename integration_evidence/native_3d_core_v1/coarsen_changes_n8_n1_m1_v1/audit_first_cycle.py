"""Audit the actual first accepted n8 event; the full campaign remains rejected at cycle1."""
from pathlib import Path
import hashlib,json,sys,time
folder=Path(__file__).resolve().parent;where=folder.parent
sys.path.insert(0,str(where))
from audit_coupled import frame
from audit_geometry_bridge import states,signature,su2_signature
from audit_envelope import WEDGE_MARKERS,wedge_domain
from audit_lifecycle import parse,nested_stage,stage,transport_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
files=['envelope_s0_initial.json','envelope_s0.json','envelope_s0_states.json','envelope_mesh_00001.su2','case.stdout']
pins={n:sha(folder/n) for n in files}
assert json.loads((folder/'validation.json').read_text())['status']=='FAIL'
assert all(row['equal'] for row in json.loads((folder/'baseline_comparison.json').read_text()).values())
result=dict(status='FAIL',campaign_status='FAIL',cycles_audited=[0],input_sha256=pins)
try:
    print('Exact original mesh geometry/field audit',flush=True)
    initial,a=frame(folder/'envelope_s0_initial.json',False)
    print('Exact accepted mesh geometry/field audit',flush=True)
    accepted,b=frame(folder/'envelope_s0.json',True)
    assert initial['volume']==accepted['volume']=='1/2'
    assert a['marker_names']==b['marker_names'] and a['facets']==b['facets'] and a['facet_groups']==b['facet_groups']
    wedge_domain(a,1);wedge_domain(b,1)
    print('Original-P1/current+BDF transfer and actual SU2 output audit',flush=True)
    transferred=states(b,json.loads((folder/'envelope_s0_states.json').read_text()))
    assert su2_signature(folder/'envelope_mesh_00001.su2',WEDGE_MARKERS)==signature(b,WEDGE_MARKERS)
    logs=(folder/'case.stdout').read_text().splitlines()
    native=[parse(s) for s in logs if s.startswith('NATIVE3D ')]
    assert len(native)==2 and [r['complete'] for r in native]==[1,0]
    assert all(transport_counters(r) for r in native)
    details=[parse(s) for s in logs if s.startswith('NATIVE3D_DETAIL ')]
    for row in details:nested_stage(row)
    assert all(sha(folder/n)==h for n,h in pins.items())
    result.update(status='PASS',frames=[initial,accepted],transport=transferred,
                  scope='First actual accepted n8 event only: exact positive volumes, no tetrahedral overlap, complete manifold links/physical skin, convex-reference containment/coverage and unchanged geometry/markers, frozen original-P1 centroid q/J and analytic integrated edge bounds, transported nodal sensor/current+BDF histories, actual SU2 output and closed detailed phase/notification/transport rows. The full three-event campaign remains FAIL at cycle1; no private second-cycle, CGNS restart, timing/scaling or Goal1 completion claim.')
except Exception as error:
    result['reason']=str(error);raise
finally:
    result['script_sha256']=sha(Path(__file__).resolve())
    result['tools_sha256']={n:sha(where/n) for n in ('audit_coupled.py','audit_geometry_bridge.py','audit_envelope.py','audit_lifecycle.py','audit_cavities.py')}
    (folder/'first_cycle_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],'first accepted cycle; full campaign',result['campaign_status'],flush=True)
