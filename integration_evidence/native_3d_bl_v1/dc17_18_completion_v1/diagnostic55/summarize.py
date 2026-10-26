"""Reconcile bounded observations without treating wait time as useful computation."""
import bisect
import csv
import json
import re
import subprocess
from collections import defaultdict
from prepare import ROOT, sha


def records(mode):
    rows=[]
    for number,line in enumerate((ROOT/mode/'run.log').read_text().splitlines(),1):
        if line.startswith(('NATIVE3D ','NATIVE3D_DETAIL ')):
            row=dict(re.findall(r'(\w+)=([^\s]+)',line));row['raw_line']=number
            row['record']=line.split()[0];rows.append(row)
    return rows


def main():
    plain,observed=records('plain'),records('observed')
    phases={m:sorted([r for r in rows if r.get('stage')=='refine'],key=lambda r:int(r['rank']))
            for m,rows in [('plain',plain),('observed',observed)]}
    numerical=['accepted','attempted','global_work','metric_shape_rejected','geometry_rejected',
               'optimization_trials','optimization_validated','final_owned_cells','cached_invalid_cells','remaining_edges']
    for a,b in zip(phases['plain'],phases['observed']):
        for key in numerical:
            assert key in a and key in b,(key,a.keys())
            assert a[key]==b[key],(key,a[key],b[key])
    binary=ROOT.parents[3]/'integration_evidence/build-integrated-v2/UnitTests/test_driver'
    symbols=[]
    for line in subprocess.check_output(['nm','-n','-C','--defined-only',str(binary)],text=True).splitlines():
        fields=line.split(None,2)
        if len(fields)==3 and fields[1] in ('T','t','W','w'):
            symbols.append((int(fields[0],16),fields[2]))
    symbols.sort();addresses=[s[0] for s in symbols]
    calls=[];summary=[]
    for rank in range(6):
        s=json.loads((ROOT/f'observed/mpi_summary_rank{rank}.json').read_text());assert s['overflow']==0
        s['outside_wrapped_mpi_seconds']=s['elapsed']-s['mpi_seconds']
        s['useful_compute_seconds']=None
        s['mpi_fraction']=s['mpi_seconds']/s['elapsed'];summary.append(s)
        aggregate=defaultdict(lambda:dict(calls=0,seconds=0.,over_100us=0,maximum=0.))
        for row in csv.DictReader((ROOT/f'observed/mpi_calls_rank{rank}.csv').open()):
            key=(row['call'],row['caller_offset'],row['module'])
            a=aggregate[key];a['calls']+=int(row['calls']);a['seconds']+=float(row['seconds'])
            a['over_100us']+=int(row['over_100us']);a['maximum']=max(a['maximum'],float(row['max_seconds']))
        assert sum(x['calls'] for x in aggregate.values())==s['calls']
        assert abs(sum(x['seconds'] for x in aggregate.values())-s['mpi_seconds'])<1e-8
        for (call,offset,module),a in aggregate.items():
            address=int(offset,16);index=bisect.bisect_right(addresses,address)-1
            symbol=symbols[index][1] if module==str(binary) and index>=0 else 'unresolved module'
            calls.append(dict(rank=rank,call=call,caller_offset=offset,module=module,symbol=symbol,**a))
    with (ROOT/'m6_mpi_callsites.csv').open('w') as f:
        w=csv.DictWriter(f,list(calls[0]));w.writeheader();w.writerows(sorted(calls,key=lambda r:(r['rank'],-r['seconds'])))
    durations=[json.loads((ROOT/m/'construction_result.json').read_text())['seconds'] for m in ['plain','observed']]
    outside=max(s['outside_wrapped_mpi_seconds'] for s in summary)
    report=dict(ranks=summary,numerical_counter_match=numerical,plain_seconds=durations[0],observed_seconds=durations[1],
        observed_relative_delta=durations[1]/durations[0]-1,
        true_instrumentation_overhead=None,
        overhead_limitation='One sequential pair on a shared host; negative elapsed delta cannot identify instrumentation overhead',
        conditional_zero_wrapped_MPI_remaining_seconds=outside,
        conditional_zero_wrapped_MPI_speedup_ceiling=max(s['elapsed'] for s in summary)/outside,
        bound_assumptions='Fixed per-rank outside-MPI work; every wrapped MPI duration unrealistically removed; no migration or algorithm changes',
        native_mesh_accepted=False,unclassified='Outside wrapped MPI includes scheduling, bookkeeping, observer overhead and any unwrapped calls; not useful-compute time')
    (ROOT/'mpi_analysis.json').write_text(json.dumps(report,indent=2)+'\n')
    (ROOT/'bounded_records.json').write_text(json.dumps(dict(plain=plain,observed=observed),indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
