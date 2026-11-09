"""Seal same-seed accepted first-adaptation comparison; overlapping clocks stay separate."""
from pathlib import Path
import hashlib,json,time
R=Path(__file__).resolve().parent
paths={293:R/'m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k',296:R/'m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k'}
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
results={};pins={str(Path(__file__).resolve()):sha(Path(__file__))}
for build,a in paths.items():
    status=json.loads((a/'status.json').read_text());assert status['state']=='terminal' and status['workflow_accepted'] and status['pins_verified']
    assert json.loads((a/'terminal_handles.json').read_text())['all_owned_handles_terminal']
    progress=json.loads((a/'construction_progress_assessment.json').read_text())
    entry=next(e for e in progress['events'] if e['stage']=='quality_entry')
    shape=next(e for e in progress['events'] if e['stage']=='quality_exit')
    final=progress['events'][-1];assert final['stage']=='exit' and all(final['complete_flags'].values())
    assert all(final['totals'][k]==0 for k in ('q_bad','j_bad','geometry_bad','size_bad'))
    total_cycles=2 if build==293 else 1
    log=(a/'run.log').read_text();section=log.split(f'Cycle 0 of {total_cycles}:',1)[1].split(f'Cycle 1 of {total_cycles}:',1)[0]
    printed=[dict(x.split('=',1) for x in line.split()[1:] if '=' in x) for line in section.splitlines() if line.startswith('NATIVE3D rank=')]
    assert len(printed)==6 and {int(x['rank']) for x in printed}==set(range(6))
    summary=[dict(rank=int(x['rank']),**{k:float(x[k]) for k in ('total','cpu','idle_cpu')}) for x in printed];service=[]
    for rank in range(6):
        p=a/f'rank_output/1/rank.{rank}/stdout';text=p.read_text();prefix=progress['source_prefixes'].get(str(p)) or progress['source_prefixes'][str(p.relative_to(R.parents[3]))]
        assert hashlib.sha256(p.read_bytes()[:prefix['bytes']]).hexdigest()==prefix['sha256']
        for line in text.splitlines():
            if not line.startswith('NATIVE3D_QUALITY '):continue
            row=dict(x.split('=',1) for x in line.split()[1:] if '=' in x)
            if int(row['round'])>shape['round']:break
            if row['stage'] not in ('window','exit'):continue
            assert row['complete']=='1' and int(row['rank'])==rank
            service.append(row)
    sums={k:sum(float(v[k]) for v in service) for k in ('commits','q_retired','q_introduced','j_retired','j_introduced','busy_seconds','join_call_seconds','tracked_collective_call_seconds')}
    for k,bad in [('q','q_bad'),('j','j_bad')]:assert sums[k+'_retired']-sums[k+'_introduced']==entry['totals'][bad]-shape['totals'][bad]
    log=(a/'run.log').read_text();table=log.split('|     Cycle|      Complexity|',1)[1];driver=[]
    for line in table.splitlines():
        cols=[x.strip() for x in line.split('|')[1:-1]]
        if len(cols)==6 and cols[0].isdigit():driver.append(dict(cycle=int(cols[0]),complexity=cols[1],points=int(cols[2]),iterations=int(cols[3]),solve_seconds=float(cols[4]),adapt_seconds=None if cols[5]=='-' else float(cols[5])))
    assert driver and driver[0]['cycle']==0 and driver[0]['adapt_seconds']>0
    quality_seconds=shape['maximum_native_elapsed']-entry['maximum_native_elapsed']
    closure_seconds=final['maximum_native_elapsed']-entry['maximum_native_elapsed']
    keys=('upper_calls','upper_work','traversal_work','scope_build_work','scope_misses','smooth_boundary_stars','smooth_boundary_reused','smooth_boundary_move_contexts','smooth_boundary_prepared_trials','smooth_boundary_guidance_reused_faces','smooth_boundary_seconds','scope_build_seconds','traversal_seconds')
    results[str(build)]=dict(driver_rows=driver,accepted_first_adapt_seconds=driver[0]['adapt_seconds'],native_seconds=final['maximum_native_elapsed'],quality_seconds=quality_seconds,entry_counts=entry['totals'],shape_exit_counts=shape['totals'],final_counts=final['totals'],quality_net_bad_per_second={k:(entry['totals'][k]-shape['totals'][k])/quality_seconds for k in ('q_bad','j_bad','geometry_bad')},whole_closure_net_bad_per_second={k:(entry['totals'][k]-final['totals'][k])/closure_seconds for k in ('q_bad','j_bad','geometry_bad')},first_quality_service_rank_sums=sums,native_printed_rank_cpu=summary,counters={k:sum(float(v.get(k,0)) for v in final['rank_rows'].values()) for k in keys},source_prefixes=progress['source_prefixes'])
    for name in ('status.json','terminal_handles.json','run.log','construction_progress_assessment.json'):pins[str((a/name).resolve())]=sha(a/name)
old,new=results['293'],results['296']
for k in ('upper_work','traversal_work','scope_build_work','scope_misses'):assert old['counters'][k]==new['counters'][k]
validation=json.loads((paths[296]/'workflow296_independent_validation.json').read_text())
previous=json.loads((paths[293]/'workflow293_independent_validation.json').read_text())
assert validation['events'][0]['file_hashes']==previous['events'][0]['file_hashes']
result=dict(time=time.time(),scope='Accepted complete first adaptation only; same seed/L1/50k/constraints/budgets.293 two-event and isolated restart remain regression baseline.',builds=results,accepted_first_adapt_difference_seconds=new['accepted_first_adapt_seconds']-old['accepted_first_adapt_seconds'],accepted_first_adapt_time_reduction_fraction=1-new['accepted_first_adapt_seconds']/old['accepted_first_adapt_seconds'],mesh_reference_and_continued_restart_byte_identical=True,expensive_geometry_work_reduced=False,causal_material_performance_gain_established=False,decision='Switch to the user-requested connected curved BL milestone. No further Euler optimization cycle justified by small timing differences or reuse counters.',timer_notes='All seconds in counter/rank sums overlap; never add smoothing/scope/traversal clocks. Join-call seconds are not total MPI wait. Printed CPU includes MPI spin and is not useful work by itself.',source_pins=pins)
assert all(sha(p)==h for p,h in pins.items())
(paths[296]/'accepted_first_performance_comparison293.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('builds','source_pins')}))
