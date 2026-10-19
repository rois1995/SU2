#!/usr/bin/env python3
"""Independent exact source-envelope geometry and original-sensor audit of native control outputs."""
import collections,hashlib,importlib.util,itertools,json,struct,sys,time
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def module(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
GEOMETRY=HERE.with_name('affine_initial_adoption_v1')/'source_policy_replay_v1/audit_envelope.py'
REFS=HERE.with_name('affine_initial_adoption_v1')/'audit_controls.py'
geom=module('independent_envelope',GEOMETRY);refs=module('reference_reader',REFS)
from audit_cavities import determinant
from audit_coupled import spd
sha=geom.m.sha
MAX_ID=2**64-1

def faces(cell):
 a,b,c,d=cell
 return ((b,c,d),(a,d,c),(a,b,d),(a,c,b))
def skin(cells):
 uses=collections.defaultdict(list)
 for c in cells:
  for f in faces(c):
   sign=(-1)**sum(f[i]>f[j] for i in range(3) for j in range(i+1,3));uses[tuple(sorted(f))].append((f,sign))
 assert all(len(v)==1 or len(v)==2 and sum(s for _,s in v)==0 for v in uses.values()),'Invalid oriented cell-face incidence'
 return [v[0][0] for v in uses.values() if len(v)==1]
def closed(surface):
 edges=collections.defaultdict(list);links=collections.defaultdict(lambda:collections.defaultdict(set));adj=collections.defaultdict(set)
 for i,(a,b,c) in enumerate(surface):
  for x,y,z in ((a,b,c),(b,c,a),(c,a,b)):
   edges[tuple(sorted((x,y)))].append((i,x,y));links[x][y].add(z);links[x][z].add(y)
 for entries in edges.values():
  assert len(entries)==2 and entries[0][1:]==tuple(reversed(entries[1][1:])), 'Not a closed oriented physical skin'
  i,j=entries[0][0],entries[1][0];adj[i].add(j);adj[j].add(i)
 for graph in links.values():
  assert all(len(v)==2 for v in graph.values())
  seen=set();pending=[next(iter(graph))]
  while pending:
   n=pending.pop()
   if n not in seen:seen.add(n);pending.extend(graph[n]-seen)
  assert len(seen)==len(graph),'Pinched/nonmanifold vertex link'
 seen=set();components=0
 for i in range(len(surface)):
  if i in seen:continue
  components+=1;pending=[i]
  while pending:
   n=pending.pop()
   if n not in seen:seen.add(n);pending.extend(adj[n]-seen)
 return components

def record_digest(cells,nodes,values):
 records=[]
 for c in cells:
  records.append(sorted(struct.pack('<9d',*(float(x) for x in nodes[n]),*(float(x) for x in values[n])) for n in c))
 h=hashlib.sha256()
 for row in sorted(records):
  for b in row:h.update(b)
 return h.hexdigest()

def audit_case(folder,stem):
 start=time.monotonic();source_path=folder/(stem+'_source.json');base_path=folder/(stem+'.json');ref_path=folder/(stem+'.native_ref')
 rows=json.loads(source_path.read_text());assert 0<len(rows)<=256,'Bounded source fixture size exceeded';base=json.loads(base_path.read_text());names,facets,planes,analytic=refs.reference(ref_path)
 assert not analytic and planes and names==base['marker_names']
 assert len(rows)==len({d['id'] for d in rows});nodes={};values={};primary={};extra=[]
 for d in rows:
  c=[]
  for v in d['vertices']:
   n=int(v[0]);p=tuple(F(float(x)) for x in v[1:4]);metric=tuple(F(float(x)) for x in v[4]);spd(metric)
   assert len(metric)==6 and len(p)==3 and 0<=n<MAX_ID
   if n in nodes:assert nodes[n]==p and values[n]==metric,'Shared coordinate/tensor mismatch'
   nodes[n]=p;values[n]=metric;c.append(n)
  assert len(set(c))==4 and determinant([nodes[n] for n in c])>0,'Nonpositive source tetrahedron'
  if d['component']==MAX_ID:primary[d['id']]=c
  else:extra.append((d,c))
 assert primary and extra
 old_nodes={n for c in primary.values() for n in c};span=max(old_nodes)+1
 assert len({tuple(sorted(c)) for _,c in extra})==len(extra)
 assert all(d['component'] in planes for d,_ in extra),'Undeclared source group'
 for d,c in extra:
  assert d['parent'] in primary
  for n in c:
   parent=n%span
   assert parent in primary[d['parent']] and values[n]==values[parent],'Extension does not copy its original parent trace'
 assert len(primary)==len(base['cells'])
 assert {tuple(sorted(c)) for c in primary.values()}=={tuple(sorted(c['nodes'])) for c in base['cells']}
 assert {int(n[0]):tuple(F(float(x)) for x in n[1:]) for n in base['nodes']}=={n:nodes[n] for n in old_nodes}
 old_cells=list(primary.values());all_cells=old_cells+[c for _,c in extra];assert len({tuple(sorted(c)) for c in all_cells})==len(all_cells)
 old_skin=skin(old_cells);old_components=closed(old_skin);new_skin=skin(all_cells);new_components=closed(new_skin)
 assert old_components==new_components
 expected={tuple(sorted(n+span for n in f)) for f in old_skin}
 assert {tuple(sorted(f)) for f in new_skin}==expected,'Augmented boundary is not the corresponding outer skin'
 assert len(extra)==3*len(old_skin)
 pairs=0
 for i,a in enumerate(all_cells):
  aa=[nodes[n] for n in a]
  for b in all_cells[i+1:]:
   bb=[nodes[n] for n in b]
   if any(max(p[k] for p in aa)<min(p[k] for p in bb) or max(p[k] for p in bb)<min(p[k] for p in aa) for k in range(3)):continue
   pairs+=1;assert geom.conforming(aa,a,bb,b),'Uncertified exact source contact'
  assert time.monotonic()-start<120,'Independent audit elapsed admission exhausted'
 radius=4*max(F(g['error']) for g in planes.values());distance_checks=0
 for f in old_skin:
  a=[nodes[n] for n in f]
  for g in new_skin:
   distance_checks+=1;assert geom.separated(a,[nodes[n] for n in g],radius),'Outer source boundary is inside its guaranteed band'
 # The unmodified actual nodal owner field is mildly anisotropic and affine in these controls.
 max_nodal_residual=0.
 for n in old_nodes:
  x,y,z=nodes[n];expected=(4+x/8,x/32,F(0),4+y/8,F(0),4+z/8)
  residual=max(abs(float(a-b)) for a,b in zip(values[n],expected));max_nodal_residual=max(max_nodal_residual,residual)
  assert residual<3e-14,'Original owner sensor changed or was composed with BL'
 original_volume=sum(determinant([nodes[n] for n in c])/6 for c in old_cells)
 collar_volume=sum(determinant([nodes[n] for n in c])/6 for _,c in extra)
 assert original_volume>0 and collar_volume>0
 outputs=HERE/'audits'/'meshes';outputs.mkdir(parents=True,exist_ok=True)
 export=outputs/(folder.name+'_'+stem+'_source_envelope.su2')
 geom.export(export,{n:list(map(float,p)) for n,p in nodes.items()},[dict(nodes=c) for c in all_cells])
 prep=json.loads((folder/(stem+'_preparation.json')).read_text())
 assert prep['nodes']==len(set(nodes)-old_nodes) and prep['cells']==len(extra) and prep['column_work']>0 and prep['contacts']>0 and prep['distances']>0
 return dict(case=stem,original_cells=len(old_cells),collar_cells=len(extra),original_nodes=len(old_nodes),outer_nodes=len(set(nodes)-old_nodes),
  components=old_components,exact_conforming_pairs=pairs,exact_old_outer_pairs=distance_checks,guaranteed_radius=float(radius),
  original_volume_exact=str(original_volume),collar_volume_exact=str(collar_volume),maximum_nodal_affine_sensor_residual=max_nodal_residual,
  original_field_signature=record_digest(old_cells,nodes,values),complete_source_field_signature=record_digest(all_cells,nodes,values),
  native_preparation=prep,audit_seconds=time.monotonic()-start,source_sha256=sha(source_path),base_sha256=sha(base_path),reference_sha256=sha(ref_path),
  export=str(export.relative_to(ROOT)),export_sha256=sha(export))

def selfcheck():
 geom.selfcheck()
 tet=[0,1,2,3];assert len(skin([tet]))==4 and closed(skin([tet]))==1
 assert closed(skin([tet,[4,5,6,7]]))==2
 try:closed([(0,1,2)]);raise RuntimeError('Open surface incorrectly accepted')
 except AssertionError:pass
 nodes={0:(F(0),F(0),F(0)),1:(F(1),F(0),F(0)),2:(F(0),F(1),F(0)),3:(F(0),F(0),F(1))}
 metrics={i:(F(2),F(0),F(0),F(3),F(0),F(4)) for i in nodes}
 expected=record_digest([tet],nodes,metrics)
 assert record_digest([[i+10 for i in tet]],{i+10:p for i,p in nodes.items()},{i+10:v for i,v in metrics.items()})==expected
 changed=dict(metrics);changed[0]=(F(3),F(0),F(0),F(3),F(0),F(4));assert record_digest([tet],nodes,changed)!=expected

def main():
 ranks=int(sys.argv[1]) if len(sys.argv)>1 else 1;assert ranks in (1,2,4)
 version=sys.argv[2] if len(sys.argv)>2 else '';assert not version or (version.startswith('v') and version[1:].isdigit())
 mode=sys.argv[3] if len(sys.argv)>3 else 'source';assert mode in ('source','joint')
 folder=HERE/(mode+('_'+version if version else '')+'_n'+str(ranks))
 build=json.loads((HERE/'build_process.json').read_text());assert build['status']=='PASS' and build['source_unchanged']
 assert build['binary_sha256']==sha(ROOT/'integration_evidence/build-integrated-v2/UnitTests/test_driver')
 assert all(sha(ROOT/p)==h for p,h in build['source_sha256'].items()),'Current source differs from executed binary'
 result=json.loads((folder/'result.json').read_text());proof=folder/'provenance.json';provenance=json.loads(proof.read_text())
 assert result['exit_code']==0 and result['provenance_sha256']==sha(proof) and result['log_sha256']==sha(folder/'run.log')
 assert provenance['ranks']==ranks
 for p,h in provenance['pins'].items():assert sha(ROOT/p)==h,('Changed execution input',p)
 assert all(sha(folder/p)==h for p,h in result['artifacts'].items())
 selfcheck();stems=['joint_affine_source_'+physics+'_m'+str(m) for physics in ('euler','rans') for m in (1,2,4) if m<=ranks]
 assert sorted(p.name[:-len('_source.json')] for p in folder.glob('joint_affine_source_*_source.json'))==sorted(stems)
 cases=[audit_case(folder,stem) for stem in stems]
 # Same frozen geometry/sensor must survive solver choice and number of adaptation workers.
 assert len({c['original_field_signature'] for c in cases})==1
 assert len({c['complete_source_field_signature'] for c in cases})==1
 audit_dir=HERE/'audits';report=dict(status='PASS_INDEPENDENT_EXACT_JOINT_SOURCE_AUDIT',ranks=ranks,cases=cases,
  binary_sha256=build['binary_sha256'],native_result_sha256=sha(folder/'result.json'),helpers_sha256={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),GEOMETRY,REFS,GEOMETRY.with_name('run.py'),ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py',ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py',ROOT/'TestCases/adaptation/capability/capcheck.py')},
  scope='Complete exported source geometry/tensor-copy/clearance controls only. C++ checks state/actual-query/N-M transport/refusal. No independent flow-state dump, CFD advance, accepted adapted mesh, transfer/restart, viscous or affordability claim.')
 (audit_dir/(folder.name+'.json')).write_text(json.dumps(report,indent=2)+'\n')
 prior=[]
 for p in sorted(itertools.chain(audit_dir.glob('source*n*.json'),audit_dir.glob('joint*n*.json'))):
  d=json.loads(p.read_text())
  if d['binary_sha256']==build['binary_sha256']:prior.extend(d['cases'])
 assert len({c['original_field_signature'] for c in prior})==1
 assert len({c['complete_source_field_signature'] for c in prior})==1
 print(report['status'],ranks,'cases',len(cases),'exact pairs',sum(c['exact_conforming_pairs'] for c in cases),flush=True)
if __name__=='__main__':main()
