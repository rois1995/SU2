# Fast path, stage 2, step 0 (item FP2-0): measure the building blocks of the surface core on real data

Author: reviewer, 2026-11-19; version 3 of 2026-11-21 (a lean version: the rows that the first surface code does not
need are removed; checked against the source). For the main agent. The user sends it; it is a user order.
Base: the commit that closes item FX1 (`f2d4dfda7d` or later).
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Evidence folder: `R/night_20261115/FP2_0/`.

This item is a MEASUREMENT, as FP0 was. It adds test code only (R3). It changes no production file (R2), no gate and no
reference. Its numbers decide the design of stage 2 (the surface changes in the new local code). No number of this item
is a pass / fail gate, except section 3.
The one question of this item: when plain `double` code splits the surface of event 1 (by length, then by the geometry
distance), does the strict code accept the result (each face on its entity, each face inside tol, the bulk build of the
surface state, the two-sided proof), and how many seconds does it take?

Short names. "tol" = the geometry tolerance of the run (`ADAP_HAUSD`, 1e-4). "Strict" = the production function.
"Float" = plain `double` code in the test file. Files: BP = `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`,
BPh = its header, RM = `Common/src/adaptation/CNativeRemesher3D.cpp`, AR / ARh = `CNativeAffineReference.cpp` / `.hpp`, GB = `CNativeGeometryBackend3D.cpp`,
CA = `CNativeCurvedAtlas3D.cpp`, CR = `Common/include/adaptation/CNativeCurvedReference.hpp`,
FP0T = `UnitTests/SU2_CFD/adaptation/CNativeFastPathStage0_tests.cpp`,
FP0P = `UnitTests/SU2_CFD/adaptation/fixtures/native_fast_path_proofs.inc`.

## 1. Data and loading

G5 = `R/night_20261115/N4/section6_3_after_N11/round_8_5t_P/G5/full587` (the data of FP0).

| Name | Files | Content |
|---|---|---|
| OUT1 | `G5/adapted_mesh_adap_00001.su2.native_ref` (no volume, no tensors) | the accepted surface after event 1 (67,170 faces, 33,587 boundary points) |
| IN1 | `G5/m6.su2.native_ref`; tensors: `R/night_20261115/FP0/cases/B/sensor.bin` | the input surface of event 1 (48,954 faces, 24,479 points) with the target of event 1 |

- The functions `LoadMesh`, `ReadFrozenSensor` and `FP0Driver` of FP0T are in an anonymous namespace: copy what you
  need into the new test file; do not change FP0T.
- OUT1: `LoadReference(G5/adapted_mesh_adap_00001.su2.native_ref, ...)` as FP0T:123. No driver, no import, no volume.
  This file has version 9: the policy and the entity geometry are stored (`EntityGeometry()` must be true). The surface
  state is `reference.original->Parameters()` (a `BoundaryParameterState`); the model is its `Geometry()`.
- IN1: `LoadReference(G5/m6.su2.native_ref, ...)` in the same way. This file has version 7: no mobility policy and no
  entity geometry. Production makes them in `PrepareMobility` (RM:67-104, anonymous namespace: a test cannot reach
  it). Do the same steps in the test: roles `{0: 2, 1: 0, 2: 2}` (far field, symmetry, wing; the marker order of the
  sidecar header), angle 60, the baseline and the budget of RM:82-90, `ceiling = size_t(12288) << 20` (the value of
  `ADAP_NATIVE_MEMORY_LIMIT` of `FP0/cases/B/run.cfg`; it is `Fp0GeometryByteLimit`, FP0P:2), then as RM:91-103:
  `auto m = state->WithMobilityPolicy(60, roles, budget, ceiling, baseline);`
  `auto p = m->WithEntityGeometry(1e-4, budget, ceiling, baseline + m->ResidentBytes());`
  `reference.original = reference.original->WithParameterState(p);`
  Time this setup. Load IN1 one time for each test case.
