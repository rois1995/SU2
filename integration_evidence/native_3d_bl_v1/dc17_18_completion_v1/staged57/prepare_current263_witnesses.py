"""Prepare the one complete eligible actual262 context for production-source checks."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
checked=json.loads((A/'inspection_reader_status.json').read_text());assert checked['all_reader_checks_passed'] and checked['known_core_global_vertex_stars_complete']
location=json.loads((A/'geometry_residual_location.json').read_text());row=next(r for r in location['geometry_bad_cells'] if r['id']==155561218)
assert row['global_support_complete'] and row['available_four_star_rows']==42
selected={'id':row['id'],'role':'actual_four_geometry_failures','nodes':row['nodes'],'context_cell_ids':row['available_context_cell_ids'],'four_star_union_size':42,'vertex_star_sizes':row['available_vertex_star_sizes'],'within_existing64':True,'physical_faces':len(row['physical_faces']),'geometry_bad':True}
selection=A/'bounded_residual_selection263.json';assert not selection.exists()
selection.write_text(json.dumps({'schema':'actual262_source263_context_v1','source_build':'build262','expected_bounded_contexts':1,'bounded_selected_contexts':[selected],'source_location_sha256':sha(A/'geometry_residual_location.json'),'full_readback_sha256':sha(A/'inspection_reader_status.json'),'scope':'OneGLOBALcomplete42context at unchanged64bound;all4knowngeometrybad/3unknownqJ.Preserveoutsideinterfaces;not allglobalunknowns.'},indent=2)+'\n')
source=R/'prepare_current237_witnesses.py';code=source.read_text()
changes=[
 ('    assert any(row["physical_faces"] == 0 for row in chosen)\n',''),
 ('    assert any(row["physical_faces"] > 0 and not row["geometry_bad"] for row in chosen)\n',''),
 ('                assert metric["q"] is not None and metric["J"] is not None','                assert (metric["q"] is not None)==metric["q_measured"] and (metric["J"] is not None)==metric["j_measured"]'),
 ('values = [cell["id"], metric["q"], metric["J"]]','values = [cell["id"], metric["q"] if metric["q"] is not None else math.nan, metric["J"] if metric["J"] is not None else math.nan]'),
 ('for first, second, _ in metric["edges"]:\n                    values.extend((first, second, math.nan))','for first, second, length in metric["edges"]:\n                    assert length is not None and math.isfinite(length)\n                    values.extend((first, second, length))'),
]
for old,new in changes:assert code.count(old)==1,old;code=code.replace(old,new)
before={str(p):sha(p) for p in (source,Path(__file__).resolve(),selection,A/'geometry_residual_location.json',A/'inspection_reader_status.json')}
context={'__file__':str(source),'__name__':'source263_preparation'};exec(compile(code,str(source),'exec'),context);context['prepare'](A,selection)
assert before=={p:sha(p) for p in before}
p=A/'current_witnesses_v1/witness_preparation.json';receipt=json.loads(p.read_text())
receipt.update(schema='native_current_source263_preparation_v1',expected_unknown_q_J=3,production_metric_semantics='Frozen current dependency sensors transported onto candidate connectivity;emptyM6composition/edgebreakpoints. No original-global target policy change.',old_cached_geometry_bad=4,old_cached_canonical_geometry_excess=sum(r['geometry_excess_l2']**2 for r in location['geometry_bad_cells']),preparer_reuse={'changes':changes,'helper_hashes':before,'transformed_sha256':hashlib.sha256(code.encode()).hexdigest()},scope='Complete actual42currentcontext;knowncachedq/J andACTUALedgeoracles;unknownq/J explicitlyNaN. New source263 fixture variant required;no cachedmatchclaimed forunknowns.')
p.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ('paired_rows','boundary_points','physical_faces','expected_unknown_q_J','old_cached_geometry_bad','old_cached_canonical_geometry_excess')}))
