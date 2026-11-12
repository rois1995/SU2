"""Validate saved MPI surface shards and summarize work; never infer final mesh acceptance."""
import hashlib,json,re,struct,sys
from pathlib import Path
R=Path(__file__).resolve().parent
sys.path.insert(0,str(R))
from inspect_native_failure3d import _read_owned
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    case=R/'cases/curved431_saved_target_surface_mpi6_20261112T080701_534595'
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
        geometry_work=total('geometry_work'),geometry_rank_max_min=max(geometry)/min(geometry),
        field_requests=total('field_requests'),field_compositions=total('field_compositions'),
        summed_rank_cpu_seconds=total('process_cpu_seconds'),stage_seconds_max=max(x['stage_seconds'] for x in rows),
        target_load_seconds_max=max(x['load_seconds'] for x in rows),import_seconds_max=max(x['import_seconds'] for x in rows),
        largest_sampled_tree_RSS_bytes=max(x['sampled_process_tree_rss_bytes'] for x in resources),
        largest_sampled_individual_RSS_bytes=status['largest_sampled_individual_process_rss_bytes'])
    counters=[]
    for line in (case/'run.log').read_text().splitlines():
        if not line.startswith('NATIVE3D_STAGE rank=') or ' stage=exit ' not in line:continue
        values=dict(re.findall(r'(\w+)=([^ ]+)',line))
        keys=['rank','rounds','updated_cells','update_seconds','election_calls','election_seconds',
            'geometry_trials','geometry_rejected','guidance_trials','guidance_rejected','guidance_seconds',
            'upper_calls','upper_work','upper_seconds','scope_misses','scope_build_seconds',
            'facet_build_seconds','traversal_seconds','retention_refusals','cache_evictions']
        counters.append({key:float(values[key]) for key in keys})
    assert len(counters)==6
    report=dict(scope='Actual nativeMPI surface-stage capacity from qualified396/original6partitions; NOT usable BL or accepted mesh.',
        case=str(case),status=status,all_case_pins_verified=len(pins),all_actual_handles_terminal=True,
        aggregate=aggregate,ranks=rows,censuses=censuses,existing_exit_counters=counters,payloads=files,
        comparison428=dict(serial_commits=before['commits'],mpi_commits=aggregate['commits'],
            commit_ratio=aggregate['commits']/before['commits'],serial_cached=before['cached_measured_cells'],mpi_cached=aggregate['cached_cells'],
            serial_work=before['work'],mpi_work=aggregate['global_work'],
            serial_cpu_seconds=before['process_cpu_seconds'],mpi_summed_cpu_seconds=aggregate['summed_rank_cpu_seconds']),
        confirmed=['All six original target layouts restored; CFD unchanged, no metric recomputation or target-profile change.',
            'All output cells positive with transported core; unique global cellIDs and bitexact shared nodal coordinates/tensors.',
            'Neither serial428 nor MPI431 completes initial coarsening/census before refinement at same388.8s budget.',
            'More native capacity gives only modest extra useful coarsening; no accepted output or throughput scaling claim.'],
        source_audit=['seed_next advances <=16 unmeasured owned IDs/rank, ordinarily after global queues drain (Engine2682-2699,3108-3121).',
            'A complete consistent census and no active candidate are required before coarsening switches to refinement (Engine3210-3263). Geometry queue insertion requires !coarsening (846).',
            'All owned cells need six-edge cached rows for the current census contract, although surface action eligibility is requested-wall-only. Simply omitting interior rows or neutralizing shared scalars is unsafe without an adapted contract.',
            'Exit remaining_edges=0 does not establish an idle final round: stop was a geometry/private-work deadline. Seed22.543s/rank does not prove interior seeding dominated389s. Updated cells are changed-cell measurements, not repeated global scans.'],
        limits=['All q/J and heights remain unmeasured; known geometry/size counts are partial and not whole-field churn.',
            'Different ownership/queue trajectories; outputs not expected byte-identical. Same source/target/constraints/budgets.',
            'CPU includes possible MPI polling. Reconstruction wall and CPU cover different conditional scopes; subtraction is not total wait.',
            'Tracked collectives are partial and overlapping. Sampled RSS is not true peak physical memory.',
            'Surface execution is not first-row/layer generation or dedicated nativeMPI BL qualification.',
            'Failure capture plus external complete396 policy is retained; not a credible successful stage checkpoint or exact search continuation.'],
        next='Localize existing seed/update work by requested-wall, other boundary and interior cells, plus queue-empty/refill and remaining initial IDs, before changing the full-census/eligibility contract. Current evidence proves the barrier but not interior-work dominance; no new scheduler/operator or relaxed admission.',goal_complete=False)
    report['assessment_script_sha256']=sha(__file__)
    (R/'surface_stage431_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(aggregate,indent=2));print(json.dumps(report['comparison428'],indent=2))
if __name__=='__main__':main()
