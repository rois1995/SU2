# Goal3 — actual curved RANS/native BL lifecycle

Branch `codex/native-3d-bl`, qualified source-refresh baseline9bd5fb76e4.
Full Goal3 ACTIVE/incomplete. The current positive projective annulus construction
is under build and UNQUALIFIED. Earlier v4/v5 variants pass the one-prism independent
integral and sphere controls, but actual RANS rotated-cell and expanded all-sector
checks expose further projection/subtractive-chord issues. Do not reuse those earlier
passes as proof of the current implementation or whole workflow.

Build71336 terminated on a missing direct test include for CompensatedCross2D; the
production objects compiled. The include is corrected; final j1 rebuild75373 is live.
Verify the actual live handle/process before any restart. Current log `integration_evidence/build-integrated-v2/native-curved-rans-positive-coordinates-rebuild.log`.
Next v7 controls must pass all sectors/wall-first permutations, independent prism,
sphere and rotated/sheared polynomial moments. Only then retry RANS n1/m1 v4.
[Preserved source/binary/helper evidence and status](../integration_evidence/native_3d_bl_v1/region_construction_v1/annular_integration_progress.json).


## Missing capability

Accepted frozen-metric meshes and actual geometry/source refresh do not establish
that CFD can evolve, preserve flow/turbulence histories, restart and adapt again.
Reuse TetraDriver, existing conservative transfer/projection accounting, native
reference persistence, SU2/CGNS roundtrip and the existing lifecycle restart helper.
No new remesher modes, acceptance thresholds or solver-specific topology dispatch.

The hidden `NativeCurvedRANS3D` control starts from the same24-cell cylindrical
Euler-type mesh with RANS/SA, smooth cylinder/disk authority, Mach.2/Re1000 and an
axial profile. It takes actual dual-time steps, computes the pressure sensor with
noise0 and existing complexity/gradation policy, constructs the BL through the full
backend and replaces/transfers the mesh. It writes actual solution/restart files,
roundtrips SU2/CGNS, then restarts with a sensor-only adaptation request while
retaining the RANS wall physics. Further actual steps refresh the pressure metric,
remesh the actual current geometry and advance again after transfer.

## Acceptance

- Whole remesh COMPLETE with unchanged geometry, h.05/growth1.2/thickness.2/fade,
  q>=.20/J>=.05/full length<=1.8,4096-work/300s per event.
- Actual flow changes on each solved mesh; finite positive density/internal energy
  and finite solver-admissible SA current/n/n-1 arrays.
- Remesh leaves donor flow/SA unchanged; current/BDF conservative projection ledger
  closes all12 projected fields, including SA, with declared geometry fill/loss.
  This is open-domain accounting when surface discretization changes, not a claim
  of identical old/new geometric volume or zero boundary flux.
- Returned nodal sensor matches original sensor-only P1/extension at actual points;
  original geometric authority survives replacement/output/restart.
- SU2/CGNS grids reload with matching signatures; actual restart restores flow and
  all SA histories by coordinate, then further RANS steps succeed.
- Serial first, then MPI2/4 N=M/M<N after complete serial workflow acceptance.
  Check source/build/binary/helper provenance; preserve rejection and current state.

Case root: `integration_evidence/native_3d_bl_v1/region_construction_v1/`.
- `native_curved_rans_v1`: inconsistent test HMIN configuration refusal.
- `native_curved_rans_n1_m1_v1`: corrected request; actual RANS step0, then coarse
  geometric complexity failure in the original parallel-chord method.
- `native_curved_volume_controls_v1`: rejected axis-fan experiment; no completed
  cylindrical reference comparison and no full runtime qualification.
The focused collector saves exact provenance, donor/accepted geometry, flow/SA traces,
projection ledgers, actual solutions/restarts and per-stage walltimes. WRT_PERFORMANCE
retains existing native section profiles. Host contention is present: these are
correctness observations, not scaling or lifecycle affordability conclusions.

This short pipe pilot cannot establish converged wall shear/forces, M6 accuracy,
high-ratio multiple fronts, general exterior/multiwall geometry or efficient parallel
region construction. Those remain full Goal3 gates.

The initial configuration refusal is preserved with its source/log receipt. Matrix
cases use `native_curved_rans_nN_mM_v1` and pin the selected worker count separately
so completed combinations can be retained and reused. No gate changed.


## Rejected integration strategy

