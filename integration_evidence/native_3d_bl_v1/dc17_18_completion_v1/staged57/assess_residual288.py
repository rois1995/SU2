"""Compare complete domains in the same sealed M6 source query at fixed work."""
from pathlib import Path
import hashlib,json,re,sys
R=Path(__file__).resolve().parent
old=R/'cases/geometry_current_queries_20261109T020817_1454632'
new=Path(sys.argv[1]).resolve()
pattern=re.compile(r'NATIVE_SURFACE_DOMAIN model=1 facet=(\d+) depth=(\d+) upper=(\S+) lower=(\S+) upper_work=(\d+) distance_work=(\d+) parameter=(.*)')
def read(c):
 status=json.loads((c/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
 command=json.loads((c/'command.json').read_text());inp=command['environment']['SU2_NATIVE_GEOMETRY_QUERY_INPUT']
 pins=json.loads((c/'pins.json').read_text());assert pins[str(Path(inp).resolve())]==hashlib.sha256(Path(inp).read_bytes()).hexdigest()
 domains={}
 with (c/'run.log').open() as f:
  for line in f:
   m=pattern.fullmatch(line.rstrip('\n'))
   if not m:continue
   facet,depth,upper,lower,work,dwork,parameter=m.groups();key=(int(facet),int(depth),parameter)
   assert key not in domains
   domains[key]=(float(upper),int(work)+int(dwork))
 query=json.loads((c/'native_final_geometry_source_result.json').read_text())
 return domains,query,pins[str(Path(inp).resolve())]
a,qa,ha=read(old);b,qb,hb=read(new);assert ha==hb and a and b
common=a.keys()&b.keys()
regressions=[k for k in common if a[k][0]<=1e-4 and b[k][0]>1e-4]
assert not regressions,regressions[:4]
def stats(d,q):
 certified=sum(4.**(-k[1]) for k,v in d.items() if v[0]<=1e-4)/37202
 assert 0<=certified<=1
 return {'query':q,'completed_domains':len(d),'certified_equal_facet_parameter_share':certified,'unresolved_domains':sum(v[0]>1e-4 for v in d.values())}
out={'schema':'native_residual_search_comparison_v1','old_case':str(old),'new_case':str(new),'input_sha256':ha,'old':stats(a,qa),'new':stats(b,qb),'common_domains':len(common),'common_newly_certified':sum(a[k][0]>1e-4 and b[k][0]<=1e-4 for k in common),'common_passing_regressions':len(regressions),'common_old_work':sum(a[k][1] for k in common),'common_new_work':sum(b[k][1] for k in common),'caveat':'Equal original-facet parameter area, not physical area or partial acceptance; common-domain work excludes distinct traversal/preparation.'}
(new/'residual_search_comparison.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
