# Primitive runtime integration — work in progress

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-curved-local`, based on Goal 1 `23197eba1715f44d61c9ad3a070f61441dfb0560`.
Goal 2 remains active. All four complete requirements remain pending.

The connected implementation adds arbitrary-axis finite cylinders and authoritative
planar disks for capped-cylinder rim junctions, alongside existing spheres and bumps.
Certified connected same-marker sphere/cylinder/disk components remove artificial
initial facet seams; marker changes, different authority parameters and incomplete
components do not grant unrestricted cross-chart movement. Cylinder projection and
source-shell construction share radial geometry. Disk caps are not independently
extruded: cylindrical side frusta provide bounded exterior coverage at projected rims.
Original-P1 sensor priority, frozen source/parent provenance and actual candidate
queries remain unchanged. No production 3D BL or general CAD capability is claimed.

Reference version 3 stores an explicit axis for every analytic record. Writers retain
version 1 for faceted references and version 2 for sphere/bump-only references;
readers accept all three, reject invalid parameters, and reconstruct immutable raw
facet authority. Existing conservative transfer, CFD rebuild, SU2/CGNS writers and
BDF restart paths are reused.

Geometry-driven selection keeps alternate surface-edge proposals, ordered by
estimated complete-face error reduction. Complete-star positive connectivity,
geometry/feature containment, strict metric quality/sliver/edge checks and current
mesh clearance remain authoritative. Sphere error now uses the finite triangle when
the perpendicular plane foot is provably outside; ambiguous cases retain the
conservative plane bound. Negative controls preserve the arithmetic envelope.

Geometry-only repairs require independent actual geometric-excess reduction in the
engine plus strict fresh shape/sliver and nonincreasing changed-edge metric excess.
They no longer need an unrelated metric-size or quality improvement. Curved repair
search rejects an oversized complete star without abandoning other bounded candidates,
using the existing candidate-wise dependency protocol. Protocol/storage failures and
missing dependencies still abort. Cavity cap 64, replacement cap 128 and global
work/memory bounds are unchanged. Planar candidate behavior is unchanged.

Frozen fixture targets: sphere surface error 0.22R, cylinder error 0.04R, metric 0.5I,
mean ratio >=0.20, scaled Jacobian >=0.05, metric edge <=1.8, maximum work 4096.
The tolerances were selected before the runtime experiments; no acceptance gate was
loosened in response to failure. Two events include actual CFD replacement, current
and both BDF histories, real SU2 restart and SU2/CGNS output/readback. These are
prescribed-metric primitive controls, not solved primitive CFD/shock validation.

## Evidence and remaining failure

- `primitive_runtime_v1`: retained fixture-build failure.
- `primitive_runtime_v2`: all serial primitive variants rejected.
- `primitive_runtime_v3`: axis-aligned cylinder completed; tilted cylinder and sphere rejected.
- `primitive_runtime_v4`: axis-aligned cylinder and sphere completed both cycles;
  tilted cylinder rejected. Sphere ends with 26 tetrahedra instead of the earlier
  rejected 128-cell equatorial refinement. One finite-triangle bound control passes.
- `primitive_runtime_v5`: rounding estimated geometry gains to 40 significant bits
  did not resolve the tilted cylinder. That experiment was removed.
- `primitive_runtime_v6`: axis-aligned cylinder and sphere complete; tilted-cylinder
  recovery encounters an oversized star. One new synthetic negative check incorrectly
  changed a cached edge length without changing its identity/coordinates; corrected
  to represent an actual changed edge in the next run. Immutable field reuse is intact.
- `primitive_runtime_v7`: build PASS; serial focused matrix FAIL (3 of 4 test cases
  pass). Axis-aligned cylinder and sphere complete both events; tilted cylinder
  accepts 77 private edits then exhausts admitted repairs. Its rejected working
  mesh has 121 cells/62 nodes. The accepted CFD state is retained. MPI2/4 were not
  launched after the serial failure. No broader regression has run for this batch.

Each completed/rejected run retains complete source/build/library/member/binary
pins, logs, source cells, inspectable states, geometry and conservative ledgers.
The independent primitive auditor checks exact positive volume, closure/manifold
orientation, interior overlap, original-priority source-resolved metric lengths,
quality/sliver, transported tensors, actual conservative current/BDF integrals,
positivity, geometry against analytic authority, restart and writer signatures.
The executed corrected auditor is `primitive_audit_v1/audit_primitives_final.py`.
`primitive_runtime_v7_audit.json` PASS_COMPLETED_LIFECYCLES_ONLY qualifies the two
completed serial lifecycles, explicitly listing the failed tilted variant and
unexecuted MPI2/4. Both current/BDF conservative ledgers and real writer/restart
checks pass; transported metric residual is zero. Cylinder minimum q/J are
0.201851/0.060953 and radial error 0.019215R; sphere q/J are 0.428774/0.207107 and
radial error 0.183503R. These are deliberately coarse geometry controls. The first
auditor reused the box-marker default incorrectly; its source and correction
record are retained. No runtime or gate changed for the audit correction.

The latest tilted-cylinder private rejected replay is exported for inspection at
`primitive_audit_v1/primitive_runtime_v7/mpi1/cylinder_axis1_m1_s0_rejected.su2`.
It is not the accepted CFD mesh. The accepted mesh/solution stay unchanged after
rejection. Runtime sphere/cylinder MPI1/2/4 and M<N qualification, negative feature/
component controls and full independent audits still need completion. The earlier
bump conservative/solved-Euler lifecycle evidence remains qualified at its published
source, not automatically at these changed production files. A locally resolved
nonuniform/shock control and broader integration regression remain outstanding.

No cluster, scaling, affordability, converged forces or full physical unsteady
qualification is required or claimed here. Follow `AGENTS.md`: focused debugging
checks, broad regression at a meaningful connected integration milestone.

Next substantial step: replay the saved tilted-cylinder obstruction and implement
coordinated boundary/transition reconstruction with off-edge/interior placement
within the existing dependency and replacement bounds. Further tie handling or
blind cap increases are not a demonstrated fix. Retain the failed matrix while
adding component/feature negative controls and subsequent MPI qualification.
