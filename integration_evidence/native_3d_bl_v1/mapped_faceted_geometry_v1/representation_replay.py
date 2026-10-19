#!/usr/bin/env python3
"""Exact representation counterexamples; no CFD/meshing or geometry acceptance claim."""
import hashlib,json,struct
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def next_positive(x):
 assert x>0
 return struct.unpack('<d',struct.pack('<Q',struct.unpack('<Q',struct.pack('<d',x))[0]+1))[0]
def ideal(a,b,t):return tuple(F(x)+F(t)*(F(y)-F(x)) for x,y in zip(a,b))
def main():
 # A chart that uses two separately rounded weights reintroduces exact-edge failure.
 rows=[]
 for t in (.1,.3):
  residual=F(1-t)+F(t)-1
  assert residual and (1-F(t))+F(t)==1
  rows.append(dict(t=t,rounded_complement=1-t,exact_rounded_pair_residual=str(residual),exact_dependent_coefficient_residual='0'))
 # Distinct latent parameters can give the same valid non-endpoint solver coordinate.
 a=(1e16,1e16,1e16);b=(1e16+8,1e16+16,1e16+24);t=.3;s=next_positive(t)
 x,y=ideal(a,b,t),ideal(a,b,s);px,py=tuple(map(float,x)),tuple(map(float,y))
 assert x!=y and px==py and px not in (a,b)
 alias=dict(a=a,b=b,parameters=[t,s],solver_point=px,ideal_points=[[str(v) for v in p] for p in (x,y)],
  interpretation='Coordinates do not uniquely encode the original parameter. This alone does NOT disprove a separately declared, deterministic canonical inverse geometry policy.')
 # Reuse the actual saved exact-feature obstruction, without rerunning its solver.
 path=HERE.with_name('affine_initial_adoption_v1')/'annulus_v1_n1/exact_feature_obstruction.json';old=json.loads(path.read_text())
 a,b=old['points'];p=ideal(a,b,.5);q=tuple(map(float,p));error=sum(abs(F(v)-w) for v,w in zip(q,p))
 assert old['exact_interior_binary64_point_count']==0 and any(F(v)!=w for v,w in zip(q,p)) and 0<error<F(1e-13)
 # Correct the review's normal offset to epsilon/2, so the stated error really fits.
 epsilon=F(1,2**40);delta=F(1,2**50)
 parameters=[(F(1,4),F(1,4)),(F(1,4),F(1,2)),(F(1,4)-delta,F(3,8))]
 def orient2(p):
  a,b,c=p;return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
 physical=[(F(1024)+u,v,F(0)) for u,v in parameters]
 actual=[tuple(F(float(x)) for x in point) for point in physical];actual[2]=(actual[2][0],actual[2][1],-epsilon/2)
 maximum=max(sum(abs(x-y) for x,y in zip(a,b)) for a,b in zip(physical,actual));assert maximum<epsilon
 recovered=[(point[0]-1024,point[1]) for point in actual]
 assert orient2(parameters)>0 and orient2(recovered)==0
 a,b,c=actual;d=(F(1024),F(0),F(1));u=[b[k]-a[k] for k in range(3)];v=[c[k]-a[k] for k in range(3)];w=[d[k]-a[k] for k in range(3)]
 volume6=u[0]*(v[1]*w[2]-v[2]*w[1])-u[1]*(v[0]*w[2]-v[2]*w[0])+u[2]*(v[0]*w[1]-v[1]*w[0]);assert volume6>0
 latent_loss=dict(epsilon=str(epsilon),maximum_exact_L1_displacement=str(maximum),ideal_parameter_orientation=str(orient2(parameters)),recovered_orientation='0',actual_positive_tetrahedron_determinant=str(volume6),
  actual_face=[[str(v) for v in p] for p in actual],ideal_parameters=[[str(v) for v in p] for p in parameters],
  scope='Local forward-geometry/positivity witness only; not a global embedding, quality, metric, or accepted CFD mesh certificate.',
  review_correction='Use normal displacement epsilon/2; the review used epsilon, whose extra tangential error exceeds its claimed displacement bound.')
 report=dict(status='EXACT_REPRESENTATION_COUNTEREXAMPLES',dependent_coefficient=rows,inverse_parameter_alias=alias,projection_recovery_loses_valid_parameter_orientation=latent_loss,
  saved_oblique_edge=dict(edge=old['edge'],realized_midpoint=q,exact_L1_rounding_error=str(error),strict_literal_subdivision_possible=False,
   explicit_mapping_contract_needed=True,evidence_sha256=hashlib.sha256(path.read_bytes()).hexdigest()),
  implementation_decisions=['Never round dependent barycentric coefficients before topology predicates.','Persist a parameter binding OR specify and certify a canonical inverse policy; distinguish these geometry models.','Retain the strict old case as negative evidence; map errors require explicit opt-in and original-source envelope admission.'],
  limitations=['Negative algebraic evidence only; not a complete mapped geometry, accepted mesh, transfer/restart or performance result.'],script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (HERE/'representation_replay.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
