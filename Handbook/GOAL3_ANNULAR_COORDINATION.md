# Annular coordination: current native result and missing capability

Goal3 and the full [general symmetry scope](GOAL3_GENERAL_SYMMETRY.md) stay open.
Production checkpoint2b48c9dd20da is now compiled and scoped-qualified; no new
CFD physical advance or M6 retry was needed. All owned jobs are terminal.

## Native implementation that worked

Existing coupled shape/edge placement now coordinates an inserted finite-surface
point with admitted smooth physical and interior complete stars. Original sensor
P1 and actual-query BL, fade, height, geometry, quality and full-field gates are
unchanged. Frozen saved-patch replay passes7cases/18743assertions. Actual native
placement accepts in12trials/4linearizations and its independently audited54-cell
replacement has q=.2018085/J=.0502886, exact interface/finite footprint/embedding,
and44.56% changed-edge squared-excess reduction. Unchanged oversized edges remain;
this patch pass does not establish whole-mesh acceptance.

Current N4/M2 Cartesian RANS/SA/current+BDF workflow_v7_n4 independently passes
both strict and relaxed budget profiles, full metric/height/geometry, conservative
transfer, SU2/CGNS/reference output and real restart. This is compatibility evidence,
not MPI qualification of the new offline multi-apex construction below.

## Difficult target still rejected in native code

Unchanged annulus_v7_n1 makes96private edits,339cells/104nodes, then reports
operator_stagnation at2300/8192work. Accepted CFD/BDF/SA states remain intact.
Independent residual reconstruction finds20bad-sizecells and exactly four edges
of length1.8659293, all from the four wall corners to one apex at first height. No
shape failure remains. Empty retired queues do not mean no remaining residual.

The retained four-corner ownership has an analytic length lower bound1.834077>1.8;
tangential movement of that shared apex cannot fix it. This is an analytic-profile
bound for the saved field, not an interval proof of every floating backend query
or whole-target infeasibility. The old single-background centroid constructor also
fails a floating geometry LP: available kernel z<=.15454 versus required z>=.3.
More cavity sweeps or relaxing permitted outer-BL demand cannot resolve these
protected first-height edges under the original retained ownership.

## New complete offline witness, native port pending

Quality-directed compound vertex-star splitting replaces that apex by four and
its36-cell star by52cells, keeping all physical faces and original link vertices.
The complete355-cell/107-node mesh independently passes exact volume, finite hole
coverage, manifold links,7819overlap checks, height0, q=.2167007/J=.0558188 and
full edges<=1.7955165. Original frozen complexity is20.00019535 (relative9.77e-6
above20). Numerical quadrature agreement is empirical, not a certified error bound.

Plain positive partitions and subsequent four-apex movement failed shape checks;
quality-directed connectivity was required in this experiment. One min-cut draft
used overflowing SciPy int32 capacities; its results are explicitly withdrawn.
The corrected bounded version self-checks flow and hard constraints and passes
independent geometry/field auditing. Opus5.5/medium suggested additional vertex
retirement, but the valid witness shows it is unnecessary for this proposal.

Next is one connected generic native multi-apex construction/complete-star
transaction implementation, followed by the unchanged full native target and MPI
lifecycle. Offline coordinates/axes must not become production constructors.
No new accepted native annular mesh, M6/exterior/oblique/junction/2D result or
accepted-event performance improvement is claimed.

[Exact evidence and history](../integration_evidence/native_3d_bl_v1/annulus_coordinated_replay_v1/README.md).
Grid: `integration_evidence/native_3d_bl_v1/annulus_coordinated_replay_v1/fan_split_v1/annulus_frozen_prototype_witness.su2`.
