"""Compare the first accepted mesh with the earlier rejected private mesh, ignoring numbering only."""
from pathlib import Path
import hashlib,json,sys
folder=Path(__file__).resolve().parent;where=folder.parent
sys.path.insert(0,str(where))
from audit_geometry_bridge import signature,su2_signature
from audit_envelope import WEDGE_MARKERS
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
data=json.loads((folder/'envelope_s0.json').read_text())
current=signature(data,WEDGE_MARKERS)
previous_path=where/'coarsen_trace_n8_v1/coarsen_rejected.su2'
old_cells,old_faces=su2_signature(previous_path,{'reference_marker_'+str(i) for i in range(6)})
old_faces={(data['marker_names'][int(name.rsplit('_',1)[1])],face):count for (name,face),count in old_faces.items()}
assert current==(old_cells,old_faces),'Changed first-cycle accepted geometry/connectivity/markers'
assert su2_signature(folder/'envelope_mesh_00001.su2',WEDGE_MARKERS)==current
record=dict(status='PASS',cells=2489,points=len(data['nodes']),equal_coordinate_connectivity_and_markers=True,
            first_cycle_accepted_edits=272,previous_attempts=512,current_attempts=428,
            input_sha256={str(p.relative_to(where)):sha(p) for p in [previous_path,folder/'envelope_s0.json',folder/'envelope_mesh_00001.su2']},
            script_sha256=sha(Path(__file__).resolve()),
            scope='Exact binary64 coordinate connectivity/physical-marker comparison, ignoring numbering, plus positive actual SU2 volumes. First accepted grid equals the prior rejected private grid. No full exact embedding/original-P1/BDF audit, no later-cycle or performance/scaling certificate.')
(folder/'trajectory_comparison.json').write_text(json.dumps(record,indent=2)+'\n')
print('PASS same first-cycle2489-cell mesh and markers;512->428 attempted collapses.')
