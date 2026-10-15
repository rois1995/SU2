# Goal3 curved-wall BL: coupled geometry, target and construction

NativeIntegrated, branch `codex/native-3d-bl`. Curved BL is not qualified.
The connected implementation builds and links. Actual queries now support
certified complete sphere, finite-cylinder and disk components. Partial/graph
charts and markers mixing components still reject; this is not general CAD.

Current source connects: true closest distance and gradient, frozen curvature
refinement in the existing tangent block, preserved normal scale/finer demands/fade,
conservative proxy envelopes, actual radial/finite-rim edge bands, projected-chord
thin quadrature and separate hidden coverage for each wall. Finite-rim uncertainty
and work remain bounded and unresolved cases reject. A rounded closest point is
not used to normalize a different thin distance. Requested curved surfaces use
min(Hausd,h0/2) in both selection and whole-patch progress. First-row checks retain
exact facet altitude/footprints and additionally reject negative smooth-wall
clearance, with facet/cell/apex diagnostics. No sensor-only donor/transport change.

The connected build and links PASS. Both compile errors are preserved in
`wall_reconstruction_v1/curved_build_failure` and `curved_build_failure_2`.
The selector-only build was stopped deliberately for batching. All owned jobs are
terminal. Current stage summary: `wall_reconstruction_v1/curved_summary.json`.

- Field controls PASS4 cases/163 assertions on their recorded binary: real radial
  queries/gradients, finer demands, finite rims, physical height and separate
  gradation. Sensor ratio1; composed fade ratio30.65099 remains uncertified.
- The first cold-cylinder run failed quadrature at edge0,11, from(0,0,0) to
  (.70710678,.70710678,1). Off-wall proxy lookup could switch distinct interior
  closest points. Tangential interpolation now binds to the smooth closest foot;
  that removes the quadrature failure without relaxing target/gates.
- Early actual-field interface probing now precedes the repair sweep. The exact
  remaining14-cell patch misses one tetrahedron from vertex71's star: with that
  point fixed,256 trials leave shape deficit .172760; adding the authoritative
  missing cell resolves the15-cell patch in14 trials (4 removals,3 flips,28 cells,
  q .254308/J .056062). Both replay inputs and independent audits are preserved.
- The previous complete cold run rejects4096 work, but admits all8 retained wall
  reconstructions and210 private edits, compared with5 edits before early probing.
  Final private grid:1026 tetrahedra,260 nodes,166 physical faces. Native diagnostics:
  joint2338 trials/246 moves/173 removals; no release or CFD replacement occurs.
- The saved-grid audit reloads the rejected SU2 file and measures the original
  sensor-plus-BL field without remeshing. All1026 cell shapes pass: qmin .210041,
  Jmin .061128. Of1368 unique actual-target edges,150 exceed1.8; maximum3.860547.
  These remain mandatory rejection gates; this is not an accepted adapted grid.
- Independent exact geometry/exported-tensor arithmetic audit PASS in declared
  scope: positive volumes, no positive-volume overlap (26110 conservative-box
  candidate pairs checked), closed oriented ball skin,48 requested wall faces with
  exact contained normal feet and altitude relative error≤1.67e-15. Native q/J
  arithmetic agrees≤3.34e-16. It does not independently verify the BL composition,
  full composed quadrature, lower-dimensional contacts or CFD/state transfer.
- Reusable diagnostic folders: `curved_replay_remaining`, `curved_replay_complete`,
  `curved_grid_audit`; the complete rejected run is now
  `curved_cold_complete_rejected_v1`, distinct from the newer timeout. Receipts and `curved_grid_independent.json` pin inputs,
  sources, binaries and helpers. The grid audit uses the original120-record source,
  not a transported/coarse spread of the fine wall tensor.
- Five focused controls PASS56 assertions, including zero-trial interface probing.
  No MPI/full regression or physical CFD/performance campaign. Earlier first-patch
 14-trial shape success and all unsuccessful full runs remain recorded.
- Single-sphere volume oracle PASS22936 assertions. Inflated proxy footprint
  partitioning had2752 charts and failed65536 points; removing it gave5 charts
  but still failed. Positive sphere-centred fans, where the centre is inside/on
  the source tetrahedron, now pass within unchanged16384 subdivision/65536 point
  and error bounds. Original barycentrics and actual-point samples are retained.
  General geometry still uses the chord path. Earlier failures remain archived.
- The visible-wall/hidden thin-sphere oracle still FAILS the16384 subdivision
  bound. It is not qualified; do not infer interacting-layer robustness.

The longest saved radial cap edge has actual length3.860547. Exact replay now
compares strict split, star insertion and grown joint reconstruction. Its balanced
binary64 point is not exactly collinear: strict split correctly rejects; star fan
is geometric but fails q .178130/J .046729; joint reconstruction previously rejected
before geometry. Joint same-component boundary reconstruction now permits the
rounded/projected point and relies on its complete oriented cavity checks. Strict
splitting, marker junctions, height, metric and external-clearance gates remain.

