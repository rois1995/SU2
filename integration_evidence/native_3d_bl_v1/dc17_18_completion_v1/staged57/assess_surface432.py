"""Validate saved MPI surface shards and summarize work; never infer final mesh acceptance."""
import hashlib,json,re,struct,sys
from pathlib import Path
R=Path(__file__).resolve().parent
sys.path.insert(0,str(R))
from inspect_native_failure3d import _read_owned
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=R/'cases/curved432_saved_target_surface_mpi6_20261112T092307_730261'
    status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
    terminal=json.loads((case/'terminal_handles_actual.json').read_text());assert terminal['wrapper_verified_terminal']
    pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
    rows=[];censuses=[];files={};points={};cell_ids=set();shared=0;physical=0
    for rank in range(6):
        path=case/f'saved_target_surface_rank{rank}.jsonl';d=json.loads(path.read_text());rows.append(d)
        assert d['rank']==rank and d['ranks']==6 and d['maximum_seconds']==388.8 and d['maximum_work']==233220
        assert not d['cfd_advanced'] and not d['metric_recomputed'] and d['target_policy_unchanged']
        assert d['positive_cells']==d['local_tets']==d['transported_core_cells']
        manifest=case/f'stage_surface_rank{rank}_manifest.json';m=json.loads(manifest.read_text());censuses.append(m['progress'])
        assert m['complete_shard'] and m['rank']==rank and m['working_rows']==d['local_tets']
        assert m['parameters_present'] and m['owned_record_bytes']==416 and m['donor_record_bytes']==344
        assert m['progress']['cache_consistent'] and not m['final_mesh_accepted']
        for kind,count_key,record in [('owned','working_rows',416),('donors','donor_rows',344)]:
            payload=case/m['files'][kind]
            with payload.open('rb') as stream: count=struct.unpack('<Q',stream.read(8))[0]
            assert count==m[count_key] and payload.stat().st_size==8+record*count
            files[str(payload)]={'bytes':payload.stat().st_size,'sha256':sha(payload)}
        parameters=case/m['files']['parameters'];assert parameters.stat().st_size==m['parameters_payload_bytes']
        files[str(parameters)]={'bytes':parameters.stat().st_size,'sha256':sha(parameters)}
        files[str(path)]={'bytes':path.stat().st_size,'sha256':sha(path)}
        files[str(manifest)]={'bytes':manifest.stat().st_size,'sha256':sha(manifest)}
        for cell in _read_owned(case/m['files']['owned'],m['working_rows'],128<<20):
            assert cell['id'] not in cell_ids;cell_ids.add(cell['id']);assert cell['sensor_present']
            physical+=any(x['present'] for x in cell['physical'])
            for k,node in enumerate(cell['nodes']):
                value=struct.pack('<9d',*(node['xyz']+cell['sensor'][6*k:6*k+6]))
                prior=points.setdefault(node['id'],value);assert prior==value
                shared+=prior is not value
    assert len(cell_ids)==sum(x['local_tets'] for x in rows)
    def total(key):return sum(x[key] for x in rows)
    geometry=[x['geometry_work'] for x in rows]
    resources=[json.loads(x) for x in (case/'resources.jsonl').read_text().splitlines()]
    before=json.loads((R/'surface_stage428_assessment.json').read_text())['stage']
    aggregate=dict(nodes=len(points),tets=len(cell_ids),positive_tets=total('positive_cells'),physical_cells=physical,
        exact_shared_nodal_checks=shared,commits=total('local_commits'),global_work=rows[0]['global_work'],
        cached_cells=sum(x['measured_cells'] for x in censuses),unmeasured_cells=sum(x['unmeasured_cells'] for x in censuses),
        known_geometry_bad=sum(x['geometry_bad_cells'] for x in censuses),known_size_bad=sum(x['size_bad_cells'] for x in censuses),
        q_measured=sum(x['q_measured_cells'] for x in censuses),J_measured=sum(x['j_measured_cells'] for x in censuses),
        geometry_work=total('geometry_work'),geometry_rank_max_min=max(geometry)/min(geometry) if min(geometry)>0 else None,
        field_requests=total('field_requests'),field_compositions=total('field_compositions'),
        summed_rank_cpu_seconds=total('process_cpu_seconds'),stage_seconds_max=max(x['stage_seconds'] for x in rows),
        target_load_seconds_max=max(x['load_seconds'] for x in rows),import_seconds_max=max(x['import_seconds'] for x in rows),
        largest_sampled_tree_RSS_bytes=max(x['sampled_process_tree_rss_bytes'] for x in resources),
        largest_sampled_individual_RSS_bytes=status['largest_sampled_individual_process_rss_bytes'])
    counters=[];counter_gaps=[]
    for line in (case/'run.log').read_text().splitlines():
        if not line.startswith('NATIVE3D_STAGE rank=') or ' stage=exit ' not in line:continue
        if line.count('NATIVE3D_STAGE rank=')!=1:
            counter_gaps.append('Interleaved exit line: counters cannot be assigned safely to a rank.');continue
        values=dict(re.findall(r'(\w+)=([^ ]+)',line))
        keys=['rank','rounds','updated_cells','update_seconds','election_calls','election_seconds',
            'geometry_trials','geometry_rejected','guidance_trials','guidance_rejected','guidance_seconds',
            'upper_calls','upper_work','upper_seconds','scope_misses','scope_build_seconds',
            'facet_build_seconds','traversal_seconds','retention_refusals','cache_evictions']
        available={}
        for key in keys:
            try:available[key]=float(values[key])
            except (KeyError,ValueError):counter_gaps.append('Unavailable exit counter '+key+' on prefix rank '+values.get('rank','unknown'))
        counters.append(available)
    assert aggregate['q_measured']==aggregate['J_measured']==0
    aggregate.update(surface_prepared=all(x['surface_prepared'] for x in rows),
        required_wall_cells=total('surface_demand_required'),measured_wall_cells=total('surface_demand_measured'),
        requested_wall_census_complete=all(x['surface_demand_complete'] for x in rows),
        requested_wall_geometry_complete=all(x['surface_geometry_complete'] for x in rows),
        seed_cell_classes=[sum(x['seed_cell_classes'][i] for x in rows) for i in range(3)],
        updated_cell_classes=[sum(x['updated_cell_classes'][i] for x in rows) for i in range(3)],
        collapse_passes=[x['collapse_passes'] for x in rows],split_passes=[x['split_passes'] for x in rows])
    assert aggregate['seed_cell_classes'][:2]==[0,0]
    if aggregate['surface_prepared']:
        assert aggregate['requested_wall_census_complete'] and aggregate['requested_wall_geometry_complete']
    baseline_path=R/'surface_stage431_assessment.json';baseline=json.loads(baseline_path.read_text())['aggregate']
    compared=['commits','global_work','cached_cells','unmeasured_cells','known_geometry_bad','known_size_bad',
        'geometry_work','geometry_rank_max_min','field_requests','summed_rank_cpu_seconds','stage_seconds_max']
    report=dict(scope='Actual M6 requested-wall surface-demand census; provisional construction, not final BL acceptance.',
        case=str(case),status=status,all_case_pins_verified=len(pins),all_actual_handles_terminal=True,
        aggregate=aggregate,ranks=rows,censuses=censuses,existing_exit_counters=counters,counter_attribution_gaps=counter_gaps,payloads=files,
        comparison431={k:dict(before=baseline[k],after=aggregate[k]) for k in compared},
        baseline_assessment_sha256=sha(baseline_path),
        confirmed=['Same saved396 composed target and CFD seed, original six partitions; no target recomputation or CFD advancement.',
            'Every saved cell positive with transported core; globally unique cell IDs and exact shared nodal coordinates/tensors.',
            'No unrelated volume rows seeded; complete changed-cell rows remain measured. Full-volume unknown flags remain explicit.'],
        limits=['No q/J or complete BL suitability claim. Known geometry/size counts are partial if their generic coverage remains incomplete.',
            'Changed coverage/ownership/operation trajectory; no byte-identical mesh expectation or baseline CPU-dominance attribution.',
            'CPU can include MPI polling; nested/query/collective timers overlap and are not total wait. Sampled RSS is not true physical peak.',
            'This is nativeMPI surface preparation, not nativeMPI FIRST_ROW/layer generation. Failure captures are not credible reusable successful stages.'],
        assessment_script_sha256=sha(__file__),goal_complete=False)
    (R/'surface_stage432_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(aggregate,indent=2));print(json.dumps(report['comparison431'],indent=2))
if __name__=='__main__':main()
