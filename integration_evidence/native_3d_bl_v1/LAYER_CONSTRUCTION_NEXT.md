# Next connected implementation: prescribed first row and transition

This records an implementation choice, not an implemented or validated capability.
The full Goal3 requirements remain mandatory.

Reuse CNativeCavity3D::Cone and the existing boundary/cavity/dependency/clearance
transactions. First implement a bounded joint wall-plus-transition proposal:

1. Admit a complete cavity containing the wall cells and enough transition above
   their current apices. Reserve all new identities before private reconstruction.
2. Reconnect the complete cavity around an admitted interior background point.
   Its altitude must exceed requested wall height. Existing Cone validation keeps
   the artificial interface and requires positive tetrahedra/star-shaped visibility.
3. Carve each wall-face child from that background fan with its own apex at the
   prescribed altitude. Prefer wall-normal placement at the face centroid; permit
   bounded tangential placement only under explicit alignment criteria. Sequential
   carving of children from disjoint background tetrahedra retains conformity.
4. Repair the transition privately with existing movement/reconnection/insertion.
   Evaluate the unchanged original-P1 sensor plus actual-query BL composition.
   Preserve height during repairs and boundary splits; validate the complete
   proposal before atomic publication. Do not publish a merely positive first row
   if the transition or final frozen metric gates fail.

An existing apex exactly at h0 cannot generally move laterally within its own
wall tetrahedron at the same altitude: that cross-section is a single point.
Replacing wall apices therefore needs the surrounding transition, not a wall-only
split. The background reconnection supplies that freedom without introducing a
second full mesher. A single shared apex is insufficient for a longer wall strip.

Add first-height/normal alignment/coverage diagnostics independently of metric
mean ratio, scaled Jacobian, lengths, geometry and exact conformity. Protect a
valid first row through all operations; BL-to-Euler must release those constraints
and use existing general coarsening without split-history dependence.

Early infeasibility checks must respect sensor coupling. For a constant SPD lower
bound S over an admitted candidate region, a displacement of normal altitude h0
has metric length at least h0/sqrt(n^T S^-1 n). If this exceeds the frozen length
ceiling, that local height/metric request is incompatible. Do not replace it with
a diagonal normal test, overwrite finer sensor requests or call bounded operator
stagnation a proof that no mesh exists globally. Curved geometry error relative
to h0 and interacting-wall clearance need separate explicit checks.

Before production enablement: curved/feature/interacting-wall authority, exact
height/geometry and full-target completion; repeated actual Euler/BL/RANS driver
replacement; MPI1/2/4 dependencies/stale/veto/N=M/M<N; conservative current+BDF/
turbulence/restart/output; independent sensor/composed gradation and lifecycle
costs. Keep the current production BL guard until construction is qualified.

Reference: Prove/Papers/Alauzet2014_ClosedAdvancingLayerMethod_ViscousMeshGeneration.pdf
(published closed-layer reconstruction/moving-mesh ideas); the local joint coning
proposal above is our bounded use of existing native operators, not a claim that
the complete published algorithm or its robustness has been implemented.
