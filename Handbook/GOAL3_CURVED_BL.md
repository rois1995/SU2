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
- Native cold construction still FAILS at4096 work. Bounded removal of free
  interior vertices now joins flips/movement, retaining the actual frozen field,
  fixed interface, current prescribed-height apices and complete-star geometry.
  Each removal consumes the existing trial budget; no limits/gates were raised.
- The exact saved20-cell blockage is replayable with the original reference and
  freshly imported120-record sensor/extension source. Whole-proposal samples show
  q .048290/J .005052, worse than the engine's first failing16-cell batch sample.
  Prioritizing removal resolves that patch in14 trials:4 removals,3 flips,33 cells,
  q .240152/J .056062 and zero shape deficit. This is private shape success, not
  full composed-edge or whole adaptation/solution-transfer acceptance.
- The independent replay audit verifies positive volumes, no positive-volume
  overlap, unchanged artificial interface and q/J arithmetic from the exported
  actual-query tensors. It does not independently qualify the BL field itself.
  Evidence: `curved_replay.json`, `curved_replay_audit.json`, and the exact fixture
  origin in `curved_replay_fixture.json`. Current native replay receipt is tied to
  its recorded binary; subsequent diagnostic-hint changes are not silently covered.
- Resolve an explicit retained-wall geometry obstruction before spending repair
  trials on incomplete fallback fans. With prioritized retirement/closure, the
  full cold target reached6 admitted private edits before rejection. Whole-patch
  shape deficits now also identify a fixed, nonphysical interface vertex whose
  complete star can be requested; with that hint the latest run reaches5 private
  edits. This is not evidence of a general improvement from the hint.
- Latest cold diagnostics: topology34 trials/5 accepted; joint4040 trials,
 119 private moves/58 removals/5 admitted repairs. Last feature edge(4,14),
  cell7359, representative q .146533/J .021848, old14/proposed31 cells. Representative
  streamed failures are not the whole-proposal minima. Initial/rejected whole
  grids, source/binary/config provenance and earlier failures remain inspectable.
  No release or CFD replacement occurred.
- Five focused controls PASS52 assertions: flips, private interface bounds,
  interior sliver resolution, prescribed-height protection and artificial-interface
  obstruction identification. No MPI/full regression or physical CFD campaign.
- Single-sphere volume oracle PASS22936 assertions. Inflated proxy footprint
  partitioning had2752 charts and failed65536 points; removing it gave5 charts
  but still failed. Positive sphere-centred fans, where the centre is inside/on
  the source tetrahedron, now pass within unchanged16384 subdivision/65536 point
  and error bounds. Original barycentrics and actual-point samples are retained.
  General geometry still uses the chord path. Earlier failures remain archived.
- The visible-wall/hidden thin-sphere oracle still FAILS the16384 subdivision
  bound. It is not qualified; do not infer interacting-layer robustness.

Next: assess the remaining curved wall/transition block from the saved full target;
point retirement and explicit closure resolve the first exact patch but do not yet
complete the event. The interface-deficit hint is plausible, not qualified as a
better construction policy. Preserve its comparison against the6-edit predecessor.
Do not infer infeasibility or increase limits from local search failure. Interacting
fan/subdivision remains a separate unresolved limitation. Exact local captured
rejection audits remain in `curved_cavity_audit*.json`; input hashes locate the
preserved current/archived captures.

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
