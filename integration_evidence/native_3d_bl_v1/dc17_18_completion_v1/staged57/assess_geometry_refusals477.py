"""Match the frozen trace to476 and retain exact certified query evidence, not a timing baseline."""
import collections,gzip,hashlib,json,re,struct,time
from pathlib import Path
from assess_surface_transaction470 import terminal_receipt,sha
R=Path(__file__).resolve().parent

def main():
    case=max((R/'cases').glob('curved476_current_trajectory_trace_serial_*'),key=lambda p:p.stat().st_mtime)
    status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
    pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
    receipt=terminal_receipt(case);(case/'terminal_handles_actual.json').write_text(json.dumps(receipt,indent=2)+'\n')
    source=json.loads((R/'current_trajectory476_assessment.json').read_text());reference=Path(source['case'])
    rows=lambda p:[json.loads(l) for l in (p/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
    records=rows(case);before=rows(reference)
    for stage in ('transaction_operation','trajectory_budget','current_trajectory_capture','current_trajectory_context'):
        assert [v for v in records if v['stage']==stage]==[v for v in before if v['stage']==stage],stage
    for name in ('trajectory_working_owned.bin','trajectory_working_parameters.bin','correction_unchanged_owned.bin'):
        assert sha(case/name)==sha(reference/name),name
    decode=lambda value:struct.unpack('>d',bytes.fromhex(value))[0]
    query=re.compile(r'NATIVE_SURFACE_QUERY authority=(\S+) budget=(\S+) start=(\d+) tolerance=([0-9a-f]+) support=(\S*) triangles=(.*)')
    result=re.compile(r'NATIVE_SURFACE_QUERY_RESULT authority=(\S+) budget=(\S+) start=(\d+) cache=(\S+) work=(\d+) bound=([0-9a-f]+)')
    domain=re.compile(r'NATIVE_SURFACE_DOMAIN model=0 facet=(\d+) depth=(\d+) upper=(\S+) lower=(\S+)')
    active=None;queries=[];memo={}
    with (case/'run.log').open() as stream:
        for line in stream:
            m=query.fullmatch(line.rstrip())
            if m:
                assert active is None
                triangles=[s.split(',')[:-1] for s in m[6].split(';') if s]
                assert triangles and all(len(t)==9 for t in triangles)
                active=dict(authority=m[1],budget=m[2],start=int(m[3]),tolerance_bits=m[4],support=[int(s) for s in m[5].split(',') if s],triangles_bits=triangles,domains=0,maximum_lower=0.,maximum_depth=0)
                continue
            m=domain.match(line)
            if m and active is not None:
                active['domains']+=1;active['maximum_lower']=max(active['maximum_lower'],float(m[4]));active['maximum_depth']=max(active['maximum_depth'],int(m[2]))
                continue
            m=result.fullmatch(line.rstrip())
            if m:
                assert active and (m[1],m[2],int(m[3]))==(active['authority'],active['budget'],active['start'])
                active.update(cache=m[4],work=int(m[5]),bound_bits=m[6],bound=decode(m[6]),tolerance=decode(active['tolerance_bits']))
                canonical=tuple(tuple(sorted(tuple(t[k:k+3]) for k in range(0,9,3))) for t in active['triangles_bits'])
                key=(tuple(active['support']),canonical,active['tolerance_bits'])
                if active['cache']=='hit' and key in memo:
                    active['maximum_lower']=memo[key]['maximum_lower'];active['maximum_depth']=memo[key]['maximum_depth']
                else:memo[key]=active
                queries.append(active);active=None
    assert active is None and queries
    known=set()
    for v in records:
        if v['stage']=='transaction_edge':
            for k in ('a','b'):known.add(tuple(struct.pack('>d',x).hex() for x in v[k]))
    groups={}
    for q in queries:
        if len(q['triangles_bits'])!=1:continue
        t=q['triangles_bits'][0];points=[tuple(t[k:k+3]) for k in range(0,9,3)];unknown=[p for p in points if p not in known]
        if len(unknown)!=1:continue
        key=unknown[0];g=groups.setdefault(key,dict(point=[decode(x) for x in key],queries=[],triangles={}))
        triangle=tuple(sorted(points));v=g['triangles'].setdefault(triangle,dict(best_upper=float('inf'),greatest_lower=0.,queries=0))
        v['best_upper']=min(v['best_upper'],q['bound']);v['greatest_lower']=max(v['greatest_lower'],q['maximum_lower']);v['queries']+=1
        if q['cache']!='hit':g['queries'].append(q)
    output=[]
    for key,g in groups.items():
        triangles=list(g['triangles'].values());g['triangle_summary']=triangles;del g['triangles']
        g['sum_upper_excess']=sum(max(0.,v['best_upper']-1e-4)**2 for v in triangles)
        g['sum_lower_excess']=sum(max(0.,v['greatest_lower']-1e-4)**2 for v in triangles);output.append(g)
    (R/'geometry_refusals477_queries.json.gz').write_bytes(gzip.compress(json.dumps(queries).encode(),mtime=0))
    report=dict(case=str(case),all_actual_handles_terminal=True,pins_verified=len(pins),identical_operations=24,identical_working_output=True,trace_bytes=(case/'run.log').stat().st_size,queries=len(queries),cache_counts=dict(collections.Counter(q['cache'] for q in queries)),candidate_point_groups=output,limitations=['Trace repeated476 solely because refused candidate query inputs were absent from its checkpoint. No timing comparison or new baseline.','Lower bounds prove sampled-point distance to the exact captured support; they do not alone establish candidate progress or whole-entity support completeness.','Unknown-point grouping is a diagnostic association; native candidate geometry admission remains authoritative.'],production_changed=False,goal_complete=False)
    (R/'geometry_refusals477_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('case','queries','cache_counts','identical_operations','identical_working_output')},indent=2))
    for g in output:print(json.dumps({k:g[k] for k in ('point','sum_upper_excess','sum_lower_excess','triangle_summary')},indent=2))
if __name__=='__main__':main()
