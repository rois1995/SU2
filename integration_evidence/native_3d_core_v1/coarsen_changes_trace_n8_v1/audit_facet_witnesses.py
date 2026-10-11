"""Independently check full-star boundary witnesses omitted from the candidate edge star."""
from pathlib import Path
from fractions import Fraction as F
import hashlib,json,re,sys
folder=Path(__file__).resolve().parent;sys.path.insert(0,str(folder.parent))
from audit_cavities import determinant,cross,sub,dot
from audit_coupled import reference_patches
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert json.loads((folder/'analysis.json').read_text())['status']=='PASS'
data=json.loads((folder/'envelope_s1_initial.json').read_text());facets={f['id']:f for f in data['facets']}
for facet in facets.values():
 points=[tuple(F(float(x)) for x in v[1:]) for v in facet['vertices']]
 facet.update(points=points,normal=cross(sub(points[1],points[0]),sub(points[2],points[0])))
 facet['axis']=next(i for i,v in enumerate(facet['normal']) if v)
patches,membership=reference_patches(facets,data['facet_groups'])
def contains(id,p):
 patch=patches[membership[id]];normal=patch['normal']
 return dot(normal,sub(p,patch['origin']))==0 and all(dot(cross(sub(b,a),sub(p,a)),normal)>=0 for a,b in patch['border'])
witnesses=[]
for line in (folder/'attempts_rank0.jsonl').read_text().splitlines():
 event=json.loads(line)
 if event['epoch']!=1 or event['approved']:continue
 match=re.fullmatch(r'Native 3D collapse leaves incident reference facet (\d+)\.',event['reason'])
 if not match:continue
 retained,removed=event['edge'];facet=int(match[1])
 point=next(tuple(F(float(x)) for x in n[1:]) for row in event['old'] for n in row['nodes'] if n[0]==retained)
 assert not contains(facet,point)
 support=[];edge_witness=[]
 for row in event['old']:
  ids=[n[0] for n in row['nodes']]
  for slot,(present,id) in enumerate(row['physical']):
   if not present or ids[slot]==removed:continue
   if not contains(id,point):
    support.append(row['id'])
    if retained in ids:edge_witness.append(row['id'])
 assert support and not edge_witness,(event['round'],support,edge_witness)
 witnesses.append(dict(round=event['round'],edge=event['edge'],reported_facet=facet,full_star_witness_cells=sorted(set(support)),edge_star_witness_cells=[]))
assert len(witnesses)==23
result=dict(status='PASS',witnesses=witnesses,script_sha256=sha(Path(__file__).resolve()),
            input_sha256={n:sha(folder/n) for n in ('analysis.json','attempts_rank0.jsonl','envelope_s1_initial.json')},
            scope='Exact rational containment in the authoritative finite planar marker groups: all23second-coarsening facet rejections have local complete removed-star witnesses outside the candidate edge cells. Candidate-edge cells show no such facet witness. This identifies an incomplete early filter, not an invalid committed mesh or proof that extending it completes the512-round campaign.')
(folder/'facet_witness_audit.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS23 full-star facet witnesses, zero edge-star facet witnesses')
