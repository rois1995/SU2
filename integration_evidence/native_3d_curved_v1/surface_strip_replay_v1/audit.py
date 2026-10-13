"""Independent exact certificate for a local proposal, never a completed remesh."""
from pathlib import Path
from fractions import Fraction as F
from itertools import combinations
from collections import Counter
import json,hashlib,math,sys,os
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant,overlap
from audit_coupled import surface,sources,weights,quad,spd
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
original=HERE.parent/'recovery_collapse_v1/mpi1/cylinder_axis1_m1_s0_rejected.json'
source_file=HERE.parent/'recovery_collapse_v1/mpi1/cylinder_axis1_m1_s0_source.json'
proposal=json.loads((HERE/'probe.json').read_text());assert proposal['input_sha256']==sha(original)
b=proposal['best'];assert b
old=json.loads(original.read_text());base={n[0]:tuple(F(x) for x in n[1:]) for n in old['nodes']};nodes=dict(base)
nodes.update({n[0]:tuple(F(x) for x in n[1:]) for n in b['nodes']})
removed=set(b['old_cell_ids']);outside=[c['nodes'] for c in old['cells'] if c['id'] not in removed];replacement=b['cells']
# Rational binary64 determinants and exact SAT intersection, including all unchanged neighbors.
for c in replacement:assert determinant([nodes[n] for n in c])>0
for a,c in combinations(replacement,2):assert not overlap([nodes[n] for n in a],[nodes[n] for n in c])
for a in replacement:
 for c in outside:assert not overlap([nodes[n] for n in a],[nodes[n] for n in c])
faces=lambda c:((c[1],c[2],c[3]),(c[0],c[3],c[2]),(c[0],c[1],c[3]),(c[0],c[2],c[1]))
counts=Counter(tuple(sorted(f)) for c in replacement+outside for f in faces(c));assert max(counts.values())==2
wall=[f for f in old['surface'] if not any(set(f['nodes']).issubset(c['nodes']) for c in old['cells'] if c['id'] in removed)]+b['surface']
skin={key for key,n in counts.items() if n==1};assert skin=={tuple(sorted(f['nodes'])) for f in wall};surface([f['nodes'] for f in wall],True)
# Artificial patch skin must retain all identities and exact coordinates.
oldcounts=Counter(tuple(sorted(f)) for c in old['cells'] if c['id'] in removed for f in faces(c['nodes']))
newcounts=Counter(tuple(sorted(f)) for c in replacement for f in faces(c))
oldwall={tuple(sorted(f['nodes'])) for f in old['surface']};newwall={tuple(sorted(f['nodes'])) for f in b['surface']}
interface={key for key,n in oldcounts.items() if n==1 and key not in oldwall}
assert interface=={key for key,n in newcounts.items() if n==1 and key not in newwall}
for f in interface:
 for n in f:assert nodes[n]==base[n]
rawsource=json.loads(source_file.read_text());sources({'original':rawsource});minimum_q=1.;minimum_j=1.;extended=original_samples=0
src=[]
for priority in (0,1):
 data=[r for r in rawsource if int(r['component']!=(1<<64)-1)==priority]
 group,_=sources({'original':data});src.extend((priority,c) for c in group)
def sample(p):
 global extended,original_samples
 for priority in (0,1):
  chosen=[]
  for pr,c in src:
   if pr!=priority:continue
   w=weights(c['points'],p)
   if min(w)>=0:chosen.append((c,w))
  if chosen:
   tensors=[tuple(sum(w[i]*c['metrics'][i][j] for i in range(4)) for j in range(6)) for c,w in chosen]
   assert all(t==tensors[0] for t in tensors),'Ambiguous source value'
   if priority:extended+=1
   else:original_samples+=1
   return tensors[0]
 raise AssertionError('Uncovered actual query')
for c in replacement:
 p=[nodes[n] for n in c];center=tuple(F(float(sum(x[k] for x in p)/4)) for k in range(3));m=sample(center);dv=float(determinant(p))*math.sqrt(float(spd(m)))
 lengths={(i,j):math.sqrt(float(quad(m,tuple(p[j][k]-p[i][k] for k in range(3))))) for i,j in combinations(range(4),2)}
 q=12*(dv/2)**(2/3)/sum(l*l for l in lengths.values());j=min(math.sqrt(2)*dv/math.prod(lengths[tuple(sorted((i,k)))] for k in range(4) if i!=k) for i in range(4));assert q>=.20 and j>=.05
 minimum_q=min(minimum_q,q);minimum_j=min(minimum_j,j)
 for n in c:sample(nodes[n])
# Inspectable whole working mesh, expressly a proposal rather than an accepted CFD mesh.
ids=sorted({n for c in replacement+outside for n in c});index={n:i for i,n in enumerate(ids)};facets={f['id']:f for f in old['facets']}
lines=['NDIME= 3','NELEM= '+str(len(replacement+outside))]+['10 '+' '.join(str(index[n]) for n in c)+' '+str(i) for i,c in enumerate(replacement+outside)]
lines+=['NPOIN= '+str(len(ids))]+[' '.join(format(float(x),'.17g') for x in nodes[n])+' '+str(index[n]) for n in ids]+['NMARK= '+str(len(old['marker_names']))]
for marker,name in enumerate(old['marker_names']):
 fs=[f for f in wall if facets[f['facet']]['marker']==marker];lines+=['MARKER_TAG= '+name,'MARKER_ELEMS= '+str(len(fs))]+['5 '+' '.join(str(index[n]) for n in f['nodes']) for f in fs]
mesh=HERE/'cylinder_axis1_strip_proposal.su2';mesh.write_text('\n'.join(lines)+'\n')
report=dict(status='PASS_LOCAL_EXACT_PROPOSAL_ONLY',minimum_q=minimum_q,minimum_J=minimum_j,original_priority_queries=original_samples,extension_queries=extended,exact_positive_conforming_no_overlap=True,fixed_artificial_interface=True,whole_mesh_cells=len(replacement+outside),input_sha256={str(p.relative_to(ROOT)):sha(p) for p in (original,source_file,HERE/'probe.json',Path(__file__))},mesh_sha256=sha(mesh),scope='Independent rational determinants and overlaps against unchanged neighbors, whole-domain skin closure, fixed interface coordinates, actual original-priority/declared extension centroids and vertices; source-resolved complete edge lengths and authoritative geometry certified separately by native focused test. Floating proposal only; not final geometry tolerance completion, CFD transfer, MPI publication or full Goal2.')
(HERE/'exact_audit.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
