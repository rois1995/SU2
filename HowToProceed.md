# How to proceed

Updated2026-11-08. **Zero accepted meshes.** Read [HANDOFF_Codex.md](HANDOFF_Codex.md)
for actual owned handles, source/build provenance and current frozen work. Historical
build numbers never override it.

## Current result

The complete same-seed M6 attempt262 refused its first event at the unchanged
native time limit. It reached shape/size closure at complete censuses but stalled
on four leading-edge geometry failures. Its final734315-cell state has four
unknown q/J scores; do not substitute an earlier clean census for final closure.
All thirty raw files are complete/sealed; source/runtime/input pins were verified
before the one recorded owned stop. All old solver and inspection handles are
terminal. Never repeat an old stop.

The latest complete inspection mesh is
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/inspection/rejected_mesh.vtu`.
It has148608points/734315tetrahedra, captured nodal metric9inPointData and correctly
sized q/J/geometry CellData. Four q/J values are explicitly unknown. Independent
readback checked SPD tensors, positive volumes and VTU/SU2 connectivity/coordinate
agreement. This is a rejected inspection mesh, not accepted CFD input. Save another
full inspection only after meaningful implementation progress; no per-trial VTUs.

## Confirmed diagnosis and remaining uncertainty

- Same saved real-CFD seed, Mach-only L1/50k. Broad sampled surface compression
  bands and requested transverse spacing4–7%chord versus existing3–4%triangles
  explain little first refinement there. Integrated forces do not prove resolved
  shocks. These cuts do not identify every lambda-shock branch.
- Local regularization219 and condition-admission254 corrections remain integrated.
  The latter changes71Hessians without changing the saved flow. Farfield40.398%
  of target complexity remains physically unqualified; weak curvature over a huge
  volume is not automatically numerical noise. Do not change norm/sensor/complexity
  without evidence.
- Smooth leading-edge bands have exact production metric edges around.13–.19,
  mostly below.5, with8–15times the original triangle density. Fixed boundary
  coordinates are not the explanation for these sampled mobile smooth regions.
  Geometry-required spacing versus avoidable insertion/limited cleanup remains
  incompletely separated. Do not impose a blanket minimum-edge gate.
- Terminal262's late four-failure windows alternate split and quality-repair states.
  485commits retire no net geometry failures;244211upper queries are exact memo
  hits with zero new traversal. This blocker is repair power/construction cycling,
  not fresh certification speed or a reason for another baseline campaign.

See [target diagnosis](Handbook/ADAPTATION_TARGET_DELIVERY253_20261107.md),
[complete capture](Handbook/ADAPTATION_CAPTURE261_20261108.md) and
[source-faithful cavity checks](Handbook/ADAPTATION_CLOSURE263_20261108.md).
CDT3D is a design reference: its criteria differ from ours, recovery L2 projection
is distinct from the Lp objective, and its iterated result is not first-adaptation50k.

## One active connected correction

A globally checked complete42-cell current cavity contains all four geometry
failures and three unknown q/J cells. No support bound was enlarged. Existing
collapse/relocation alone finds no useful closure. Existing split8184–205050218
plus one boundary quality move reduces failed physical faces/cells4→3 with zero
q/J failures across the whole affected context. Production current-source metric
transport/rebasing and publication tensors agree; strip/ring connectivity and
coordinates agree. Nonlocal contact/publication and actual mesh closure are
separate obligations.

Connected repair266 compiled;64of65serial cases passed. Its new geometry-repair
search exhausted a4096-work phase-return fixture. That failure is preserved, not
waived, and no266MPI/M6 ran. Correction267 restricts this new path to failed-face
edges and requires raw/final failed-face retirement without increasing geometry-bad
cell count. Strict excess descent, shape/size/heights/contact gates remain. Support
is imported completely before the existing disjoint/versioned reservation; existing
operators,160work,node ranges,memory accounting and publication are reused.

Build267 and serial65cases/56523assertions passed, including the previously
failing phase return and actual useful cavity. The ONE MPI6 qualification is
currently frozen/running. See
[current design and failed qualification](Handbook/ADAPTATION_COUPLED_REPAIR266_20261108.md).
Require the actual-cavity candidate plus the previously failing phase-return check,
then ONE MPI6 milestone for conflicts/empty ranks/curves/features/heights/census/
ownership/cache equality. If qualified, immediately run full same-seed two-event
M6; judge whole-field q/J/geometry counts and useful work, not commits/debt alone.
If it fails, preserve evidence and fix its demonstrated blocker inside this cycle.

Keep validity-first construction unchanged. Quality-aware coarsening/splitting,
larger cavities, a different scheduler/geometry representation and mandatory
curvature demands remain unproven alternatives in [the backlog](Handbook/ADAPTATION_BACKLOG.md).
One owned heavy job, low priority, single-threaded libraries, source/runtime/input
freezing and resource receipts apply throughout.

## Completion

Two strictly accepted Euler meshes at50k metric complexity; actual point/cell
counts reported separately. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and requested
heights stay strict. Require conservative admissible transfer at every field/time
level (true CV residual<=1e-10, scaled target-integral defect<=1e-12), finite
continued adapted CFD, matching actual adapted restart/mesh/reference ordering,
and a real isolated adapted disk restart with distinct input/output filenames.
Original-grid continuation after refusal is not adapted proof. Resume-cycle0
filename overwrite and rejected-event consumption remain open; isolation does not
fix them. Then dedicated BL/constraint/MPI qualification and RANS500k.
