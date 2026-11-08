"""Seal fixed-work actual-query coverage; never treat an incomplete proof as acceptance."""
from pathlib import Path
from fractions import Fraction
import collections,hashlib,json,re,sys
R=Path(__file__).resolve().parent
assert len(sys.argv)==2
D=Path(sys.argv[1]).resolve();assert D.parent==(R/'cases').resolve()
status=json.loads((D/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
assert json.loads((D/'terminal_handles.json').read_text())['all_owned_handles_terminal']
before=json.loads((R/'cases/geometry_current_queries_20261108T211658_779719/direction_trace_assessment.json').read_text())
counts=collections.Counter();passed=collections.Counter();facets=set();bounds=[];graphs=[];actual=False;markers=0;pending_model=None
for line in (D/'run.log').open():
    if line.startswith('NATIVE_SURFACE_PLANES triangles=607712 '):pending_model=line.strip()
    if line.startswith('NATIVE_SURFACE_MODEL pieces=607712 '):
        assert not actual;actual=True;markers+=1;graphs=[pending_model,line.strip()];continue
    if not actual:continue
    if line.startswith('NATIVE_SURFACE_PLANES '):graphs.append(line.strip())
    if line.startswith('NATIVE_SURFACE_BOUND '):bounds.append(line.strip())
    if not line.startswith('NATIVE_SURFACE_DOMAIN '):continue
    values=dict(re.findall(r'(\w+)=([^ ]+)',line));key=(values['model'],int(values['depth']));counts[key]+=1
    if float(values['upper'])<=1e-4:passed[key]+=1
    if key[0]=='1':facets.add(int(values['facet']))
assert markers==1 and len(facets)==37202
weight=sum((Fraction(n,4**depth) for (model,depth),n in passed.items() if model=='1'),Fraction())
count_values={str(k):v for k,v in counts.items()};pass_values={str(k):v for k,v in passed.items()}
query=json.loads((D/'native_final_geometry_result.json').read_text())
comparison_keys=[k for k in query if 'seconds' not in k]
match=(count_values==before['domain_counts'] and pass_values==before['certified_counts'] and
       str(weight)==before['certified_equal_facet_parameter_weight_fraction'] and graphs==before['graphs'] and
       all(query[k]==before['query'][k] for k in comparison_keys))
result={'schema':'native_bounded_final_direction_trace_v1','source_log_sha256':hashlib.file_digest((D/'run.log').open('rb'),'sha256').hexdigest(),
        'log_bytes':(D/'run.log').stat().st_size,'domain_counts':count_values,'certified_counts':pass_values,'source_facets_seen':len(facets),
        'certified_equal_facet_parameter_weight_fraction':str(weight),'certified_equal_facet_parameter_share':float(weight/37202),
        'weight_caveat':'Equal reference-facet parameter area, not physical area or a complete certificate.',
        'last_bounds':bounds[-3:],'graphs':graphs,'reverse_reached':any(k[0]=='0' for k in counts),
        'classification_decisions_match282':match,'query':query,'full_mesh_accepted':False}
(D/'direction_trace_assessment.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2));assert match,'Inspect changed non-time query decisions before qualification'
