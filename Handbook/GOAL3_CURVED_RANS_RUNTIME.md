# Goal3 — actual curved RANS/native BL lifecycle

Branch `codex/native-3d-bl`, qualified source-refresh baseline9bd5fb76e4.
Full Goal3 ACTIVE/incomplete. The connected runtime check compiles, but the actual
workflow is not accepted. Initial HMIN.1/height.05 config refusal is preserved; current
HMIN.025 admits the request. Actual RANS step0 then fails BL complexity cell0 before
remeshing. A cylinder-axis extension of the existing spherical fan also exhausts the
unchanged65536-point limit in the new cylindrical volume control. All owned jobs are
terminal. That unqualified production change is restored to published9bd5fb76e4;
its exact source/build/binary/log receipt remains in the rejected control folder.
The local build still contains that rejected experimental object: rebuild before any
new run. No mesh/runtime/performance qualification follows from these failures.

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

## Next connected design: positive cylindrical annulus integration

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
