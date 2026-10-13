"""Source-box broadphase must return exactly the exhaustive closed-box candidates."""
from pathlib import Path
import ast,json,hashlib
HERE=Path(__file__).resolve().parent;script=HERE/'audit_lifecycle.py'
module=ast.parse(script.read_text());defs=[n for n in module.body if isinstance(n,ast.FunctionDef) and n.name in ('source_tree','query_sources')]
ns={};exec(compile(ast.Module(body=defs,type_ignores=[]),str(script),'exec'),ns)
records=[(i%2,{'lo':(float(i)/8,0.,0.),'hi':(float(i)/8+.25,1.,1.),'key':(i,)}) for i in range(40)];tree=ns['source_tree'](records)
for i in range(81):
 a=(i/16,0.,0.);b=(i/16+.125,.5,1.)
 expected=sorted([r for r in records if all(r[1]['hi'][k]>=min(a[k],b[k]) and max(a[k],b[k])>=r[1]['lo'][k] for k in range(3))],key=lambda r:(r[0],r[1]['key']))
 assert ns['query_sources'](tree,a,b)==expected
report={'status':'PASS','cases':81,'auditor_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),'selfcheck_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'scope':'Deterministic closed source-box point/segment region candidate equivalence; exact donor and edge checks remain required.'}
(HERE/'source_index_selfcheck.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS81source-box equivalence cases')
