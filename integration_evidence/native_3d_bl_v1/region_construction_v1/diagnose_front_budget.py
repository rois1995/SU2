"""Diagnose the fixed-topology band's cell-cost obstruction without a new SU2 run."""
import collections,json,math
from pathlib import Path
import mpmath as mp
from construct import HERE,sha
mp.mp.dps=70
case=HERE/'native_front_constant_v1';data=json.loads((case/'core_construction_rejected.json').read_text());initial=json.loads((case/'core_initial.json').read_text());retained=json.loads((case/'core_input_retained.json').read_text())
assert all(initial[k]==retained[k] for k in ('cells','nodes','surface','facets','facet_groups','marker_names','original'))
markers={f['id']:data['marker_names'][f['marker']] for f in data['facets']};wall=[f for f in data['surface'] if markers[f['facet']]=='cylinder']
wall_nodes={n for f in wall for n in f['nodes']};apices={n for c in data['cells'] if sum(i in wall_nodes for i in c['nodes'])==3 for n in c['nodes'] if n not in wall_nodes}
first=[c for c in data['cells'] if any(n in apices for n in c['nodes'])];edges=collections.Counter(tuple(sorted((f['nodes'][i],f['nodes'][(i+1)%3]))) for f in wall for i in range(3))
rim=sum(v==1 for v in edges.values());interior=sum(v==2 for v in edges.values());assert all(v in (1,2) for v in edges.values())
per_band=2*len(wall)+4*(rim+interior);assert len(first)==per_band==840
nodes={n[0]:n[1:] for n in data['nodes']};radius=min(math.hypot(nodes[n][0],nodes[n][1]) for c in first for n in c['nodes'])
h,growth,thickness=mp.mpf('.01'),mp.mpf('1.2'),mp.mpf('.2');distance=1-mp.mpf(radius);full=mp.mpf('.9')*thickness
def primitive(d):return d/h if d<=h/2 else mp.mpf('.5')+(growth+1)/(2*(growth-1))*mp.log((h+(growth-1)*d)/(h+(growth-1)*h/2))
remaining=primitive(full)-primitive(distance);segments=int(mp.ceil(remaining/mp.mpf('1.8')));minimum_cells=(segments+1)*per_band
assert segments==4 and minimum_cells==4200 and minimum_cells>4096
report={'status':'PASS_FIXED_BAND_BUDGET_OBSTRUCTION','physical_wall_triangles':len(wall),'wall_rim_edges':rim,'wall_interior_edges':interior,'cells_per_existing_band':per_band,
        'first_front_minimum_radius':radius,'unfaded_metric_span_remaining_lower_bound':str(remaining),'minimum_additional_fronts_with_unchanged_edge_limit':segments,
        'minimum_cells_before_core_and_planning':minimum_cells,'event_work_limit':4096,'retained_input_identical':True,
        'input_rejection_sha256':sha(case/'core_construction_rejected.json'),'native_receipt_sha256':sha(case.with_suffix('.json')),'helper_sha256':sha(Path(__file__)),
        'scope':'This constant-sensor cylinder keeps all wall-front edges/topology and uses the existing coordinated Band. Every additional band costs840 tetrahedra. Even using the full1.8 radial-edge allowance, at least4 further bands are required after the first;4200 cells already exceed4096 before core/sampling/front work. The outer fade is omitted, so the normal span is a lower bound. This proves an obstruction to this fixed-topology band strategy, not physical mesh infeasibility or a universal cell lower bound. More efficient connectivity/front distribution/core coupling is required; changing1.62 to1.8 alone cannot close this case.'}
(case/'construction_cost_lower_bound.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],minimum_cells,'>',4096)
