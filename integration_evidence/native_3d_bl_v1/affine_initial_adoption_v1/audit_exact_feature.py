#!/usr/bin/env python3
"""Exact finite-coordinate feature audit for a saved constant-isotropic original P1 field."""
import gzip,hashlib,importlib.util,json,re,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
LATTICE=ROOT/'integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1/certificate.py'
lattice=module('lattice',LATTICE);controls=module('affine_controls',HERE/'audit_controls.py')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 folder=HERE/sys.argv[1];stem=sys.argv[2];edge=tuple(map(int,sys.argv[3:5]));assert len(edge)==2 and edge[0]!=edge[1]
 source=folder/(stem+'_initial.json');authority=folder/(stem+'_rejected_rank0.native_ref');profile=folder/(stem+'_frozen_profile.json')
 captured=json.loads(gzip.decompress((folder/'executed_source.json.gz').read_bytes()))
 boundary_source=captured['Common/src/adaptation/CNativeBoundary3D.cpp']
 assert 'if (Orientation(v[0].p, v[1].p, v[2].p, p, stats) != 0) return false;' in boundary_source
 metric_source=captured['Common/include/adaptation/CNativeMetric3D.hpp']
 maximum=float(re.search(r'constexpr double MaximumMetricEdge = ([0-9.]+);',metric_source).group(1))
 data=json.loads(source.read_text());names,facets,planes,analytic=controls.reference(authority);assert not analytic
 assert facets=={f['id']:f for f in data['facets']} and names==data['marker_names']
 nodes={n[0]:n[1:] for n in data['nodes']};a,b=[nodes[n] for n in edge]
 incident=[f for f in facets.values() if set(edge).issubset({v[0] for v in f['vertices']})];assert len(incident)==2
 assert all(f['id'] not in planes for f in incident),'Declared geometry needs its own mapping contract, not an exact-facet certificate'
 triangle=[[tuple(map(F,v[1:])) for v in f['vertices']] for f in incident]
 n=[lattice.cross(lattice.sub(t[1],t[0]),lattice.sub(t[2],t[0])) for t in triangle]
 assert any(lattice.cross(n[0],n[1])),'Coplanar facets do not force an exact intersection crease'
 sensor=data['original'][0]['vertices'][0][4];theta=F(sensor[0]);assert theta>0
 assert sensor==[float(theta),0,0,float(theta),0,float(theta)]
 assert all(v[4]==sensor for c in data['original'] for v in c['vertices']),'This audit supports a constant isotropic original field only'
 assert F(json.loads(profile.read_text())['core'])==theta
 lattice.selfcheck();bound=lattice.interval(a,b);g=bound['parameter_denominator_gcd'];assert g<=64,'Finite audit work limit: cannot enumerate the required necessary lattice points'
 possible=[];rejected=[]
 for k in range(1,g):
  exact=[F(x)+F(k,g)*(F(y)-F(x)) for x,y in zip(a,b)];rounded=list(map(float,exact));valid=all(F(x)==q for x,q in zip(rounded,exact))
  row={'parameter':str(F(k,g)),'rounded':rounded,'exact_coordinates':[str(q) for q in exact],'exactly_representable_axes':[F(x)==q for x,q in zip(rounded,exact)]}
  (possible if valid else rejected).append(row)
 assert not possible,'This invocation must establish a no-interior-point obstruction'
 squared=theta*sum((F(x)-F(y))**2 for x,y in zip(a,b));limit=F(maximum);assert squared>limit*limit
 result={'status':'EXACT_BODY_FEATURE_OBSTRUCTION_CONSTANT_ORIGINAL_SENSOR','edge':edge,'points':[a,b],
  'required_feature':{'facets':[f['id'] for f in incident],'markers':[names[f['marker']] for f in incident],'both_undeclared_exact_facets':True,'exactly_distinct_planes':True},
  'lattice_necessary_conditions':bound,'representable_interior_points':possible,'exact_interior_binary64_point_count':len(possible),'rejected_necessary_points':rejected,
  'captured_kernel':{'literal_membership_requires_exact_plane':True,'maximum_metric_edge':maximum,'boundary_source_sha256':hashlib.sha256(boundary_source.encode()).hexdigest(),'metric_header_sha256':hashlib.sha256(metric_source.encode()).hexdigest()},
  'sensor_field':{'constant_original_tensor':sensor,'donor_cells':len(data['original']),'sensor_edge_squared_exact':str(squared),'limit_squared_exact':str(limit*limit),'sensor_length_approximate':float(squared)**.5,'exceeds_length_limit_exactly':True},
  'interpretation':'The exact finite body/farfield intersection must remain covered. There is no binary64 mesh vertex inside the required interval, and its constant original-P1 sensor length already exceeds1.8. Local cavity changes cannot supply a missing representable feature point. A declared geometry/mapping contract is needed before another full unchanged attempt.',
  'scope':['Exact geometry/lattice and exact mathematical original-P1 sensor comparison','Full composed incompatibility follows the required preservation of finer sensor demands; no native interval quadrature or floating tensor-dominance proof is claimed','This does not diagnose initial plane invalidity, budget infeasibility, or all possible declared geometry contracts','The separate first-row background blockage is real; removing it cannot overcome this later immutable-body obstruction'],
  'hashes':{str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),LATTICE,HERE/'audit_controls.py',source,authority,profile,folder/'executed_source.json.gz')}}
 (folder/'exact_feature_obstruction.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],'edge',edge,'sensor length',result['sensor_field']['sensor_length_approximate'],'necessary candidates',g-1,'representable',len(possible),flush=True)
if __name__=='__main__':main()
