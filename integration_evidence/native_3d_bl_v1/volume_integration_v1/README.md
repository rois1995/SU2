# Goal 3 thin-volume integration — implementation under verification

Working branch codex/native-3d-bl; this is not a qualified3DBL remesher.

The new3D volume rule uses the existing conservative-transfer convex clipper in
original tetrahedron barycentric coordinates. Distance bands isolate first-height,
growth and fade scales before positive degree-two quadrature. Sensors interpolate
on the original cell; ApplySample shares the actual-query intersection/fade policy.
Exact native orientation measures physical volume and clipped piece fractions.
Nearest-face changes are checked at edge/face/cell samples; hidden finite-wall
support uses conservative separating projections and triggers bounded subdivision.
Work/point/depth/polyhedron caps reject explicitly. These are bounded numerical
quadrature checks, not a formal certificate for every possible curved field.

Native ComputeMetric now includes this rule in3D complexity trials. Geometry
samples are reused across scale/gradation trials; transported nodal tensors stay
sensor-only. Preparation has an aggregate2GiB requested quadrature storage limit;
that limit is not an RSS or whole-CFD bound. Construction failures are elected
collectively with the actual global cell ID. Existing geometry preparation timing
and point-count reporting apply to3D as well.

The driver supplies the retained original3D reference to steady/window/PREDICT
metric construction. Initial BL use loads/validates original sidecars or imports a
reference before metric evaluation. Configured analytic BL authorities deliberately
reject pending genuine analytic distance/normal implementation, rather than using
facets as a substitute. Production remeshing still rejects3DBL until first-height
and coordinated transition construction are qualified. General goal scope is intact.

The grouped build and focused checks are terminal. Five stages PASS, but the
whole aggregate is FAIL: hidden finite-wall support exhausts65536points.
Closest-feature support directions resolve the original depth failure; direct
tetrahedral rules avoid redundant fans but do not complete this case. Preserve
the target/caps and address feature-aware integration. See failed aggregate,
per-case receipts and REJECTED_EXPERIMENTS.json. Cases:
microscopic coarse tetrahedron vs independent one-dimensional integral, positive
weights/volume/first and second moments/reversed orientation, production coarse-cube
BL complexity with sensor-only tensors, immutable wall extraction, changing nearest
face and hidden finite-wall support. Existing2D/3D BL kernels are checked after the
connected implementation. Each complete case retains provenance/logs independently.

After the build is current, run from repository root:

```sh
python3 integration_evidence/native_3d_bl_v1/volume_integration_v1/run.py
```

Serial correctness only, sampled CPU/nice19/threads1,180s per independent case.
No CFD timestep, production BL adaptation, MPI/AD runtime or performance claim.
