# Goal3 curved-wall BL: coupled geometry, target and construction

NativeIntegrated, branch `codex/native-3d-bl`. Curved BL is not qualified.
The current connected implementation is being built. Actual queries now support
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
  main accepted0; topology trials8/accepted0; joint trials4071/private moves37/
  accepted0. Wall16 triangles are unchanged; defect.07612 exceeds coupled.025.
  Initial/rejected SU2/JSON/reference grids and config are inspectable. No release
  or CFD replacement occurs. Partial independent geometry checks pass positive
  cells, oriented incidence/skin and exact volume closure; disjointness/full
  metric/curved-height acceptance is not thereby established.
- Single-sphere volume oracle PASS22936 assertions. Inflated proxy footprint
  partitioning had2752 charts and failed65536 points; removing it gave5 charts
  but still failed. Positive sphere-centred fans, where the centre is inside/on
  the source tetrahedron, now pass within unchanged16384 subdivision/65536 point
  and error bounds. Original barycentrics and actual-point samples are retained.
  General geometry still uses the chord path. Earlier failures remain archived.
- The visible-wall/hidden thin-sphere oracle still FAILS the16384 subdivision
  bound. It is not qualified; do not infer interacting-layer robustness.

Next: fix early work-exit diagnosis so it retains the actual failed curved patch,
metric residual and specific dependencies, then give that wall/transition
reconstruction the needed coupled freedom. Current last metric/feature fields are
unset on the cold budget exit: q1/J1 are defaults, not evidence. Do not increase
budgets or add placement variants without the reproduced blocker. Address the
interacting fan/subdivision limitation separately; preserve both complete targets.

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

## Next substantial implementation

Implement these together against a curved complete construction/removal target:

1. Finite analytic geometry authority supplies closest points and normals, with
   marker/component/chart limits. Reuse projection and canonical charts; do not
   silently substitute a facet or extrapolate beyond a chart. Begin with supported
   radial geometry and retain explicit rejection for unsupported queries.
2. The same actual-query curved composition feeds remeshing and complexity. Edge
   band intersections and volume integration must resolve thin smooth regions even
   when endpoint/midpoint or original-facet samples miss them. A point-query extension
   with unchanged facet-band clipping is insufficient. Preserve original P1 sensor
   donors, finer demands, fade and noise0; audit any additional curvature demand
   before freezing rather than retuning a rejected target.
3. Coordinate projected wall children, normal points and transition topology using
   the retained-topology reconstruction and complete dependencies. Relate the frozen
   geometric error to h0; diagnose smooth-wall clearance/height and facet altitude
   separately. Avoid claiming that an independent Hausdorff tolerance gives a usable
   thin first row. Any changed height contract must be explicit and qualified.

Acceptance remains complete geometry/coverage, positive/conforming/disjoint tets,
original frozen-field q/J/full edges, height and normal-foot checks, sensor transport,
atomic refusal and repeat removal. Qualify curved integration against independent
radial references, then MPI/transfer/restart at the connected milestone. Planar
behavior must retain the existing evidence. Thin/feature/interacting layers and
physical viscous accuracy remain separate required Goal3 checks.

No production target, query, geometry or height behavior changed in this experiment.