The runtime failure precedes remeshing: coarse curved tetrahedra exhaust the generic
parallel-chord retained-point bound during actual BL complexity construction. Frozen
sensor engine tests bypassed that stage. Extending the positive sphere fan to a
certified point on the cylinder axis also exhausts the same point budget. It is
preserved, not promoted. Do not add another fallback or raise the bounds.

## Connected design under qualification: positive cylindrical annulus integration

Use the existing convex tetrahedron entry/exit-face projection partition, with chords
parallel to the authoritative cylinder axis. For a single complete cylindrical wall
and cells inside its finite axial interval, the BL distance is radial. In each
projected triangle, use polar coordinates about the projected cylinder axis; split
angles at triangle vertices and at triangle-edge intersections with every requested
radial BL band circle. This exposes thin support before density probes.

For each angle, intersect its ray with the triangle to obtain the positive radial
interval, then split at the actual geometric radial levels. Entry/exit face
barycentrics give the axial chord; axial interpolation gives original-tetrahedron
barycentrics for each sample. The positive Jacobian is `r * axial_length`; integrate
angle, bounded radius and axial fraction with directional error estimates and refine
the largest unresolved component. Retain sensor-dependent rules only when that
sensor/core is unchanged. Keep65536 points/16384 work and the existing1e-4 volume
error criterion; do not alter target composition/fade, source P1, geometry or mesh gates.

Reuse existing projection/clipping, EdgeBreakpoints/SamplePoints/SetBLSourceQuery and
actual density callbacks. General/multiwall/rim-outside-slab integration keeps its
existing separately declared limits; this change must not imply it is solved.

Acceptance: independent polar integral of the complete coarse triangular prism,
positive weights/valid source barycentrics/exact volumes, unchanged spherical control,
then the complete actual curved RANS cycle under original mesh constraints. Only
then MPI1/2/4 N=M/M<N, one resumable combination at a time. A local integration
improvement alone is not the runtime milestone. The prepared cylinder oracle is
currently unqualified because the rejected rule threw before completing it.


Implementation now reuses the existing projected face-pair charts, physical edge-band
roots, actual SamplePoints/ApplySample density and representable original-cell source
queries. Tensor-product3/5 probes estimate each directional density and volume error;
retained3-point rules keep weights positive. The largest unresolved direction is split
under the same bounds. Summed volume errors must be<=1e-10 of the original volume,
and actual volume closure<=2e-10; density error remains<=1e-4. No field relaxation.
Preserved stages include v2 angular-duplicate refusal, v3 completion with the incorrect
unit-axial reference, v4 corrected oracle PASS and RANS n1/m1 v2 rotated-cell refusal.
Next focused folder is `native_curved_volume_controls_v5`, followed on success by
`native_curved_rans_n1_m1_v3`. Prior failures/source/binary/helper receipts remain
immutable. Absolute-angle subtraction is replaced by compensated ray predicates and
local positive angular spans; exactly radial projected edges contribute only their
endpoint rays, avoiding redundant circle-root angles.


## Current geometric construction and exact failure evidence

A standalone call to the existing Polygon::Clip shows self-clipping identical
triangles moves/duplicates vertices in5 of8 rotations; sector2 moves the original
axis point to about(1.435e-17,-8.786e-34). Source/output/header hash are preserved in
`native_curved_rans_n1_m1_v3/projection_diagnosis.json`. Shared-base projected triangles
now intersect by exact house orientation tests and the unique tighter-ray apex,
preserving common source vertices for identical and nonidentical projections.
The frame is centered on the cylinder axis, independent of source node ordering.

Further all-sector checks exposed a nonpositive chord from subtracting near-boundary
coordinates. The current design builds sectors with positive blends of geometric
rays, conditioned only by exact powers of two. The angular Jacobian is
`cross(ray0,ray1)/norm((1-t)*ray0+t*ray1)^2`. Closed ray-end barycentric coordinates
and distances from both radial endpoints form positive original-triangle weights;
axial heights interpolate positive chart vertex heights. This avoids subtracting
nearly equal chord values or global angles. Strict CompensatedCross2D protects fma
compensation from fast-math; exact binary high-cancellation sign controls are included.

No tolerance merging, zero-height clamp, budget increase, field reset or acceptance
relaxation.65536 points/16384 work and1e-4 density criterion remain; volume error and
closure retain1e-10/2e-10 bounds. The current implementation still needs execution.
Its advanced-stage clock includes Postprocess's legacy automatic metric calls under
COMPUTE_METRIC=YES/ADAP_LOOP=NO; `solve_seconds` is therefore not CFD-only time.
No lifecycle performance or affordability conclusion may use that raw denominator.
