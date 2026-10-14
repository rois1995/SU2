# Goal3 curved-wall BL: coupled geometry, target and construction

NativeIntegrated, branch `codex/native-3d-bl`. Curved BL is not qualified.
`CBoundaryLayerMetric::FromNativeReference` still explicitly rejects analytic
wall authority. That rejection stays until the connected path is ready.

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
