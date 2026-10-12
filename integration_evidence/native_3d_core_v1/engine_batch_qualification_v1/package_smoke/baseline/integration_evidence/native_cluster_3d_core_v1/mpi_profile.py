"""Audit verbose mpiP counts/times; aggregate rank work is never elapsed or pure wait."""
from collections import defaultdict
from decimal import Decimal,InvalidOperation
import math,re,statistics

def number(token):
    try:value=float(token);decimal=Decimal(token)
    except (ValueError,InvalidOperation):raise ValueError('Invalid mpiP number: '+token)
    if not math.isfinite(value) or value<0:raise ValueError('Invalid mpiP number: '+token)
    # Printed precision bounds comparison; no solver/mesh tolerance is changed.
    error=float(Decimal(5).scaleb(decimal.as_tuple().exponent-1))
    if not math.isfinite(error):raise ValueError('Invalid mpiP precision')
    return value,error

def total(values):
    value=error=0
    for v,e in values:value+=v;error+=e
    return value,error

def closes(a,b):
    if any(not math.isfinite(x) or x<0 for x in (*a,*b)):
        raise ValueError('Invalid mpiP derived total')
    if abs(a[0]-b[0])>a[1]+b[1]+1e-12*max(a[0],b[0],1):
        raise ValueError('mpiP time totals do not close at printed precision')

def summary(text,ranks):
    if type(ranks) is not int or ranks<1:raise ValueError('Invalid MPI rank count')
    sections={};current=None
    for line in text.splitlines():
        match=re.match(r'^@--- (.*?) -{2,}',line)
        if match:
            current=match[1]
            if current in sections:raise ValueError('Duplicate mpiP section')
            sections[current]=[]
        elif current is not None and line.strip() and set(line.strip())!={'-'}:
            sections[current].append(line.split())
    def section(prefix,header):
        keys=[k for k in sections if k.startswith(prefix)]
        if len(keys)!=1:raise ValueError('Missing/duplicate verbose mpiP section: '+prefix)
        return [row for row in sections[keys[0]] if row[0]!=header]
    tasks=re.findall(r'^@ MPI Task Assignment\s*:\s*(\d+)\s',text,re.M)
    if sorted(map(int,tasks))!=list(range(ranks)):raise ValueError('mpiP rank assignments differ')
    timing={}
    for row in section('MPI Time (seconds)','Task'):
        if len(row)!=4 or row[0] in timing:raise ValueError('Invalid/duplicate mpiP rank time')
        if row[0]!='*' and (not row[0].isdigit() or int(row[0])>=ranks):
            raise ValueError('Invalid mpiP rank time')
        timing[row[0]]=tuple(number(token) for token in row[1:])
    if set(timing)!={str(r) for r in range(ranks)}|{'*'}:raise ValueError('Missing mpiP rank time')
    for col in (0,1):closes(timing['*'][col],total(timing[str(r)][col] for r in range(ranks)))
    for r in range(ranks):
        app,mpi,_=timing[str(r)]
        if mpi[0]-mpi[1]>app[0]+app[1]:raise ValueError('MPI time exceeds single-thread application time')
    stacks=defaultdict(dict)
    for row in section('Callsites:','ID'):
        if len(row)<3 or not row[0].isdigit() or not row[1].isdigit():
            raise ValueError('Invalid mpiP callsite stack')
        site,level=map(int,row[:2])
        if level in stacks[site]:raise ValueError('Duplicate mpiP stack level')
        stacks[site][level]=' '.join(row[2:])
    stack_title=next(k for k in sections if k.startswith('Callsites:'))
    if int(stack_title.split(':')[1].strip())!=len(stacks):raise ValueError('mpiP declared callsite count differs')
    calls=defaultdict(dict)
    for row in section('Callsite Time statistics (all, milliseconds)','Name'):
        if len(row)!=9 or not row[1].isdigit() or not row[3].isdigit():
            raise ValueError('Invalid mpiP callsite time row')
        name,site,rank=row[:3];site=int(site);count=int(row[3])
        if count<1 or (rank!='*' and (not rank.isdigit() or int(rank)>=ranks)):
            raise ValueError('Invalid mpiP callsite rank/count')
        key=(name,site)
        if rank in calls[key]:raise ValueError('Duplicate mpiP callsite rank')
        values=[number(token) for token in row[4:]]
        maximum,mean,minimum=values[:3]
        if maximum[0]<mean[0] or mean[0]<minimum[0]:raise ValueError('Invalid mpiP duration order')
        calls[key][rank]=dict(count=count,seconds=mean[0]*count/1000,error=mean[1]*count/1000)
    if not calls:raise ValueError('Missing mpiP callsite statistics')
    time_title=next(k for k in sections if k.startswith('Callsite Time statistics (all, milliseconds)'))
    if int(time_title.split(':')[1].strip())!=sum(len(rows)-('*' in rows) for rows in calls.values()):
        raise ValueError('mpiP declared rank-record count differs')
    per_rank=[dict(rank=r,application_seconds=timing[str(r)][0][0],mpi_seconds=timing[str(r)][1][0],
                   observed_api_calls=0,estimated_api_seconds=0,printed_time_error=0) for r in range(ranks)]
    sites=[];functions=defaultdict(lambda:dict(observed_api_calls=0,estimated_rank_work_seconds=0,printed_time_error=0))
    for (name,site),rows in calls.items():
        aggregate=rows.get('*');individual=[v for r,v in rows.items() if r!='*']
        if aggregate is None or aggregate['count']!=sum(v['count'] for v in individual):
            raise ValueError('mpiP callsite counts do not close')
        closes((aggregate['seconds'],aggregate['error']),total((v['seconds'],v['error']) for v in individual))
        if 0 not in stacks.get(site,{}):raise ValueError('Missing mpiP callsite stack')
        for rank,row in rows.items():
            if rank=='*':continue
            r=per_rank[int(rank)];r['observed_api_calls']+=row['count']
            r['estimated_api_seconds']+=row['seconds'];r['printed_time_error']+=row['error']
        sites.append(dict(call=name,site=site,observed_api_calls=aggregate['count'],
                          estimated_rank_work_seconds=aggregate['seconds'],printed_time_error=aggregate['error'],
                          stack=[dict(level=k,frame=v) for k,v in sorted(stacks[site].items())]))
        functions[name]['observed_api_calls']+=aggregate['count'];functions[name]['estimated_rank_work_seconds']+=aggregate['seconds'];functions[name]['printed_time_error']+=aggregate['error']
    for r in per_rank:
        closes((r['estimated_api_seconds'],r['printed_time_error']),timing[str(r['rank'])][1])
    mpi=[r['mpi_seconds'] for r in per_rank]
    return dict(per_rank=per_rank,observed_api_calls=sum(r['observed_api_calls'] for r in per_rank),
                reported_rank_work_mpi_seconds=timing['*'][1][0],
                mpi_time_maximum_over_mean=max(mpi)/statistics.mean(mpi) if any(mpi) else None,
                per_function=dict(functions),callsites=sorted(sites,key=lambda s:s['estimated_rank_work_seconds'],reverse=True),
                scope='Whole-process wrapped MPI API statistics. Callsite seconds are count times printed mean milliseconds; rounding bounds are explicit. Rank sums are work, not elapsed. No pure wait, physical wire traffic, chronological phase or complete MPI/vendor coverage claim.')
