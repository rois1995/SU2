"""Compare emitted native boundaries and exit counters; missing events stay missing."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parent
PHASES=['collapse_setup','surface_setup','interior','repartition_setup','quality','alternation_setup','finish']
def summarize(path):
    data=json.loads(path.read_text());ranks=data['ranks'];assert set(ranks)==set(map(str,range(6)))
    output=[]
    count=min(len(v['events']) for v in ranks.values())
    for index in range(count):
        events=[ranks[str(r)]['events'][index] for r in range(6)]
        if not all(e['exit_marker_present'] for e in events):continue
        phases={}
        for name in PHASES:
            bins=[e['bins'].get(name) for e in events]
            if not any(bins):continue
            assert all(bins),(index,name,'inconsistent phase presence')
            calls=[b['counters'].get('election_calls',0) for b in bins]
            assert len(set(calls))==1,(name,calls)
            edits=[b['counters'].get('accepted',0) for b in bins];assert min(edits)>=0
            phases[name]={'max_rank_boundary_seconds':max(b['seconds'] for b in bins),'global_accepted_edits':sum(edits),'world_election_calls_common_per_rank':calls[0],'world_election_seconds_rank0':bins[0]['counters'].get('election_seconds',0),'world_election_mean_us_rank0':bins[0]['counters'].get('election_seconds',0)/calls[0]*1e6 if calls[0] else None}
        rows=[]
        for rank,event in enumerate(events):
            stage=event['stages'][-1];details=ranks[str(rank)]['refine_details'];assert len(details)>index
            detail=details[index]
            rows.append({'rank':rank,'native_inner_seconds':float(detail['total']),'complete':detail['complete']=='1','stop':detail.get('stop'),'upper_seconds':float(stage['upper_seconds']),'upper_calls':int(stage['upper_calls']),'upper_work':int(stage['upper_work']),'world_election_seconds':float(stage['election_seconds']),'world_election_calls':int(stage['election_calls']),'update_seconds':float(stage['update_seconds']),'geometry_trials':int(stage['geometry_trials']),'geometry_rejected':int(stage['geometry_rejected']),'other_seconds':float(detail['other']),'directory_election_seconds':float(detail['nested_directory_election']),'directory_election_calls':int(detail['directory_elections']),'final_private_tets':int(detail['final_owned_cells']),'surface_lane_seconds':float(stage['surface_seconds']),'interior_lane_seconds':float(stage['interior_seconds'])})
        for rank,event in enumerate(events):
            total=sum(b['counters'].get('accepted',0) for b in event['bins'].values());assert total==float(event['stages'][-1]['accepted'])
        output.append({'invocation':index+1,'phases':phases,'ranks':rows})
    return output
output={'reference':summarize(ROOT/'reference293_markers.json'),'current':summarize(ROOT/'current_markers.json'),'semantics':'Phase wall=max rank disjoint marker-bin sum; edits=sum rank deltas; World election count common per rank, mean uses rank0. Named lane counters have a different scope. Directory election family separate. Final finish is unresolved if not separately emitted.'}
(ROOT/'comparison_summary.json').write_text(json.dumps(output,indent=2)+'\n')
print('Complete events:',{k:len(output[k]) for k in ['reference','current']})
for k in ['reference','current']:
    for e in output[k]:print(k,e['invocation'],{p:round(v['max_rank_boundary_seconds'],3) for p,v in e['phases'].items()})
