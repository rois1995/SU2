"""Reuse actual residual localization, then prove complete current support from full readback."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k'
source=R/'analyze_geometry256_residuals.py';sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
ready=json.loads((A/'capture_ready.json').read_text());checked=json.loads((A/'inspection_reader_status.json').read_text())
assert checked['all_reader_checks_passed'] and checked['known_core_global_vertex_stars_complete']
assert ready['owned_rows']==ready['expected_owned_rows']==checked['actual_cells']
assert json.loads((A/'inspection_reader_terminal_handles.json').read_text())['all_owned_handles_terminal']
changes=[('euler_build256_canonical_geometry_debt_mach_only_real_cfd50k','euler_build262_compact_capture_mach_only_real_cfd50k'),('== 78','== 4'),('All78cached','All4cached')]
code=source.read_text()
for old,new in changes:assert code.count(old)==1,old;code=code.replace(old,new)
before={str(p):sha(p) for p in (source,Path(__file__).resolve(),A/'inspection_reader_status.json')}
exec(compile(code,str(source),'exec'),{'__file__':str(source),'__name__':'__main__'})
assert before=={p:sha(p) for p in before}
p=A/'geometry_residual_location.json';out=json.loads(p.read_text());expected={x['id']:x for x in checked['per_core_complete_contexts']}
for row in out['geometry_bad_cells']:
 oracle=expected[row['id']];assert row['available_four_star_rows']==oracle['four_star_union_size']
 assert row['available_vertex_star_sizes']==oracle['vertex_star_sizes'];row['global_support_complete']=True
out['feature_unknown_seeds']=sum(not x['feature_known'] for x in out['geometry_bad_cells'])
out['scope']='All4actualgeometrybadseeds with GLOBALcomplete vertexstars,proved againstindependentfullVTU/raw-ownedreadback. Nullq/J remainunknown;unmeasuredfeatureflags areNOTproof ofnofeature. No candidate/admission/acceptance proof.'
common=set.intersection(*(set(x['nodes']) for x in out['geometry_bad_cells']));bad={x['id'] for x in out['geometry_bad_cells']};unknown=set(map(int,checked['unknown_cell_records']))
coverage=[]
for row in out['geometry_bad_cells']:
 if row['available_four_star_rows']>64:continue
 cells=set(row['available_context_cell_ids']);coverage.append({'core':row['id'],'cells':len(cells),'all_known_geometry_bad_covered':bad<=cells,'geometry_bad_in_context':sorted(bad&cells),'all_unknown_q_J_covered':unknown<=cells,'unknown_q_J_in_context':sorted(unknown&cells)})
out.update(common_nodes_in_all_geometry_bad=sorted(common),eligible_existing64_context_coverage=coverage,full_readback_sha256=sha(A/'inspection_reader_status.json'),localizer_reuse={'changes':changes,'helper_hashes':before,'transformed_sha256':hashlib.sha256(code.encode()).hexdigest()})
p.write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({k:out[k] for k in ('geometry_bad_seeds','seeds_with_unknown_shape','feature_unknown_seeds','common_nodes_in_all_geometry_bad','eligible_existing64_context_coverage')}))
