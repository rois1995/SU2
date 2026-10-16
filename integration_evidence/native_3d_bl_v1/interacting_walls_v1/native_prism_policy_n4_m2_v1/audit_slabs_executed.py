"""Exact saved-data coverage check for the declared unit-box planar policy case."""
import hashlib,itertools,json,re,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
audit=HERE/'independent_policy.json';a=json.loads(audit.read_text());assert a['status']=='PASS_LIMITED_SCOPE'
p=HERE/a['frames'][0]['file'];data=json.loads(p.read_text());nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']}
levels=sorted({v[2] for v in nodes.values()});assert levels[0]==0 and levels[-1]==1
index={z:i for i,z in enumerate(levels)};volumes=[F(0)]*(len(levels)-1)
for c in data['cells']:
 points=[nodes[n] for n in c['nodes']];at=sorted({index[v[2]] for v in points});assert len(at)==2 and at[1]==at[0]+1
 v=determinant(points)/6;assert v>0;volumes[at[0]]+=v
assert all(v==b-a for v,a,b in zip(volumes,levels,levels[1:]))
config=HERE/'policy_m2.cfg';height=float(re.search(r'ADAP_BL_FIRST_HEIGHT= \(([^)]+)\)',config.read_text())[1]);assert abs(float(levels[1])/height-1)<1e-10
edges={tuple(sorted((c['nodes'][i],c['nodes'][j]))) for c in data['cells'] for i,j in itertools.combinations(range(4),2)};wall=[n for n,p in nodes.items() if p[2]==0]
for n in wall:assert any(p[:2]==nodes[n][:2] and p[2]==levels[1] and tuple(sorted((n,k))) in edges for k,p in nodes.items())
out={'status':'PASS_EXACT_COMPLETE_SINGLE_WALL_NORMAL_SLAB_COVERAGE','native_configuration_sha256':sha(config),'independent_mesh_audit_sha256':sha(audit),'mesh_sha256':sha(p),'helper_sha256':sha(Path(__file__)),'normal_intervals':len(volumes),'first_height':height,'wall_normal_first_edges':len(wall),'slabs':[{'lower':str(a),'upper':str(b),'exact_volume':str(v)} for a,b,v in zip(levels,levels[1:],volumes)],'scope':'Every positive cell spans consecutive front planes, and every slab has exact full unit-area volume. Reuses the independently proven embedding/finite geometry/no-overlap certificate. Normal first edges exist at every wall node. This declared planar class only; no centroid orthogonality or viscous accuracy claim.'}
(HERE/'policy_front_coverage.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'])