Joint cap reconstruction produces geometry and a specific obstructing face.
Following neighboring faces grows2→14 cells, but the common fan still fails shape
(q .104424/J .015465). Changing the new point alone also fails. Full34-cell apex
star admission permits retained splitting, but coarse single-axis moves fail.
Simultaneous cap-point placement and free-apex depth movement passes the native
unchanged-field refinement gate for13 of15 bounded proposals. The saved36-cell
private replacement independently has positive volumes, oriented incidence and
no positive-volume overlap; its artificial interface, volume and original sensor
donors equal the34-cell source. This is private progress, not complete-mesh acceptance.
Evidence: `curved_edge_replay.json`, `curved_edge_geometry.json`, old/grown/joint
JSON and pinned sources/binary. Test-only placement parameters are not production
modes or special handling of this fixture.

For the saved edge, existing production mover selection requires a54-cell
complete edge/mover union, within
the64-cell bound. Eligibility now follows the edited edge's geometry rather than
the domain-wide presence of curved walls; flat caps and interior patches can use
coordinated strips, while curved wall edges retain the separate wall path. Rounded
same-chart insertion also supports simultaneous volume moves. Four focused shared
geometry controls PASS1716 assertions; refreshed independent geometry/normal-foot
checks PASS in their scope. The latest full cold run times out at420 seconds
(return124), with6828 recorded rejection observations and no final mesh export.
These observations are not the engine work counter or unique mesh failures. There
is no release/CFD replacement or end-to-end robustness/performance claim.

Latest initial grid/private rejected pair/log: `curved_cold`. Earlier complete
1026-cell rejected grid/receipt: `curved_cold_complete_rejected_v1`. Saved-grid
residual replay can use that hash-verified complete evidence after an incomplete
export. Do not associate its150 oversized edges with the newer partial run.
Next: production joint layout/selection must realize the valid private repair and
resolve the full cap/wall/interior residual under the unchanged target and gates.
More common-fan growth, extra sweeps or larger budgets are not justified by these
results. Keep representative streamed failures distinct from whole-proposal minima.

Full cold-grid success and independent curved whole-mesh audit, MPI/return/transfer/
restart and physical RANS/performance gates remain required. Partial/graph/mixed
components remain unsupported. External-fluid cold geometry making the normal row
cross its smooth wall still needs curvature resolution before row initialization.
The100-case directional tangent arithmetic check is preserved separately; it is
not a compiled/native mesh certificate. No speedup/scaling/affordability claim.

## What the new diagnostic proves

Evidence: `integration_evidence/native_3d_bl_v1/curved_wall_feasibility_v1`.
The script, twelve-case scale/height sweep and two inspectable one-tet SU2 meshes
are reproducible. They are mathematical diagnostics, not native adaptation output.

For a symmetric spherical wall triangle, plane defect is
`delta = R*(1-cos(theta))`. Its normal-centroid apex has exact facet altitude `h0`
and footprint1/3, but signed clearance to the sphere is `h0-delta`. A30-degree
triangle with R=1 and h0=.001 gives clearance/h0=-132.975. This rejects that
construction; it does not prove the adaptive target infeasible. Boundary refinement
can resolve the geometry. Exact altitude alone cannot establish physical wall height.

Resolving a circular chord defect epsilon requires chord length at most
`2*sqrt(epsilon*(2*R-epsilon))`. In the illustrative constant right-tet metric,
refining tangents that much while retaining the coarse tangential scale can violate
q/J despite perfect normal height. For epsilon=.1*h0 and h0/R=.001, q=.02168 and
J=.00113. A matching tangential proxy yields q=.83995,J=.70711,Lmax=sqrt(2).
The formulas match native Measure; the proxy is not its spatially varying composed
field or a proposed acceptance certificate. The .1 factor is an experiment setting,
not a new production tolerance. Scale invariance passes R=.1,1,1000.

## Next missing construction capability

Complete the wall/transition proposal on the known cold target, including the
newly admitted perimeter stars and collective normal-row connectivity. Geometry,
actual metric and prescribed first height must hold together; the failed proposal
is not proof that the target is infeasible. Retain the broad target requirements.

Curvature resolution before first-row initialization is still needed where a
coarse external-fluid wall would put its normal-row point across the smooth wall.
The complete radial query/field/edge facilities above already exist; do not treat
them as future implementation or repeat their primitive tests as milestones.

Also resolve interacting thin-volume integration within its declared accuracy/work
policy and qualify composed gradation separately. Full geometry/MPI/transfer/RANS/
restart/accuracy/performance gates remain in the requirement audit.
