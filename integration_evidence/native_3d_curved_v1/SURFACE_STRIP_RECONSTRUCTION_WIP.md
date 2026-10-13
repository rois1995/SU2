# Coordinated surface strip — implementation in qualification

Repository: SU2_NativeIntegrated, branch codex/native-3d-curved-local. Goal 2 remains
ACTIVE and all four full completion gates remain PENDING.

The previous bounded one-/two-apex and isolated two-triangle proposals did not
resolve the tilted-cylinder operator stall. The next family retains existing
volume topology, splits a physical edge, and simultaneously moves several wall
vertices and any admitted volume-transition vertices. Every moved vertex requires
its complete incident cell star, so the artificial patch interface stays fixed.

surface_strip_replay_v1 found a floating proposal on the latest retained
recovery_collapse_v1 serial rejected working mesh: four wall vertices, 27 old to
29 new tetrahedra, no free interior vertices needed in this particular proposal.
The local geometry deficit decreases from 1.0619735667340762e-4 to
9.880625821437209e-5 (about 6.96%). Minimum metric mean ratio is 0.2040545676,
minimum scaled Jacobian 0.0550986176, maximum metric edge about 1.0. Frozen
sensor 0.5I, q >= 0.20, J >= 0.05, edge <= 1.8 and surface tolerance 0.04R
are unchanged. This is local progress, not completion of the whole mesh's geometry
requirements or proof of arbitrary feasibility.

SplitSurfaceStrip implements the geometric transaction in CNativeBoundary3D:
at most eight moves with fresh reserved identities, two same-chart analytic
curved split faces, incident-feature projection (disk first), fixed artificial
interface, positive inherited tetrahedra, complete chart-oriented children and
existing prescribed-interface/manifold validation. Output swaps only on success.
The new point can lie off the old edge; the full oriented surface fan is checked.
The established single-vertex movement reuses the incident projection helper.
No sensor interpolation, extension construction or acceptance limit is changed.

The saved regression uses the real immutable original and predeclared exterior
source cells from the blocked event, not an enclosing substitute donor. It checks
actual centroid/edge metric progress and coverage, useful geometry progress, exact
pairwise candidate conformity and clearance against all unchanged cells, and
atomic rejection for incomplete dependencies, duplicate identities and finite
chart/feature departure. The bounded scientific search uses SciPy only outside
production; no new SU2 dependency is introduced.

Qualification and failures:

- surface_strip_native_v1 builds, but the saved 27-cell union is not a manifold
  cavity: edge (0,9) has four incident cavity-skin triangles. Original tested
  fixture/diff and failed receipt remain preserved.
- surface_strip_replay_v2 closes that edge link by admitting only cell189732.
  The patch has 28 old/30 new cells, within unchanged 64/128 limits. The extra
  cell is unchanged, with q0.383/J0.152, and adds no physical face.
- surface_strip_native_v2 builds and passes six of seven selected serial tests.
  The new strip passes native geometry/topology, then rejects actual source
  edge (0,200000): uncovered original/extension interval. MPI2/4 were NOTRUN
  after the serial failure; no replicated/distributed qualification follows.
- The independent corrected audit passes exact positive volumes, candidate-pair
  and all unchanged-neighbor overlap tests, whole-domain manifold skin and fixed
  artificial coordinates. Actual centroid quality is satisfactory, but moved
  cap-rim vertices are not all covered by the immutable source. Status is
  FAIL_FROZEN_SOURCE_COVERAGE. No accepted grid or CFD state is produced.
- cap_gap.py exact rational clipping for the scientific proposal finds an edge
  (0,12) uncovered tail with physical length about1.05e-18. Native reprojection
  can differ by ULP; this number does not quantify the native edge's gap.
  Radial cylinder frusta and skipped disk faces do not certify the floating-point
  cap envelope. Do not relax containment or append coverage after failure.
- The inspector grid surface_strip_replay_v2/cylinder_axis1_strip_proposal.su2
  contains the whole203-to205-cell working mesh. It is explicitly an unaccepted
  proposal with incomplete metric coverage, not a simulation-ready output.

The first independent auditor had incorrect split-parent skin bookkeeping and
then a relative script path error. Initial sources/tracebacks are preserved;
corrected audit and hashes are authoritative. No full-field certificate is green.
All owned jobs are terminal. No broad regression was repeated.

Next substantial connected step: generate a conforming cylinder/cap exterior
collar before freeze, with consistent shared nodal identities and finite declared
projection envelope. Independently overlapping cap/side extrusions would create
same-priority interpolation ambiguity and are not an adequate fix. Preserve
original-P1 priority and original boundary tensors. Recheck the saved native strip
under freshly generated source; do not retroactively change the saved target.
Then add bounded native placement without scientific optimizer dependency and
union import/reservation/link closure in the engine, actual-query validation and
atomic publication. Retry full tilted-cylinder cycles and connected MPI/CFD
controls. Never inject saved coordinates into production or redefine the full
objective around the replay. No cluster, performance, general CAD, production3DBL
or full-physical-unsteady qualification is claimed.
