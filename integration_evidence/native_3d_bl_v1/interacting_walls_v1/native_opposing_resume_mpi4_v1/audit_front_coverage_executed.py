"""Certify every native prismatic height slab from already audited exact mesh volumes."""
import collections,hashlib,json,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
case=HERE/sys.argv[1];auditPath=case/'independent.json';audit=json.loads(auditPath.read_text())
assert audit['status'] in ('PASS_COMPLETE_OPPOSING_CONSTRUCTION_REMOVAL_GEOMETRY_AND_FULL_FIELD_SCOPE_LIMITED','PASS_CONSTRUCTION_ONLY_NATIVE_REMOVAL_REJECTED')
path=case/audit['frames'][0]['file'];assert sha(path)==audit['frames'][0]['input_sha256']
data=json.loads(path.read_text());nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']};levels=sorted({p[2] for p in nodes.values()});index={z:i for i,z in enumerate(levels)}
assert levels[0]==0 and levels[-1]==1;volumes=collections.defaultdict(F);counts=collections.Counter()
for c in data['cells']:
    p=[nodes[n] for n in c['nodes']];at=sorted({index[x[2]] for x in p});assert len(at)==2 and at[1]==at[0]+1
    volumes[at[0]]+=determinant(p)/6;counts[at[0]]+=1
assert set(volumes)==set(range(len(levels)-1))
for i,v in volumes.items():assert v==levels[i+1]-levels[i]
height=F(.02);assert abs(float(levels[1]/height)-1)<1e-10 and abs(float((1-levels[-2])/height)-1)<1e-10
widths=[float(b-a) for a,b in zip(levels,levels[1:])];midpoints=[float((a+b)/2) for a,b in zip(levels,levels[1:])]
lower=[w for w,z in zip(widths,midpoints) if z<=.5];upper=[w for w,z in reversed(list(zip(widths,midpoints))) if z>.5]
growth=max(b/a for sequence in (lower,upper) for a,b in zip(sequence,sequence[1:]))
result={'status':'PASS_EXACT_COMPLETE_NORMAL_SLAB_COVERAGE','independent_audit_sha256':sha(auditPath),'mesh_sha256':sha(path),'helper_sha256':sha(Path(__file__)),
        'normal_intervals':len(widths),'maximum_away_from_wall_interval_growth':growth,'wall_normal_first_row_edges':audit['frames'][0]['wall_normal_first_row_edges'],
        'maximum_centroid_tilt':audit['frames'][0]['maximum_centroid_tilt'],
        'slabs':[{'lower':float(levels[i]),'upper':float(levels[i+1]),'cells':counts[i],'exact_volume':str(volumes[i])} for i in range(len(widths))],
        'scope':'Each positive tetrahedron lies between two consecutive global front planes and exact slab volume equals its complete unit-square area times width. Reuses separately proved no-overlap/finite-domain geometry, so every declared slab including both first bands is covered. This aligned planar class only, not general multilayer, centroid orthogonality, CFD accuracy or affordability.'}
(case/'front_coverage.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
