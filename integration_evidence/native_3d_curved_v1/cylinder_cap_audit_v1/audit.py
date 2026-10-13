"""Independent exact audit of fresh capped-cylinder source and saved joint strip."""
from pathlib import Path
from fractions import Fraction as F
from decimal import Decimal,localcontext
from itertools import combinations
import ast,json,hashlib,math,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant,sub,overlap
from audit_coupled import sources,weights,quad,spd,surface
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
length_source=HERE.parent/'primitive_audit_v1/audit_primitives_final.py'
module=ast.parse(length_source.read_text());definition=next(n for n in module.body if isinstance(n,ast.FunctionDef) and n.name=='exact_length')
exec(compile(ast.Module(body=[definition],type_ignores=[]),str(length_source),'exec'),globals())
case=HERE.parent/sys.argv[1];receipt=json.loads((case/'validation.json').read_text());assert receipt['status']=='PASS'
for name,digest in receipt['artifact_sha256'].items():assert sha(case/name)==digest
results={};signatures={};inputs={str(length_source.relative_to(ROOT)):sha(length_source)}
for ranks in (1,2,4):
 work=case/('mpi'+str(ranks));rows={}
 for varying in ('constant','varying'):
  source_path=work/('generated_cylinder_cap_'+varying+'_source.json');mesh_path=work/('generated_cylinder_cap_'+varying+'_strip.json')
  data=json.loads(source_path.read_text());mesh=json.loads(mesh_path.read_text());inputs[str(source_path.relative_to(ROOT))]=sha(source_path);inputs[str(mesh_path.relative_to(ROOT))]=sha(mesh_path)
  assert len(data)==120;allcells,allnodes=sources({'original':data});assert len(allnodes)==36
  # Shared identities must mean one exact coordinate/tensor at every side/cap seam.
  for a,b in combinations(allcells,2):assert not overlap(a['points'],b['points']),'Exterior collar overlaps itself or original source'
  original=[d for d in data if d['component']==(1<<64)-1];extended=[d for d in data if d['component']!=(1<<64)-1];assert len(original)==24 and len(extended)==96
  originals={n[0]:(n[1:4],n[4]) for d in original for n in d['vertices']};span=max(originals)+1
  for d in extended:
   for n in d['vertices']:assert n[4]==originals[n[0] if n[0]<span else n[0]-span][1]
  src=[]
  for priority,records in enumerate((original,extended)):
   cells,_=sources({'original':records});bykey={tuple(sorted(n[0] for n in d['vertices'])):d for d in records}
   for c in cells:c['ids']=[n[0] for n in bykey[c['key']]['vertices']];src.append((priority,c))
  def query(p):
   for priority in (0,1):
    found=[]
    for pr,c in src:
     if pr!=priority:continue
     w=weights(c['points'],p)
     if min(w)>=0:found.append((c,w))
    if found:
     tensors=[tuple(sum(w[i]*c['metrics'][i][j] for i in range(4)) for j in range(6)) for c,w in found]
     assert all(m==tensors[0] for m in tensors),'Ambiguous same-priority tensor'
     return priority,tensors[0]
   raise AssertionError('Uncovered actual strip query')
  nodes={n[0]:tuple(F(x) for x in n[1:]) for n in mesh['nodes']};minimum_q=1.;minimum_J=1.;maximum_length=0.;edges=set();extended_queries=0
  for c in mesh['cells']:
   p=[nodes[n] for n in c['nodes']];det=determinant(p);assert det>0
   center=tuple(F(float(sum(v[k] for v in p)/4)) for k in range(3));priority,m=query(center);extended_queries+=priority;volume=float(det)*math.sqrt(float(spd(m)))
   lengths={(i,j):math.sqrt(float(quad(m,sub(p[j],p[i])))) for i,j in combinations(range(4),2)}
   q=12*(volume/2)**(2/3)/sum(x*x for x in lengths.values());j=min(math.sqrt(2)*volume/math.prod(lengths[tuple(sorted((i,k)))] for k in range(4) if k!=i) for i in range(4));assert q>=.20 and j>=.05
   minimum_q=min(minimum_q,q);minimum_J=min(minimum_J,j)
   for n in c['nodes']:priority,_=query(nodes[n]);extended_queries+=priority
   edges.update(tuple(sorted((c['nodes'][i],c['nodes'][j]))) for i,j in combinations(range(4),2))
  for a,b in edges:maximum_length=max(maximum_length,exact_length(src,nodes[a],nodes[b]))
  assert maximum_length<=1.8 and extended_queries>0
  signature=sorted([sorted(tuple(n[1:4]+n[4]) for n in d['vertices']) for d in data]);digest=hashlib.sha256(json.dumps(signature,separators=(',',':')).encode()).hexdigest()
  if varying in signatures:assert signatures[varying]==digest,'Physical source differs with rank count'
  else:signatures[varying]=digest
  rows[varying]=dict(source_cells=120,original_cells=24,extension_cells=96,unique_nodes=36,exact_source_no_overlap=True,unchanged_copied_sensors=True,minimum_q=minimum_q,minimum_J=minimum_J,maximum_exact_metric_edge=maximum_length,extension_queries=extended_queries,physical_source_signature=digest)
 results[str(ranks)]=rows
report=dict(status='PASS_FRESH_COLLAR_AND_STRIP_ONLY',results=results,input_sha256=inputs,auditor_sha256=sha(Path(__file__).resolve()),scope='Exact collar embedding, shared source coordinates/tensors, copied boundary sensors, original-priority actual strip samples and exact complete edge intervals, strict strip shape/length and rank-consistent physical source at MPI1/2/4. Runtime CFD accepted-state conservation/restart audited separately; no final whole-mesh/Goal2 or efficiency claim.')
(HERE/'fresh_collar_audit.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(results,indent=2))
