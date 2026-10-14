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
- Native cold construction still FAILS at4096 work,120 private cells/50 nodes:
  main accepted0; coupled topology trials11/accepted0; joint trials4068/private
  moves31/accepted0. The original16 wall triangles remain unchanged in the returned
  rejected mesh. No release or CFD replacement occurs.
- Early streamed work/time exits now retain the real feature/quality sample.
  Bounded rank-local capture saves the exact old/proposed patch and reason; default
  production calls do not write evidence. Latest feature is edge(1,17), cell2243:
  q .16316864070501946, J .033442629663047924, old20/proposed42 cells.
- Before the coupled change, an18/53-cell captured rejection had q .156010/J .028588.
  Exact independent audits of both captures verify positive volumes, no positive-
  volume overlap, oriented incidence and unchanged artificial interface. These are
  partial local geometry certificates, not full smooth-wall/metric/event acceptance.
- Curved perimeter edges of the incident requested wall faces can now be split
  together before normal-row carving. Shared cylinder/disk rim midpoints project
  through all incident authorities. Missing perimeter stars request specific
  dependencies. Node/cell/work bounds and final gates are unchanged. A raw retained
  proposal improves to q .179485/J .045541, but the full cold target still fails;
  perimeter freedom alone does not resolve the transition connectivity.
- Focused shared geometry controls PASS3 cases/1525 assertions, plus the refreshed
  exact planar geometry audit. No new MPI or full regression; the cold failure is
  retained as a required target, not waived by these controls.
- Single-sphere volume oracle PASS22936 assertions. Inflated proxy footprint
  partitioning had2752 charts and failed65536 points; removing it gave5 charts
  but still failed. Positive sphere-centred fans, where the centre is inside/on
  the source tetrahedron, now pass within unchanged16384 subdivision/65536 point
  and error bounds. Original barycentrics and actual-point samples are retained.
  General geometry still uses the chord path. Earlier failures remain archived.
- The visible-wall/hidden thin-sphere oracle still FAILS the16384 subdivision
  bound. It is not qualified; do not infer interacting-layer robustness.

Next: use the reproduced curved patch to coordinate wall distribution and its
transition topology beyond single-edge reconstruction. Do not repeatedly expand
cavities or add placement variants without addressing that construction limit.
The actual frozen target, strict shape/sliver/length gates and atomic rejection
stay intact. Interacting fan/subdivision remains a separate unresolved limitation.
Evidence: `wall_reconstruction_v1/curved_cavity_audit.json` and
`curved_cavity_audit_before_perimeter.json`; input hashes locate the preserved
current/archived captures. `audit_curved_cavity.py` reuses existing exact predicates.

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
