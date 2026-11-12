"""Bounded-support topology comparison; this does not construct a replacement."""
import contextlib,hashlib,io,json,runpy
from pathlib import Path
R=Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    replay=runpy.run_path(str(R/'surface_patch465_topology_audit.py'))
assessment=R/'surface_patch466_assessment.json'
a=json.loads(assessment.read_text())
extra={tuple(sorted((f['vertices'][k],f['vertices'][(k+1)%3])))
       for f in a['records'] if f['stage']=='trajectory_face' and f['pass']==0 and f['bound']>1e-4 for k in range(3)}
edges=replay['edges']|extra
support=[c for c in replay['cells'] if any(u in c and v in c for u,v in edges) or 4218 in c or 4219 in c]
topology=replay['ball'](support)
vertices={x['vertex'] for x in topology['bad_vertex_links']}
closure=set(support)|{c for c in replay['cells'] if any(v in c for v in vertices)}
assert len(support)==57 and len(closure)==80
report=dict(additional_original_neighbor_demand_edges=len(extra-replay['edges']),complete_proposed_support_cells=len(support),
    combinatorial_topology=topology,within_64=len(support)<=64,
    full_star_closure_vertices=sorted(vertices),full_star_closure_cells=len(closure),
    full_star_closure_topology=replay['ball'](list(closure)),full_star_closure_within_64=len(closure)<=64,
    assessment_sha256=hashlib.sha256(assessment.read_bytes()).hexdigest(),script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope='Original neighboring-demand support preflight only. No replacement feasibility, native acceptance, permission to enlarge a reservation or proof that every smaller corrected cavity fails.')
(R/'surface_patch466_neighbor_support_preflight.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ('complete_proposed_support_cells','full_star_closure_cells','full_star_closure_within_64')},indent=2))
