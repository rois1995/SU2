# Surface freedom and recovery investigation — incomplete

Current repository SU2_NativeIntegrated, branch codex/native-3d-curved-local.
Goal 2 remains ACTIVE/all four full requirements PENDING. Fixed source/geometry,
q0.20/J0.05/metric edge1.8/surface error0.04R limits were not relaxed.

- surface_swap_replay_v1: no admissible immediate geometry-progress swap; zero
  volume candidates tried. This prefilter result does not establish that surface
  reconnection followed by another operation is impossible.
- surface_fan_replay_v1: opposite-diagonal projected wall point plus interior
  reconstruction tried ten growing patches, but all retained very thin cells.
- frontal_fan_replay_v1: jointly optimized cylinder angle/height and interior
  position, finding a floating-point q0.212375/J0.053244 candidate. Its reported
  local geometry deficit reduction is only about1e-12 relative, essentially the
  existing progress threshold. It is not treated as a useful robust repair,
  accepted production mesh or exact conforming/source/conservation certificate.
- frontal_fan_replay_v2: stricter experimental >=1% local geometric-deficit
  reduction, otherwise unchanged target/gates; no proposal in560 bounded trials.
  This is a limited fan/patch/search family, not global infeasibility proof.

Production changed only to let the existing streamed vertex-star recovery try
CollapseVertex after its movement trials. Previously that recovery path tried
movement only; coarsening waited until refinement completed. The complete vertex
star is already imported/reserved. Existing feature/reference containment, positive
inherited connectivity, actual frozen-field quality/sliver/changed-edge progress,
nonincreasing geometry error, exact clearance and atomic publication remain.
This applies to streamed repair; planar direct-field path is unchanged. No target
regularization, metric interpolation, geometry reference or acceptance relaxation.

recovery_collapse_v1 builds successfully and passes four of five selected serial
cases. Tilted-cylinder COMPLETE expectation still fails on operator stagnation;
accepted CFD state retained. Its source/build/member/binary pins, logs and rejected
working mesh are saved. MPI and broader current-source regression for this change
remain outstanding; the previous completed MPI observation predates this change.
No performance/cluster/generalCAD/production3DBL claim and no full gate achieved.

All four floating-point probes were sequential, nice19/oneCPU/threads1 with bounded
old/new cell count and hard300s limits. The native check used the existing bounded
controller. All handles are terminal; no owned heavy job remains live.

Next major step: coordinated short physical-surface strip reconstruction and its
volume transition, permitting several connected wall triangles to be rebuilt
rather than an isolated two-triangle fan. Preserve marker/feature/component
boundaries and complete versioned MPI dependencies; evaluate every actual new
centroid/edge from the same frozen original/declared source. Do not continue tuning
single-fan placement or blindly increase caps. Completed-state MPI audits, full
integration regression, genuine solved-Euler refresh and resolved nonuniform/shock
control remain part of the original Goal 2 completion requirements.
