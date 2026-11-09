"""Compare exact common completed domains of the same actual M6 geometry query."""
from pathlib import Path
from fractions import Fraction
import hashlib,json,re,sys,collections
R=Path(__file__).resolve().parent
assert len(sys.argv)==3 and sys.argv[1] in ('source','reverse')
mode=sys.argv[1];new=Path(sys.argv[2]).resolve();assert new.parent==(R/'cases').resolve()
old=R/'cases'/('geometry_current_queries_20261109T020817_1454632' if mode=='source' else 'geometry_current_queries_20261109T001246_1206798')
def parse(c):
 status=json.loads((c/'status.json').read_text());assert status['exit_code']==0 and status['pins_verified']
 assert json.loads((c/'terminal_handles.json').read_text())['all_owned_handles_terminal']
 domains=[];counts=collections.Counter();passed=collections.Counter();graphs=collections.Counter()
 for line in (c/'run.log').open():
  if line.startswith('NATIVE_SURFACE_GRAPH '):
   v=dict(re.findall(r'(\w+)=([^ ]+)',line));graphs['calls']+=1;graphs['coverage_work']+=int(v['coverage_work']);graphs['lift_work']+=int(v['lift_work'])
  if not line.startswith('NATIVE_SURFACE_DOMAIN '):continue
  v=dict(re.findall(r'(\w+)=([^ ]+)',line));depth=int(v['depth']);counts[depth]+=1
  domains.append((v['model'],v['facet'],v['depth'],v['parameter'].strip(),v['upper'],v['lower'],int(v['upper_work']),int(v['distance_work'])))
  if float(v['upper'])<=1e-4:passed[depth]+=1
 q=json.loads((c/'native_final_geometry_source_result.json').read_text())
 return domains,counts,passed,graphs,q
oldcmd=json.loads((old/'command.json').read_text());newcmd=json.loads((new/'command.json').read_text())
oldpins=json.loads((old/'pins.json').read_text());newpins=json.loads((new/'pins.json').read_text())
for key in ['SU2_NATIVE_TRANSACTION_REFERENCE','SU2_NATIVE_GEOMETRY_QUERY_INPUT']:
 assert oldcmd['environment'][key]==newcmd['environment'][key]
 path=str(Path(newcmd['environment'][key]).resolve());assert oldpins[path]==newpins[path]
before,oc,op,og,oq=parse(old);after,nc,np,ng,nq=parse(new)
common=min(len(before),len(after))
assert [v[:6] for v in before[:common]]==[v[:6] for v in after[:common]],'Changed a completed query domain or bound: inspect before qualification'
oldwork=sum(v[6]+v[7] for v in before[:common]);newwork=sum(v[6]+v[7] for v in after[:common])
assert before==after,'Preparation must preserve every domain, bound, and per-domain charged work'
result={'schema':'native_exact_face_coverage_bounded_comparison_v1','direction':mode,'old_case':str(old),'new_case':str(new),'same_actual_input':True,'completed_old_domains':len(before),'completed_new_domains':len(after),'common_completed_domains':common,'common_domain_decisions_and_bounds_identical':True,'common_domain_old_work':oldwork,'common_domain_new_work':newwork,'common_domain_work_ratio':newwork/oldwork,'complete_domain_work_identical':True,'old_query':oq,'new_query':nq,'old_domain_counts':dict(oc),'new_domain_counts':dict(nc),'old_certified_counts':dict(op),'new_certified_counts':dict(np),'old_nested_graph_work':dict(og),'new_nested_graph_work':dict(ng),'work_caveat':'Graph work nested within domain/traversal; interrupted final query can lack a completed domain line. Different work cap progress is not complete certification or whole-run timing.','full_mesh_accepted':False}
if mode=='source':
 for label,values in [('old',op),('new',np)]:result[label+'_equal_facet_parameter_coverage']=float(sum((Fraction(n,4**d) for d,n in values.items()),Fraction())/37202)
for label,c in [('old',old),('new',new)]:
 log=(c/'run.log').read_text()
 user=float(re.search(r'User time \(seconds\): ([0-9.]+)',log)[1]);system=float(re.search(r'System time \(seconds\): ([0-9.]+)',log)[1])
 result[label+'_serial_tree_cpu_seconds']=user+system
 result[label+'_pins']={str(c/n):hashlib.file_digest((c/n).open('rb'),'sha256').hexdigest() for n in ['run.log','command.json','pins.json','status.json','terminal_handles.json','native_final_geometry_source_result.json']}
(new/'prepared_border_comparison.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if 'pins' not in k},indent=2))
