"""Summarize measured rank costs without adding nested phases or per-rank maxima."""
from collections import defaultdict
import math,statistics
from audit_lifecycle import ENGINE,STAGES,closes,transport_counters

NATIVE=('support','reference','import','source','weights','partition','donor_migration',
        'communicator','refine','coarsen','wait','return')
DETAIL={'refine':ENGINE,'coarsen':ENGINE,'import':('points','cells','faces','source','query','reader'),
        'weights':('points','cells','faces','source','query','reader'),
        'return':('points','cells','faces','source','query','reader'),
        'partition':('graph','parmetis','migration'),'source':None}
STAGE=dict(STAGES,envelope=('metric','observation','remesh','replace'),
           envelope_restart=('write','teardown','reload'))
METADATA={'stage','rank','ranks','workers','complete','detailed','time_iter'}
SCOPE=('Exclusive fractions use the rank with the largest parent wall time; phase maxima are '
       'separate diagnostics and never summed. Numeric measure sums are rank work, not elapsed '
       'time. Engine work imbalance uses active workers only. Process CPU includes MPI progress; '
       'elapsed minus CPU is not MPI wait. The native wait phase is subset completion only. '
       'For M<N, active_worker_exclusive is a separately closed worker row; inactive subset waiting can dominate the global critical row. Tracked bytes include self traffic and omit direct/vendor communication. New wrapper totals '
       'close import+parent+worker; nested bridge/engine details are subsets and are not added again. '
       'Tracked collectives measure wrapper calls and elapsed packing/progress time, not all MPI calls or pure waiting. '
       'Historical bytes-only rows remain legacy coverage. Directory elections are API calls, not MPI-call counts. RSS is '
       'a process lifetime high-water mark. Different stages/events remain separate; event_index is the within-stage log ordinal, not a shared cycle identifier.')

def distribution(rows,key):
    values=[r[key] for r in rows]
    if any(not math.isfinite(v) or v<(-1e-10 if key in ('other','cpu_other') else 0) for v in values):
        raise ValueError('Invalid profile measure: '+key)
    mean=statistics.mean(values);maximum=max(values)
    return dict(rank_values=[dict(rank=int(r['rank']),value=v) for r,v in zip(rows,values)],
                rank_sum=sum(values),mean=mean,maximum=maximum,
                maximum_over_mean=maximum/mean if mean>0 else None)

def summarize(rows,phases,active_workers=False):
    if not rows:raise ValueError('Missing rank profile')
    ranks=rows[0]['ranks']
    if ranks<1 or int(ranks)!=ranks or any(r['ranks']!=ranks for r in rows):
        raise ValueError('Inconsistent rank count')
    if len(rows)!=ranks or {r['rank'] for r in rows}!=set(range(int(ranks))):
        raise ValueError('Missing/duplicate rank profile')
    if any(set(r)!=set(rows[0]) for r in rows):raise ValueError('Inconsistent profile schema')
    rows=sorted(rows,key=lambda r:r['rank'])
    if 'workers' in rows[0]:
        workers=rows[0]['workers']
        if workers<1 or int(workers)!=workers or workers>ranks or any(r['workers']!=workers for r in rows):
            raise ValueError('Inconsistent worker count')
    else:workers=ranks
    keys=set(rows[0])-METADATA
    if phases is not None:
        for r in rows:
            closes(r,phases)
            if 'cpu_other' in r:
                closes(dict(total=r['cpu'],other=r['cpu_other'],**{k:r[k+'_cpu'] for k in phases}),phases)
    tracked=[transport_counters(r) for r in rows]
    measured=rows[:int(workers)] if active_workers else rows
    if active_workers and any(r[k]!=0 for r in rows[int(workers):] for k in keys):
        raise ValueError('Inactive engine rank reported work')
    measures={k:distribution(measured,k) for k in sorted(keys)}
    result=dict(ranks=int(ranks),workers=int(workers) if 'workers' in rows[0] else None,measured_ranks=[int(r['rank']) for r in measured],measures=measures)
    result['transport_accounting']='wrapper_totals' if all(tracked) else ('legacy_bytes_only' if 'tracked_bytes' in keys else 'not_measured')
    if phases is not None:
        critical=max(measured,key=lambda r:r['total'])
        labels=(*phases,'other')
        result.update(elapsed_seconds=critical['total'],critical_rank=int(critical['rank']),
                      exclusive_seconds={k:critical[k] for k in labels},
                      exclusive_fractions={k:critical[k]/critical['total'] if critical['total'] else None for k in labels},
                      dominant_exclusive_phase=max(labels,key=lambda k:critical[k]))
        if not active_workers and workers<ranks:
            worker=max(rows[:int(workers)],key=lambda r:r['total'])
            result['active_worker_exclusive']=dict(rank=int(worker['rank']),rank_wall_seconds=worker['total'],
                seconds={k:worker[k] for k in labels},
                fractions={k:worker[k]/worker['total'] if worker['total'] else None for k in labels},
                dominant_phase=max(labels,key=lambda k:worker[k]))
    if 'hits' in measures and measures['requests']['rank_sum']:
        result['cache_hit_fraction']=measures['hits']['rank_sum']/measures['requests']['rank_sum']
    if 'accepted' in measures and result.get('elapsed_seconds',0)>0:
        result['accepted_per_second']=measures['accepted']['rank_sum']/result['elapsed_seconds']
    return result

def events(rows,schemas,active_stages=()):
    grouped=defaultdict(lambda:defaultdict(list))
    for row in rows:
        if row['stage'] not in schemas:raise ValueError('Unknown profile stage: '+row['stage'])
        grouped[row['stage']][row['rank']].append(row)
    result=[]
    for stage,by_rank in sorted(grouped.items()):
        if len({len(v) for v in by_rank.values()})!=1:raise ValueError('Unmatched stage events')
        for index in range(len(next(iter(by_rank.values())))):
            result.append(dict(stage=stage,event_index=index,
                               profile=summarize([v[index] for v in by_rank.values()],schemas[stage],stage in active_stages)))
    return result

def profiles(native,stages,details):
    native=[dict(r,stage='native') for r in native]
    return dict(native_events=events(native,{'native':NATIVE}),
                exclusive_stage_events=events(stages,STAGE),
                nested_detail_events=events(details,DETAIL,('refine','coarsen')),scope=SCOPE)