- Nodal tensors (IN1 only): read `FP0/cases/B/sensor.bin` (format as FP0T:81-105). Give each boundary record its
  tensor by bit-equal coordinates (the record ids need not be the sensor ids). All 24,479 IN1 points have such a
  record (checked by the reviewer's helper).
- Entities of OUT1 (body / sheet / marker): far field 0/0/0 (2,825 faces), symmetry 0/2060/1 (11,762), wing 0/10154/2
  (51,142) and 0/27868/2 (1,441). "Wing faces" = the last two (52,583). Give each result for each entity.
- New test file, hidden tag that G1 does not select (proposal `[.][NativeFastPathStage2]`). One line
  `FP2_0 <name> {json}` for each measurement, with counts and seconds (`std::chrono::steady_clock`). Nice 19, one rank,
  no other owned job (R1). Write the peak memory of the process in the report.

## 2. The measurements

If a function that a row names cannot be reached from a test: do NOT change production code. Write the name, the place
and what a small public helper would need, and continue (as in FP0). A call that throws or stops at a limit: write the
message and the place, and continue. That is a result, not a failure of the item.

### B1. Bulk build of the surface state (OUT1)
Read from the loaded state with the public accessors `Authority()`, `Records()` (use `Entry.point`), `Surface()` (the
values), `Coverage()` (the traces), `Mobility()`, `GeometryTolerance()`, `Revision()`. Call the validating constructor
(BPh:88-91) as `Rebind` does (BP:718-736): `live` and `bound` as BP:722-723, the budget
`BoundaryAtlasBudget(BoundaryAtlasBytes(Coverage(), Mobility()), ceiling, bound)` (the function `AtlasActive` of BP:729
is anonymous; it is true for an entity state), `ceiling = size_t(12288) << 20`, and the LOADED revision (the revision
is part of the encoded state). Time it. Time `BuildMobilityPolicy` alone on the same geometry (`angle` and `roles`
from `Mobility()`; a fresh budget that is made in the same way).
Report: seconds of the constructor; seconds of the policy build; the difference; the modeled peak of the reservation and
the change of the process RSS around the call; `Snapshot()` bytes equal to the loaded state (yes / no);
`Coverage().traces.size()`.

### B2. Cost of the exact local checks (OUT1)
For each boundary point: `Geometry().Admissible(binding, point)` and `Geometry().Realize(binding)`. For each surface
face: `ContainsEntityFace(face.facet, the three records)` (BPh:97; input as BP:259-260).
Report: seconds and microseconds for one call of each; failures (expected 0); the count of points whose `Realize` is not
bit-equal to the stored coordinates (expected 0) and the largest difference.

### G1. Float closest point by a walk (OUT1, wing faces)
Build a neighbour table of the reference facets (3 neighbours for a facet, from `Geometry().Reference().Facets()`; an
edge is protected when its sorted vertex pair is in `Mobility().features`). The float search:
- in one facet: `detail::MinimizeTriangle` on the net normalized by `NormalizeTriangleProjection`, as GB:147-155 does
  (CR:357-482); count the calls that do not converge. Parameters: `SurfaceCoordinates` returns the weights (w, u, v) as
  a point (x, y, z); `MinimizeTriangle` takes u = `.y` and v = `.z` (GB:152: the patch at (1-a-b, a, b));
- start facet (the wing is ONE sheet: the upper and the lower side have the same entity, and a point on the trailing
  edge has facets of both sides; a start on the wrong side gives the distance to the trailing-edge line): if the anchor
  facet `face.facet` supports all three bindings of the face (`Geometry().Supports`, ARh:297): start on it, at the
  `SurfaceCoordinates` of the three corners interpolated to the sample (as FP0P:354-368). Else: among the facets of
  the binding of the face point nearest to the sample (`ForFacets`), those with `Entity(side) == Entity(face.facet)`
  AND `Geometry().OrientedFace(side, binding, the face corners)` (ARh:177; the test that `ContainsEntityFace` uses,
  BP:310-311); of them the lowest id; start parameters `SurfaceCoordinates(side, binding)`. If none: count the sample
  and use the lowest id with the equal entity;
- a minimum on a facet edge that is not protected: continue in the neighbour across that edge, from the same edge point;
  a minimum at a facet vertex: try each facet of the fan around the vertex that is reached without crossing a protected
  edge; never enter a facet again that this sample has visited; stop when no neighbour lowers the distance, or after
  64 facet changes (count these cases);
- distance = |sample - patch point| in double, with the patch point `TrianglePatch(the ORIGINAL net, 1-u-v, u, v)` (not
  the normalized net: `NormalizeTriangleProjection` changes the scale).
Write the walk as one function that row P uses too.
Samples: for each wing face 4 (the centre and the three edge midpoints of the flat face).
Report: seconds; microseconds for one sample; steps and facet changes for one sample (mean, largest); for each entity
the largest and the mean distance and the count of faces above 0.5 tol, 0.8 tol and tol.

### G2. The same against `Project` (10,000 samples of G1, fixed stride)
`NativeMappedGeometryBackend3D(state.Authority(), state.Mobility())` (the state must live longer than the backend), a
budget with a large work limit (`Fp0AtlasBudget`, FP0P:27-46). Scope as `ProjectEntitySurface` (BP:1435-1451) with the
hint = the start facet of G1 and `previous = FacetPoint(hint, .25, .25)`.
Pass 1: `absolute_error = tol / 8` (the production value; it gives the time). Pass 2: `absolute_error = 1e-12` (it forces
the neighbour search; it gives the accuracy).
Report for each pass: microseconds for one call; failures (count, first message, and how many are at the trailing
edge or at the tip; the failures of pass 2 are not comparable with those of pass 1, GB:181); the largest difference
of the distance to G1; the count of samples whose two foot points are more than 1e-9 apart.

### T. Two strict values for one face (helpers for row P; no run of their own)
- production: `EntityFaceError(face.facet, bindings, points, &budget, need)` (BPh:101), with
  `SurfaceQueryNeed::Classify()` (as the census does: above tol or not) or `SurfaceQueryNeed::Magnitude()`. Note: for a
  face that passes, the value is only "a bound at or below tol";
- tight: `Geometry().BoundPhysicalSurface(support, {the face points}, tau, budget, SurfaceBoundPurpose::ErrorMagnitude)`
  (ARh:213-215) with `tau = 1e-5`; `support` = the facets by the rule of BP:401-408, sorted and unique, for EACH face
  (also for a face that production handles on its direct path, BP:385-397). It is an upper bound with a certified
  gap of at most `tau`. Its cost is not known: time the first 20 faces, and give each list of faces at most 5 minutes
  (report how many faces of the list were done).

### P. Prototype: a float split pass on the surface alone (IN1)
Data: the IN1 surface as arrays of the test (points: id, coordinates, binding, tensor; faces: three ids, anchor facet;
a map edge -> two faces). The volume is not used. The nodal tensors as loaded. One code with two steps:
- step L (length): split each surface edge whose length is above 1.8 (the closed formula of FP0 A2) and that is not a
  feature edge, longest first, one change for a pair of faces in a pass, until no such edge is left or 20 passes;
- step D (distance; this is what the engine does today with its "geometry split", but in float): after step L, for
  each face whose 4-sample value (G1) is above 0.8 tol: mark its longest edge that is not a feature edge. Split the
  marked edges, longest first (length in space), one change for a pair of faces in a pass. Repeat until no face is
  above 0.8 tol or 20 passes. A face whose three edges are feature edges, or whose split is refused: count it, leave it.
Feature edge (all IN1 points have vertex bindings): the sorted pair of the two original vertices is in
`Mobility().features`. An edge with a new point at one end is never a feature edge (each new point has
`MobilityDimension == 2`, see the refusals below).
New point: query = the flat midpoint of the edge. Start of the walk: a facet that supports the bindings of both ends
and has the entity of the edge's faces, at the mean of the `SurfaceCoordinates` of the two ends; if no such facet
exists: the start rule of G1 for the first face of the edge. Make a binding from (facet, u, v) with the snapping of
GB:158-170; coordinates = `Realize(binding)`; id = above the largest IN1 id; tensor = the mean of the two ends. Refuse
and count: a binding with `MobilityDimension != 2`; a binding or coordinates equal to those of an existing point (the
constructor refuses both, BP:242); a walk that does not converge. Each new face keeps the anchor of its parent.
One result: PD = the surface after step L and step D. (The reviewer's helper counted 1,675 IN1 edges above 1.8, none on
the wing: step L is small, and the wing changes come from step D.) An environment switch that stops after the passes
(without the strict checks below) is useful for your development runs.
Report: for step L and for step D: passes, splits, refusals by reason, seconds (and the microseconds for one split);
faces and points at the end for each entity, beside IN1 and beside OUT1; the faces that step D left above 0.8 tol.
Then for IN1 (before the pass), for the NEW faces of PD and for ALL faces of PD: the count that fail
`ContainsEntityFace`; the count with the 4-sample value above 0.5 tol, 0.8 tol and tol; the count that the production
census would mark: `EntityFaceError(..., SurfaceQueryNeed::Classify())` above tol. For this: run `Classify` on all IN1
faces; an unchanged face of PD takes its IN1 value; for the new faces of PD time the first 2,000 and, if all would
need more than 20 minutes, use a fixed stride and write it. Keep the per-face values (row P4 uses them).
The tight bound (row T) of the 200 faces of IN1 and of the 200 faces of PD with the largest 4-sample value, beside
their 4-sample value.

### P3. Bulk build of the result PD
Give the surface PD (points with their ids and bindings, faces with anchors, the traces of the IN1 entity state: they
stay valid because no feature edge is split) to the validating constructor as in B1.
Report: accepted, or the first message; the seconds. The messages of the constructor name no face and no point: after
a refusal, find in test code the first face or point that fails the same test (`ContainsEntityFace`, equal
coordinates, equal bindings, a point without a face) and report it with its values of row P.

### P4. The two-sided strict proof on the result PD
Only if P3 accepts PD. On the state that P3 built (no volume is necessary), with the budget of FP0 A11
(`Fp0AtlasBudget(726442, state.ResidentBytes() + Authority()->ResidentBytes(), start, 5400)`):
- mesh to model: `OwnedFaceReverseBounds(all faces of Surface(), budget)` (the first call of A11, FP0P:245). A value
  above tol is the first face that it found above tol, not the largest value;
- model to mesh: `GeometryCertificates(budget, false)` (the direction model to mesh for each entity, BP:525). Do NOT use
  the form `GeometryCertificates(budget, true, &reverse)`: for an entity above tol it runs the full two-sided union,
  and its cost is not known.
Report for each entity: the two bounds, accepted or not, the seconds. The faces above tol come from the per-face
`Classify` values of row P: their count for each entity and, for the first 20, the 4-sample value, the value of
`EntityFaceError(..., Magnitude())` and the coordinates.

## 3. Gates of this item

1. `git diff` shows no change outside `UnitTests/`, your evidence folder and the documentation files that section 4
   item 3 of `AGENTS.md` names (the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`,
   `Handbook/ADAPTATION_TIMINGS.md`). Build without a new warning.
2. G1: the tables are identical to the reference (the new test cases have a hidden tag).
3. No G2, G3, GQ or G5 run for this item.
4. Commit and push the test code and the report ("FP2-0").

## 4. Report

`R/night_20261115/FP2_0/reports/FP2_0_<build>.md`: one table with all rows of section 2, each "could not be reached /
stopped" case with its message, the peak memory, each correction of your test code (R3), each question (R13). Update the
progress file. Commit, push, verify that no owned job runs, stop and wait for the user.

## 5. Not in this item

Any production change. The design of the surface core. Gates on the measured values. The wall layers. A change of the
target in `ComputeMetric`, and a size from the curvature of the wall (rows C1, P2, P2b of version 2: the user parked
this topic). The rows B3 (feature traces in bulk), G3 as a full run, G4 (model to mesh by samples) and C2 (contact) of
version 2: they come later, if the numbers of this item ask for them.
