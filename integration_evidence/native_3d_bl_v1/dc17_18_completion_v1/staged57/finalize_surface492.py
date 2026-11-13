"""Consolidate the single492 run; preserve base geometry-unresolved and later direction evidence."""
from pathlib import Path
import collections,hashlib,json
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
checkpoint=json.loads((R/'saved_surface492_checkpoint.json').read_text());case=Path(checkpoint['source_case'])
native=json.loads((case/'actual_saved_surface_stage_rank0.json').read_text())
directions=json.loads((R/'nose492_point_direction_supplement.json').read_text())
status=json.loads((case/'status.json').read_text());handles=json.loads((case/'terminal_handles_actual.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified'] and handles['all_owned_handles_terminal']
assert native['splits']==305 and native['surface_prepared'] is False and checkpoint['current_nodes']==61855
raw=[json.loads(x) for x in (case/'nose_target_observation.jsonl').read_text().splitlines()];groups=collections.Counter()
for row in raw:
    if row['stage']!='refusal':continue
    why=row['reason']
    key='frozen_geometry_nonprogress' if 'preserve/improve' in why else 'incompatible_height_planes' if 'incompatible height planes' in why else 'normal_row_clearance' if 'geometry-height incompatibility' in why else 'no_normal_foot_freedom' if 'leave no normal-foot freedom' in why else 'longest_edge_frontier' if 'longest changed-edge' in why else 'incomplete_free_apex_stars' if 'complete free apex stars' in why else 'other_link_or_visibility_or_fit'
    groups[key]+=1
summaries=[r for r in raw if r['stage']=='fixed_summary'];assert len(summaries)==2 and all(r['measured']==0 for r in summaries)
margins=[];gaps=[]
for row in directions['records']:
    if row['label']!='spanwise':continue
    for side in row['incident_samples']:
        k=side['curvatures'];i=max(range(2),key=lambda j:abs(k[j]));p=side['projections']
        margins.append(p[1-i]-p[i]);gaps.append((abs(k[i])-abs(k[1-i]))/max(abs(k[i]),1e-300))
assert directions['counts']['unknown']==0 and directions['counts']['spanwise']>directions['nose_split_records']/2
metadata=json.loads((R/'target492_metadata_summary.json').read_text())['rank_headers'][0]['metadata']
inputs=[R/'joint_curvature492_debt.json',R/'surface492_pre_joint_review_outcome.json',R/'saved_surface492_checkpoint.json',R/'saved_production_surface492_serial_assessment.json',R/'nose_target_observation492_assessment.json',R/'nose492_direction_supplement.json',R/'nose492_point_direction_supplement.json',R/'target_checkpoint492_qualification.json',R/'target_reference492_fidelity.json',case/'status.json',case/'terminal_handles_actual.json',case/'actual_saved_surface_stage_rank0.json',Path(__file__).resolve()]
result=dict(decision='reject_tested_size_only_implementation',reason='36 of65 published fixed-nose splits remain spanwise at their saved bindings: the frozen plan direction-reject criterion is met.',
    surface_ready=False,final_mesh_accepted=False,goal_complete=False,one_production_run=True,no_more_local_variants_this_cycle=True,
    plan=str(R.parents[3]/'Handbook/ADAPTATION_MAPPED_CURVATURE_TARGET_492_20261113.md'),case=str(case),
    nominal_complexity=500000,seed_target_estimate=metadata['estimated_complexity'],empirical_uncertainty_indicator=metadata['indicator'],
    tangent_scale=metadata['tangential_scale'],core=metadata['core'],evolved_mesh_complexity='unmeasured',
    input_mesh=dict(nodes=61550,tets=303823),output_mesh=dict(nodes=checkpoint['current_nodes'],tets=checkpoint['current_cells'],positive_tets=native['positive_cells'],provisional=True),
    configured_native_seconds=native['maximum_seconds'],native_seconds=native['seconds'],outer_seconds=status['seconds'],seconds_per_accepted_split=native['seconds']/native['splits'],outer_seconds_per_accepted_split=status['seconds']/native['splits'],
    accepted_splits=native['splits'],accepted_threshold_250_met=True,
    geometry_guidance_evaluations=native['geometry_guidance_trials'],geometry_guidance_refusals=native['geometry_guidance_rejected'],geometry_guidance_refusal_fraction=native['geometry_guidance_rejected']/native['geometry_guidance_trials'],
    top_level_rejected_or_deferred_selections=next(r['total_rejected'] for r in raw if r['stage']=='split_summary'),private_failed_checks=sum(groups.values()),private_failure_categories=dict(groups),
    logical_split_trial_denominator='not separately serialized; do not substitute private checks or rejected/deferred selections',
    direction=dict(counts=directions['counts'],nose_split_records=directions['nose_split_records'],spanwise_fraction=directions['counts']['spanwise']/directions['nose_split_records'],minimum_spanwise_projection_margin=min(margins),minimum_relative_principal_gap=min(gaps),maximum_forward_roundoff_error=directions['maximum_forward_roundoff_error'],source='Exact saved bindings/all incident WING3D sides of immutable cubic authority; local differential diagnostic, not global curvature certificate.'),
    geometry_criterion='unresolved: first full-entity query exhausted each30s observation budget; no completed per-face before/after certificates or two-halving proof',fixed_face_observations=summaries,
    surface_census=dict(measured=native['surface_measured'],required=native['surface_required'],fraction=native['surface_measured']/native['surface_required'],complete=native['surface_census_complete']),
    native_geometry_work=native['geometry_work'],diagnostic_geometry_work=sum(s['geometry_work'] for s in summaries),native_reserved_work_bytes=native['maximum_work_bytes'],per_rank_memory_ceiling=12<<30,
    item3='Size demand responds, but geometric split direction/selection and unweighted admission objective remain unchanged. Spanwise majority and705 frozen-error refusals remain observed obstacles; height/normal-foot/link refusals also persist.',
    item4='Unequal native old/fresh bound tightness remains unchanged. Existing489 attribution is preserved; this run cannot separate genuine regression from conservative refusal. Full-entity certificate cost blocks fixed-face comparison in its allotted time.',
    next='Stop the requested cycle. Review coordinated surface construction/selection/admission and certification strategy before another run; no raised limits, relaxed gates or further local nose variants.',
    measurement_limits=['Initial bound-only assessment is unresolved; this consolidated result adds exact-binding direction evidence and rejects on that independent frozen criterion.','No progress conclusion from changing bad-face counts or pre-BL q/J.','No controlled algorithmic speedup claim: target and saved input differ from historical484.','All final geometry/quality/height/transfer/resource gates and the complete RANS workflow remain outstanding.'],
    provenance={str(p):sha(p) for p in inputs})
result['joint_envelope_debt']={'path':str(R/'joint_curvature492_debt.json'),'sha256':sha(R/'joint_curvature492_debt.json'),'proven':True}
result['qualification_limit']='Focused individual-principal constraints and checkpoint fidelity pass, but the combined oblique curvature envelope is not guaranteed. This rejects the tested field, not every correctly intersected curvature tensor.'
result['before_further_qualification']='Correct the joint tangent curvature intersection and distinguish its checkpoint semantics before any rebuilt-target/wider doubly-curved qualification; no additional implementation or native run in this closed cycle.'
output=R/'surface492_outcome.json';assert not output.exists();output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ('decision','accepted_splits','native_seconds','seconds_per_accepted_split','output_mesh','surface_census')}))
