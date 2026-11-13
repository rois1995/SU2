"""Verify provisional492 PrepareSurface output and summarize fixed-nose observations."""
import hashlib,json,math,struct,sys
from pathlib import Path
from assess_surface_transaction470 import sha
from inspect_native_failure3d import _read_owned
R=Path(__file__).resolve().parent
if len(sys.argv)!=4:
    raise SystemExit('usage: assess_saved_production_surface492.py CASE_DIR PRODUCER_DIR QUALIFICATION_JSON')
case,producer,qualification=map(lambda p:Path(p).resolve(),sys.argv[1:])
status=json.loads((case/'status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
handles=json.loads((case/'terminal_handles_actual.json').read_text())
assert handles['actual_host_proc'] and handles['all_owned_handles_terminal']
producer_receipt=json.loads((R/'target_checkpoint492_producer.json').read_text())
qualified=json.loads(qualification.read_text())
assert producer_receipt['producer']==str(producer)==qualified['producer']
assert qualified['actual_target_checkpoint_qualified'] and qualified['recomposed_target_checked_after_reload']
assert sha(R/'target_checkpoint492_qualification.json')==sha(qualification)
fixed=json.loads((R/'nose492_fixed_faces.json').read_text())
assert sha(R/'nose492_fixed_facets.dat') and fixed['faces']
source=json.loads((R/'saved_surface485_checkpoint.json').read_text())
for key in ('owned','parameters','prepared_reference'):
    item=source[key];assert sha(item['path'])==item['sha256']
assert source['current_cells']==303823 and source['current_nodes']==61550 and source['stage_height']==6.4e-5
seed=R/'cases/curved441_ramp5_seed_stage_20261112T123956_1290282'
assert sha(seed/'run.cfg')==source['configuration']['sha256']

manifests=sorted(case.glob('stage_resumed_surface_rank*_manifest.json'))
assert len(manifests)==1
m=json.loads(manifests[0].read_text());assert m['complete_shard'] and m['private_capture'] and not m['final_mesh_accepted']
assert m['rank']==0 and m['ranks']==1 and m['parameters_present'] and m['parameters_payload_bytes']>0
result=json.loads((case/'actual_saved_surface_stage_rank0.json').read_text())
assert result['rank']==0 and result['ranks']==1 and result['final_mesh_accepted'] is False
assert result['captured'] and result['same_target_policy'] and result['input_parameters_unchanged']
assert result['stage_height']==source['stage_height'] and result['maximum_seconds']==388.8 and result['maximum_work']==233220
files={name:case/path for name,path in m['files'].items()}
assert m['owned_record_bytes']==416 and m['donor_record_bytes']==344

def rows(path,width):
    with path.open('rb') as stream:
        n=struct.unpack('<Q',stream.read(8))[0]
        assert path.stat().st_size==8+n*width
        for _ in range(n):
            row=stream.read(width);assert len(row)==width;yield row
        assert not stream.read(1)

face_slots=((1,2,3),(0,3,2),(0,1,3),(0,2,1))
def physical_faces(path,expected_rows,facet_ids,maximum_bytes):
    found={};positions={}
    for cell in _read_owned(path,expected_rows,maximum_bytes):
        for node in cell['nodes']:
            if node['id'] in positions:assert positions[node['id']]==node['xyz']
            else:positions[node['id']]=node['xyz']
        for slot,(a,b,c) in enumerate(face_slots):
            physical=cell['physical'][slot]
            if physical['present'] and physical['facet'] in facet_ids:
                key=(physical['facet'],tuple(sorted(cell['nodes'][i]['id'] for i in (a,b,c))))
                found[key]=found.get(key,0)+1
    return found,positions

def replay_face_splits(roots,split_records,positions):
    """Return predicted descendant faces/depth and the root faces touched by each split."""
    leaves={root:[(root[1],0)] for root in roots};affected=[];coords_exact=[];missing=0
    for record in split_records:
        edge=tuple(sorted(map(int,record['edge'])));point=record.get('point')
        point_id=int(point[0]) if point else None;hit=[]
        for root,current in leaves.items():
            replacements=[]
            for face,depth in current:
                if not set(edge).issubset(face):
                    replacements.append((face,depth));continue
                rest=[node for node in face if node not in edge]
                if len(rest)!=1 or point_id is None or point_id in face:
                    missing+=1;replacements.append((face,depth));continue
                hit.append(root)
                replacements.extend(((tuple(sorted((edge[0],point_id,rest[0]))),depth+1),
                    (tuple(sorted((point_id,edge[1],rest[0]))),depth+1)))
            leaves[root]=replacements
        affected.append(sorted(set(hit)))
        original_edge=tuple(map(int,record['edge']))
        a,b=record.get('a'),record.get('b');exact=bool(record.get('endpoints_known') and a is not None and b is not None)
        for node_id,coord in zip(original_edge,(a,b)):
            if node_id in positions:
                exact=exact and coord is not None and tuple(positions[node_id])==tuple(coord)
            else:exact=False
        coords_exact.append(exact)
        if point_id is not None:
            if point_id in positions:missing+=1
            else:positions[point_id]=point[1:4]
    return leaves,affected,coords_exact,missing

def _self_check_lineage():
    root=(9,(1,2,3));positions={1:(0.,0.,0.),2:(1.,0.,0.),3:(0.,1.,0.)}
    records=[{'edge':[1,2],'point':[4,.5,0.,0.],'endpoints_known':True,'a':[0.,0.,0.],'b':[1.,0.,0.]},
        {'edge':[1,4],'point':[5,.25,0.,0.],'endpoints_known':True,'a':[0.,0.,0.],'b':[.5,0.,0.]}]
    leaves,touched,exact,missing=replay_face_splits([root],records,positions)
    assert {depth for _,depth in leaves[root]}=={1,2} and all(touched) and all(exact) and missing==0
    assert len(leaves[root])==3
    absent,absent_hits,_,miss=replay_face_splits([(9,(7,8,9))],records,{})
    assert miss==0 and not any(absent_hits) and absent[(9,(7,8,9))]==[((7,8,9),0)]
    predicted={(9,leaf) for leaf,_ in leaves[(9,(1,2,3))]}
    assert predicted!={(9,(1,2,3))}  # A missing child face cannot pass exact output coverage.
_self_check_lineage()

expected={}
for row in rows(Path(source['owned']['path']),416):
    identity=struct.unpack_from('<Q',row,8)[0];assert identity not in expected
    expected[identity]=hashlib.sha256(row[8:144]+row[224:416]+b'\xff'*16).digest()
assert len(expected)==source['current_cells']
donor_ids=set();cell_ids=set();nodes={}
for row in rows(files['donors'],344):
    identity=struct.unpack_from('<Q',row)[0]
    assert identity not in donor_ids and expected[identity]==hashlib.sha256(row).digest()
    donor_ids.add(identity)
assert donor_ids==set(expected) and len(donor_ids)==result['donor_cells']
for cell in _read_owned(files['owned'],m['working_rows'],12<<30):
    assert cell['id'] not in cell_ids and cell['sensor_present'];cell_ids.add(cell['id'])
    for i,node in enumerate(cell['nodes']):
        value=(node['xyz'],cell['sensor'][6*i:6*i+6])
        if node['id'] in nodes:assert nodes[node['id']]==value
        else:nodes[node['id']]=value
assert len(cell_ids)==result['output_cells']==result['positive_cells']==result['sensor_cells']
assert len(nodes)>0
assert result['global_output_cells']==len(cell_ids)

# The stage observer reports support envelopes. It does not serialize exact face ancestry.
raw_path=case/'nose_target_observation.jsonl'
raw=[json.loads(line) for line in raw_path.read_text().splitlines() if line.strip()]
fixed_rows=[row for row in raw if row['stage']=='fixed_face']
summaries=[row for row in raw if row['stage']=='fixed_summary']
probes=[row for row in raw if row['stage']=='principal_probe']
accepted=[row for row in raw if row['stage']=='accepted_split']
refusals=[row for row in raw if row['stage']=='refusal']
split_summary=next(row for row in raw if row['stage']=='split_summary')
assert split_summary['accepted_records']==len(accepted)==result['splits']
assert split_summary['accepted_stats']==result['splits']
precision_values={row['precision'] for row in fixed_rows}
same_precision=bool(fixed_rows) and precision_values=={1e-4/16}
expected_masks={1:fixed['counts']['1'],2:fixed['counts']['2'],4:fixed['counts']['4']}
by_side={}
for side in ('before','after'):
    sr=next((row for row in summaries if row['side']==side),None)
    fs=[row for row in fixed_rows if row['side']==side]
    groups={}
    for bit in (1,2,4):
        rows_for_bit=[row for row in fs if row['mask']&bit]
        complete_rows=[row for row in rows_for_bit if row['scope_complete']]
        complete=bool(sr and sr['complete'] and sr['missing_authority_facets']==0)
        groups[str(bit)]={'observed_support_faces':len(rows_for_bit),
            'maximum_complete_scope_lower':max((r['lower'] for r in complete_rows),default=None),
            'maximum_complete_scope_upper':max((r['upper'] for r in complete_rows),default=None),
            'complete_scope_faces':len(complete_rows),'incomplete_scope_faces':len(rows_for_bit)-len(complete_rows),
            'all_scopes_complete':bool(rows_for_bit) and len(complete_rows)==len(rows_for_bit),
            'observer_complete':complete,'initial_fixed_face_count':expected_masks[bit],
            'exact_original_face_descendants_proven':False,'two_geometric_halvings_proven':False}
    by_side[side]={'summary':sr,'support_faces':len(fs),'groups':groups}

# Reconstruct exact descendant topology only when the saved stats prove this was a split-only edit.
facet_masks={}
for face in fixed['faces']:
    facet=int(face['facet']);facet_masks[facet]=facet_masks.get(facet,0)|int(face['mask'])
assert len(facet_masks)==fixed['supports']
input_faces,input_positions=physical_faces(Path(source['owned']['path']),source['current_cells'],facet_masks,12<<30)
output_faces,output_positions=physical_faces(files['owned'],m['working_rows'],facet_masks,12<<30)
assert all(count==1 for count in input_faces.values()) and all(count==1 for count in output_faces.values())
def observed_face_map(side):
    result={}
    for row in fixed_rows:
        if row['side']!=side:continue
        key=(int(row['facet']),tuple(sorted(map(int,row['face']))))
        assert key not in result
        result[key]=row
    return result
before_observed=observed_face_map('before');after_observed=observed_face_map('after')
split_only=(result['moved']==0 and result['reconnected']==0 and result['collapses']==0 and
    result['commits']==result['splits']==len(accepted))
roots=list(input_faces)
leaves,affected_roots,coordinate_exact,lineage_errors=replay_face_splits(roots,accepted,dict(input_positions))
predicted_faces={}
for root,descendants in leaves.items():
    for face,depth in descendants:
        key=(root[0],face);predicted_faces[key]=depth
expected_output={key:1 for key in predicted_faces}
pre_coverage_exact=set(before_observed)==set(input_faces)
after_coverage_exact=set(after_observed)==set(predicted_faces)
output_topology_exact=output_faces==expected_output
record_coordinates_exact=all(coordinate_exact[i] for i,roots_hit in enumerate(affected_roots) if roots_hit)
split_points_exact=all(int(split['point'][0]) in output_positions and
    tuple(output_positions[int(split['point'][0])])==tuple(split['point'][1:4])
    for i,split in enumerate(accepted) if affected_roots[i] and split.get('point'))
no_compound_edits_proven=(split_only and lineage_errors==0 and output_topology_exact and
    record_coordinates_exact and split_points_exact)
summary_by_side={row['side']:row for row in summaries}
observation_coverage_complete=all(summary_by_side.get(side,{}).get('complete') is True and
    all(row['scope_complete'] for row in rows_for_side)
    for side,rows_for_side in (('before',list(before_observed.values())),('after',list(after_observed.values()))))

# Resolve each frozen source face to its exact replayed leaf faces and their certificates.
selected_faces={}
for face in fixed['faces']:
    root=(int(face['facet']),tuple(sorted(int(n['id']) for n in face['nodes'])))
    assert root not in selected_faces
    selected_faces[root]=int(face['mask'])
face_lineage=[];face_status={1:[],2:[]};tolerance=1e-4
for root,mask in selected_faces.items():
    pre=before_observed.get(root);desc=[]
    for face,depth in leaves.get(root,[]):
        key=(root[0],face);obs=after_observed.get(key)
        desc.append({'vertices':list(face),'depth':depth,'physical_output_present':key in output_faces,
            'observation_present':obs is not None,'scope_complete':bool(obs and obs['scope_complete']),
            'lower':obs['lower'] if obs else None,'upper':obs['upper'] if obs else None})
    exact=bool(no_compound_edits_proven and pre and pre['scope_complete'] and desc and
        all(d['physical_output_present'] and d['observation_present'] and d['scope_complete'] for d in desc))
    max_depth=max((d['depth'] for d in desc),default=None)
    uppers=[d['upper'] for d in desc if d['upper'] is not None]
    lowers=[d['lower'] for d in desc if d['lower'] is not None]
    pre_upper=pre['upper'] if pre else None
    within_two=bool(exact and max_depth is not None and max_depth<=2 and len(uppers)==len(desc))
    if within_two and max(uppers)<=tolerance:
        decision='support'
    else:decision='unresolved'
    entry={'facet':root[0],'original_face':list(root[1]),'mask':mask,'lineage_proven':exact,
        'pre_bound':{'lower':pre['lower'],'upper':pre['upper'],'scope_complete':pre['scope_complete']} if pre else None,
        'descendants':desc,'maximum_depth':max_depth,'max_descendant_lower':max(lowers,default=None),
        'max_descendant_upper':max(uppers,default=None),'certified_upper_improved':bool(pre_upper is not None and uppers and max(uppers)<pre_upper),
        'within_two_halvings':within_two,'decision':decision}
    face_lineage.append(entry)
    if mask&1:face_status[1].append(decision)
    if mask&2:face_status[2].append(decision)
patch_gates={}
for bit,name in ((1,'tip'),(2,'root')):
    states=face_status[bit]
    decision='reject' if 'reject' in states else 'support' if states and all(x=='support' for x in states) else 'unresolved'
    patch_gates[name]={'status':decision,'original_faces':len(states),'faces':[
        row for row in face_lineage if row['mask']&bit],
        'lineage_and_observation_coverage_proven':no_compound_edits_proven and bool(states) and all(row['lineage_proven'] for row in face_lineage if row['mask']&bit),
        'maximum_descendant_depth_le_2_proven':bool(states) and all(
            row['lineage_proven'] and row['maximum_depth'] is not None and row['maximum_depth']<=2
            for row in face_lineage if row['mask']&bit)}
    by_side['after']['groups'][str(bit)].update(
        exact_original_face_descendants_proven=patch_gates[name]['lineage_and_observation_coverage_proven'],
        within_two_halvings_proven=patch_gates[name]['maximum_descendant_depth_le_2_proven'],
        patch_decision=decision)

# Direction uses the actual replayed ancestor face, not nearest-face attribution.
probe_by_facet={}
for row in probes:
    if row['side']=='before':probe_by_facet.setdefault(row['facet'],row)
direction_counts={'around_nose':0,'spanwise':0,'ambiguous':0,'unmapped_or_unknown':0}
direction_rows=[]
for index,split in enumerate(accepted):
    contexts=[root for root in affected_roots[index] if root in selected_faces and selected_faces[root]&7]
    if not contexts:continue
    edge=tuple(sorted(map(int,split['edge'])))
    classes=set()
    for root in contexts:
        probe=probe_by_facet.get(root[0])
        if not no_compound_edits_proven or not probe or not coordinate_exact[index]:continue
        curv=probe['curvatures'];dirs=probe['directions']
        dominant=max(range(2),key=lambda i:abs(curv[i]));other=1-dominant
        a,b=split.get('a'),split.get('b')
        if not split.get('endpoints_known') or a is None or b is None:continue
        v=[b[k]-a[k] for k in range(3)];length=math.sqrt(sum(x*x for x in v))
        if not length:continue
        projection=[abs(sum(v[k]*dirs[i][k] for k in range(3)))/length for i in range(2)]
        delta=projection[dominant]-projection[other]
        classes.add('around_nose' if delta>1e-12 else 'spanwise' if delta < -1e-12 else 'ambiguous')
    label=next(iter(classes)) if len(classes)==1 else ('ambiguous' if classes else 'unmapped_or_unknown')
    direction_counts[label]+=1
    direction_rows.append({'edge':list(edge),'a':split.get('a'),'b':split.get('b'),'point':split.get('point'),
        'endpoints_known':split.get('endpoints_known'),'label':label,'ancestor_faces':[list(root[1]) for root in contexts],
        'classification_scope':'Exact split-only reconstructed ancestry; endpoint coordinates checked against saved positions.'})

direction_complete=direction_counts['unmapped_or_unknown']==0
classified=direction_counts['around_nose']+direction_counts['spanwise']+direction_counts['ambiguous']
if direction_complete and classified and direction_counts['around_nose']>classified/2:
    direction_gate='support'
elif direction_complete and classified and direction_counts['spanwise']>classified/2:
    direction_gate='reject'
else:direction_gate='unresolved'

if direction_gate=='reject' or any(patch_gates[name]['status']=='reject' for name in ('tip','root')):
    overall_gate='reject'
elif (result['splits']>=250 and direction_gate=='support' and
      all(patch_gates[name]['status']=='support' for name in ('tip','root'))):
    overall_gate='unresolved'  # Logical split-trial refusal fraction has no serialized denominator.
else:overall_gate='unresolved'

probe_checks=[]
for probe in probes:
    for i,curvature in enumerate(probe['curvatures']):
        if curvature==0:continue
        radius=1/abs(curvature);e=min(1e-4,radius)
        cap=(1.8/(2*math.sqrt(e*(2*radius-e))))**2
        demand=probe['directional_demands'][i]
        assert demand>=cap*(1-1e-8)
        probe_checks.append({'side':probe['side'],'facet':probe['facet'],'principal':i,
            'curvature':curvature,'metric_demand':demand,'required_cap':cap,'cap_preserved':True})

refusal_counts={}
for row in refusals:
    key=f"failure={row.get('failure')}; candidate={row.get('candidate')}; reason={row.get('reason')}"
    refusal_counts[key]=refusal_counts.get(key,0)+1
observation={
 'case':str(case),'raw_observation':{'path':str(raw_path),'sha256':sha(raw_path)},
 'fixed_set':{'path':str(R/'nose492_fixed_faces.json'),'sha256':sha(R/'nose492_fixed_faces.json'),
    'authority_facets':fixed['supports'],'counts':fixed['counts'],'selection':fixed['selection']},
 'coverage':by_side,
 'lineage_reconstruction':{'split_only_counters':{'moved':result['moved'],'reconnected':result['reconnected'],
    'collapses':result['collapses'],'commits':result['commits'],'splits':result['splits'],
    'published_records':len(accepted)},'split_only_gate':split_only,'lineage_errors':lineage_errors,
    'input_support_faces':len(input_faces),'before_observation_faces':len(before_observed),
    'predicted_support_leaves':len(predicted_faces),'after_observation_faces':len(after_observed),
    'output_support_faces':len(output_faces),'before_coverage_exact':pre_coverage_exact,
    'after_coverage_exact':after_coverage_exact,'output_topology_exact':output_topology_exact,
    'tracked_endpoint_coordinates_exact':record_coordinates_exact,'tracked_split_point_coordinates_exact':split_points_exact,
    'no_compound_edits_proven':no_compound_edits_proven,'observation_coverage_complete':observation_coverage_complete},
 'bound_method':'Mapped complete-authority BoundPhysicalSurface/ErrorMagnitude with full-entity support audit.',
 'precision':{'values':sorted(precision_values),'same_before_after':same_precision,'requested':1e-4/16},
 'published_split_records':len(accepted),'accepted_split_stat':result['splits'],
 'direction_classification':{'counts':direction_counts,'gate':direction_gate,'complete_for_all_nose_split_records':direction_complete,
    'nose_split_records':len(direction_rows),'records':direction_rows,
    'method':'Ambient edge direction projected onto the mapped authority principal directions for splits assigned to a selected original face by exact split-only lineage.'},
 'rejections':{'callback_records':len(refusals),'callback_reasons':refusal_counts,
    'geometry_guidance_trials':result['geometry_guidance_trials'],
    'geometry_guidance_rejected':result['geometry_guidance_rejected'],
    'guidance_rejection_fraction':(result['geometry_guidance_rejected']/result['geometry_guidance_trials'] if result['geometry_guidance_trials'] else None),
    'logical_split_trial_denominator_available':False,
    'note':'Rejected-trial callbacks, geometry-guidance counters and logical split trials are distinct; no logical-trial refusal fraction inferred.'},
 'principal_probes':probes,'principal_cap_checks':probe_checks,
 'patch_decision':{'tip':patch_gates['tip'],'root':patch_gates['root'],'overall':overall_gate,
    'reason':'Split-only replay proves descendants only when edit counters, input/output physical faces, published split records and fixed observations agree exactly. Otherwise the affected criterion remains unresolved.',
    'reject_requires':'A certified lower bound above tolerance after two geometric curved-direction halvings; topological depth alone is insufficient and does not trigger rejection here.',
    'support_requires':'Complete fixed-face descendant coverage, two halvings, every required upper bound at or below tolerance, and all other plan gates.',
    'plan_gates':{'logical_refusal_fraction':'unresolved; exact logical trial denominator unavailable',
      'accepted_splits':{'observed':result['splits'],'support_threshold_met':result['splits']>=250},
      'split_direction':direction_gate,'tip_patch':patch_gates['tip']['status'],
      'root_patch':patch_gates['root']['status'],'overall':overall_gate}},
 'bad_face_counts_used_as_progress':False}
(R/'nose_target_observation492_assessment.json').write_text(json.dumps(observation,indent=2)+'\n')

assessment={'case':str(case),'status':status,'pins_verified':len(pins),'actual_handles_terminal':True,
 'input_donors_exact':len(donor_ids),'input_cells':source['current_cells'],'output_cells':len(cell_ids),'output_nodes':len(nodes),
 'all_output_cells_positive_and_sensor_present':True,'same_target_policy':result['same_target_policy'],
 'input_parameters_unchanged':result['input_parameters_unchanged'],'surface_ready':result['surface_prepared'],
 'final_mesh_accepted':False,'goal_complete':False,'result':result,
 'files':{k:{'path':str(p.resolve()),'sha256':sha(p),'bytes':p.stat().st_size} for k,p in files.items()},
 'fixed_nose_observation_assessment':str(R/'nose_target_observation492_assessment.json'),
 'limitations':['Provisional validity-first surface output, not accepted mesh or solver restart.',
 'No first row, strict closure, final quality/height gates, transfer or CFD continuation established.',
 'Exact nose-face lineage is credited only when split-only counters, replayed descendants, output faces and observation rows all agree.',
 'Rejection callbacks are not logical split-trial counts; guidance counters are reported separately.',
 'No progress inferred from changing bad-face counts.']}
checkpoint={'provisional':True,'final_mesh_accepted':False,'solver_restart':False,'exact_scheduling_resume':False,
 'source_case':str(case),'owned':{'path':str(files['owned'].resolve()),'sha256':sha(files['owned']),'bytes':files['owned'].stat().st_size},
 'parameters':{'path':str(files['parameters'].resolve()),'sha256':sha(files['parameters']),'bytes':files['parameters'].stat().st_size},
 'donors':{'path':str(files['donors'].resolve()),'sha256':sha(files['donors']),'bytes':files['donors'].stat().st_size},
 'composed_target':{'path':str((producer/'target_rank0.bin').resolve()),'sha256':sha(producer/'target_rank0.bin')},
 'all_rank_targets':[{'path':str(producer/f'target_rank{rank}.bin'),
    'sha256':producer_receipt['files'][f'target_rank{rank}.bin']['sha256'],
    'reference_bytes':source['prepared_reference']['bytes'],'same_reference':True} for rank in range(6)],'target_qualification':{'path':str(qualification),'sha256':sha(qualification)},
 'target_producer_receipt':{'path':str(R/'target_checkpoint492_producer.json'),
    'sha256':sha(R/'target_checkpoint492_producer.json'),'pins':str(producer/'pins.json'),
    'pins_sha256':sha(producer/'pins.json')},
 'metric_stage':json.loads((producer/'saved_cfd_metric_rank0.jsonl').read_text().splitlines()[0]),
 'prepared_reference':{'path':str(R/'prepared_reference441.native_ref'),'sha256':sha(R/'prepared_reference441.native_ref'),
    'bytes':(R/'prepared_reference441.native_ref').stat().st_size},
 'input_checkpoint':{'path':str(R/'saved_surface485_checkpoint.json'),'sha256':sha(R/'saved_surface485_checkpoint.json')},
 'input_current_state':{'owned_sha256':source['owned']['sha256'],'parameters_sha256':source['parameters']['sha256'],
    'cells':source['current_cells'],'nodes':source['current_nodes']},
 'current_cells':len(cell_ids),'current_nodes':len(nodes),'stage_height':result['stage_height'],
 'stage_event':0,'final_height':source['final_height'],'growth':source['growth'],'thickness':source['thickness'],
 'current_nodal_tensors':'Current authoritative transported sensors in Owned; complete new target and authority retained separately.',
 'configuration':{'path':str(seed/'run.cfg'),'sha256':sha(seed/'run.cfg')},
 'build_receipt':{'path':str(R/'build494_terminal_handles.json'),'sha256':sha(R/'build494_terminal_handles.json')},
 'runtime_pins':{'path':str(R/'runtime_20261113_474/loaded_runtime_pins.json'),
    'sha256':sha(R/'runtime_20261113_474/loaded_runtime_pins.json')},
 'saved_seed':{'path':str(seed),'provenance_sha256':sha(seed/'provenance.json'),
    'qualification_sha256':sha(seed/'qualification.json')},
 'fixed_faces':{'path':str(R/'nose492_fixed_faces.json'),'sha256':sha(R/'nose492_fixed_faces.json')},
 'nose_observation':{'path':str(R/'nose_target_observation492_assessment.json'),'sha256':sha(R/'nose_target_observation492_assessment.json')},
 'provenance':{'case_pins':str(case/'pins.json'),'case_pins_sha256':sha(case/'pins.json'),
    'case_status':str(case/'status.json'),'case_status_sha256':sha(case/'status.json'),
    'actual_handles':str(case/'terminal_handles_actual.json'),'actual_handles_sha256':sha(case/'terminal_handles_actual.json'),
    'resource_receipt':str(case/'resources.jsonl'),'resource_receipt_sha256':sha(case/'resources.jsonl'),
    'target_producer_receipt':str(R/'target_checkpoint492_producer.json'),'target_producer_receipt_sha256':sha(R/'target_checkpoint492_producer.json'),
    'target_qualification':str(qualification),'target_qualification_sha256':sha(qualification)},
 'acceptance_limits':['Provisional surface stage only; final mesh acceptance remains false.',
    'Face lineage and within-two-halving evidence are conditional on exact split-only replay and output/observation agreement.',
    'Logical split-trial refusal fraction unavailable from callback records; no denominator inferred.',
    'No first-row, growth, transition, strict closure, quality/height acceptance or CFD restart.']}
(R/'saved_surface492_checkpoint.json').write_text(json.dumps(checkpoint,indent=2)+'\n')
assessment['checkpoint']={'path':str(R/'saved_surface492_checkpoint.json'),'sha256':sha(R/'saved_surface492_checkpoint.json')}
(R/'saved_production_surface492_serial_assessment.json').write_text(json.dumps(assessment,indent=2)+'\n')
print(json.dumps({'output_cells':len(cell_ids),'output_nodes':len(nodes),'surface_ready':result['surface_prepared'],
 'final_mesh_accepted':False,'nose_decision':overall_gate,'assessment':str(R/'saved_production_surface492_serial_assessment.json')},indent=2))
