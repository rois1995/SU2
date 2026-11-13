"""Post-run mathematical counterexample; no native source, target or mesh mutation."""
from pathlib import Path
import hashlib,json
import numpy as np
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
metric=np.array([[2.,1.],[1.,2.]]);covariance=np.linalg.inv(metric);cap=3.;updated=metric.copy()
# shortcut: individual rank-one constraints do not enforce their sum; use a joint tangent-metric intersection before wider doubly-curved qualification.
for direction in np.eye(2):
    alpha=max(0.,cap-1/(direction@covariance@direction));updated+=alpha*np.outer(direction,direction)
individual=[float(np.linalg.eigvalsh(updated-cap*np.outer(d,d)).min()) for d in np.eye(2)]
joint=float(np.linalg.eigvalsh(updated-cap*np.eye(2)).min())
assert min(individual)>0 and joint<0 and np.allclose(np.linalg.eigvalsh(updated),[2.5,4.5])
source=R.parents[3]/'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp'
result=dict(counterexample_proven=True,base_metric=metric.tolist(),individual_principal_caps=[cap,cap],
 updated_metric=updated.tolist(),individual_constraint_minimum_eigenvalues=individual,
 joint_curvature_envelope_minimum_eigenvalue=joint,updated_metric_eigenvalues=[2.5,4.5],
 implication='Current code preserves the old metric and each individual principal constraint, but can undersize an oblique direction at a doubly-curved point compared with the joint curvature tensor. Analytic sphere capping is stronger.',
 scope='Post-run mathematical limitation, not an attribution of any particular M6 refusal or a construction rerun.',
 next='Before wider doubly-curved qualification, intersect the complete tangent curvature tensor with the existing tangent metric and validate mixed directions. Do not claim current focused tests/reload prove that property.',
 tested_source=dict(path=str(source),sha256=sha(source)),helper=dict(path=str(Path(__file__).resolve()),sha256=sha(__file__)))
output=R/'joint_curvature492_debt.json';assert not output.exists();output.write_text(json.dumps(result,indent=2)+'\n')
print(dict(individual_constraints_pass=individual,joint_envelope_minimum=joint,source_unchanged=True))
