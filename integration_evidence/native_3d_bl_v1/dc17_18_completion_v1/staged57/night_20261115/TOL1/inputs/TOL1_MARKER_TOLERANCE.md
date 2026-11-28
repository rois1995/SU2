# TOL1: a geometry tolerance for each boundary in the native 3D remesher; one capability run

Author: reviewer, 2026-11-28 (reviewed by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/TOL1/inputs/`. Start only after SC1 is closed (it is:
`a94f2f0980`). Short names R, N, "mode S", "standing gates" as in the order FP3-A (your copy in `N/FP3_A/inputs/`).
If a `reviewer-*` unit is active, do not touch it and start no build, test binary or solver while it is active (R1).

User order (2026-11-28), on the flat far-field cells of CL2: "Yes, I think a looser tolerance on the farfield would be
better." This is the user's text for a change of the R1 geometry gate: in list mode (section 2) each boundary marker
has its own geometry tolerance. The standing gates and references keep the one scalar 1e-4 (section 3). No other gate,
budget or limit changes.

## 1. Why

| Fact | State |
|---|---|
| CL2 (norm 2, hgrad 1.6, `ADAP_HAUSD= 1E-4`) events 2-4 were rejected on the same cell: a flat tetrahedron whose four vertices are input FARFIELD points (r = 100), two of its faces farfield faces, q 0.19996 (gate 0.2), isotropic q 0.095 | measured (`reviewer_handoff/scripts/s10/study_quality_step.md` section 1) |
| The engine refused every move and topology trial on it for geometry or progress; the farfield is held by 1e-4 on a flat faceted reference (2,060 planar nets), not by an explicit freeze rule; all 1,063 input farfield points survive in CL2 | measured / read in the source (`reviewer_handoff/scripts/s11/study_native_tolerance.md` section 4) |
| The R-SG failure of 2026-11-23 had the same far-field cells | measured |
| The native 3D path reads ONE scalar from `GetAdap_Hausd()` through two copies: A `control.surface_tolerance` (`CNativeRemesher3D.cpp:236`, engine) and B `BoundaryParameterState::geometry_tolerance` (`CNativeRemesher3D.cpp:96`; sidecar v9; final proof, SC1 certificates, local core). The checks are per face or per entity, and the face knows its reference facet and so its marker | read in the source (s11 section 1) |
| Farfield and symmetry share 64 input vertices (the circle y = 0, r = 100, edges 9.814 m; 67 in CL2 mesh 1); wing and farfield share none; wing and symmetry share 210 | measured (s11 section 3) |
| With FARFIELD 1.0 and SYMMETRY 1e-4, a collapse of a circle vertex cuts the polygon corner by 0.48 m: the symmetry faces see 0, the farfield faces see 0.48 (passes at 1.0), and the final proof fails on the symmetry model-to-mesh direction | inferred (s11 section 3) |
| `GetAdap_Hausd()` returns 0.01 in list mode (DEF1, MMG only); list + `NATIVE_CAVITY` is an error today (`CConfig.cpp:6530-6533`) | read in the source |
| On CL2 mesh 1 the farfield edges are 8.45 m (median); the metric asks 16.6 / 18.5 m tangentially; 1.0 allows about 24.5 m (meets 78 % of the directions) | measured / inferred (s11 section 6) |
| Whether a loose farfield alone lets the engine repair the CL2 cap | NOT KNOWN; the capability run (section 4) measures it |
| Flat cells with four wing points (R-SG mesh: 12 below the gate) | measured (s10 section 8); NOT in this order |

## 2. The production change (R2: allowed by this text, nothing else)

The model. In list mode each boundary marker m has a value T(m). The strict value s is the smallest listed value.
1. A face or an entity is checked with the value of ITS marker (gate readers, 2.4).
2. Where two markers with DIFFERENT values meet, the seam is frozen: its mesh vertices and its mesh edges stay the
   input ones (2.3). So no edit can cut the seam, and the final proof of each marker sees only its own faces.
3. Every other reader uses s (strict readers, 2.5).
In scalar mode (one number) the table is empty, every reader gets the same double, takes the same branch and charges
the same bytes as today. With a list of EQUAL values the result is the scalar result (2.3 adds no corner).

### 2.1 Configuration

1. Keep the DEF1 parsing and value checks of the list (empty field, odd count, junk text, value <= 0 or not finite,
   duplicate, inheritance both ways). With `NATIVE_CAVITY` each listed value v must also satisfy `v <= 1e100` (so a support
   box `radius + v` stays finite, `CNativeBoundaryParameters3D.cpp:533`, `CNativeAffineReference.cpp:2574`):
   "ADAP_HAUSD: <name> value is above 1e100." In the native 3D setup, when the reference is known, s must also satisfy
   `s >= max(1e-12 * X, 16 * std::nextafter(std::numeric_limits<double>::min(), +inf))`, X = the largest absolute
   coordinate of the reference (so s/16 stays far above the rounding of the
   certified chord bounds, `CNativeAffineReference.cpp:834`, `:2950`, and above the floor of `WithEntityGeometry`,
   `CNativeBoundaryParameters3D.cpp:479`): "ADAP_HAUSD: the smallest value is below the native floor <value>."
   MMG parsing and the scalar path are unchanged. Remove the DEF1 error "ADAP_HAUSD per marker is available only with
   ADAP_REMESHER= MMG" (`CConfig.cpp:6530-6533`) and update the DEF1 fatal fixture that expects it
   (`UnitTests/Common/adaptation/DEF1Options_tests.cpp:261`); keep the MMG cases.
2. Names, in two places (`SU2_MPI::Error` each):
   - in `CConfig` AFTER `SetMarkers` (the constructors call it after `SetPostprocessing`, `CConfig.cpp:151-155`),
     with `NATIVE_CAVITY` and a list: every listed name must be a marker of the configuration file
     (`GetnMarker_CfgFile()`, `GetMarker_CfgFile_TagBound(i)`): "ADAP_HAUSD: <name> is not a boundary marker";
   - in the native 3D setup, before the table is built: every marker of the native reference inventory
     (`state.marker_names`, the physical markers with faces, `CNativeImport3D.cpp:1062`) must be listed: "ADAP_HAUSD:
     with ADAP_REMESHER= NATIVE_CAVITY every boundary marker needs a value; missing: <name>".
   A listed, declared marker without faces is allowed; its value still counts in s. So s = the smallest listed value
   (one reading).
3. `GetAdap_Hausd()` in list mode: with `NATIVE_CAVITY` it returns s; with MMG it keeps 0.01 (DEF1 unchanged). So
   every unconverted reader (the transfers, the BL readers, the prints) gets the strict value.
4. 2D native with a list: error in `CNativeRemesher::CheckSupport` (`CNativeRemesher.cpp:34`): "ADAP_HAUSD per
   marker is available for the 3D native remesher only."
5. 3D native with a list needs a mapped reference with entity geometry (the M6 case). In list mode, before the first
   adaptation, an unmapped reference, a reference without curves (`PrepareMobility` returns early,
   `CNativeRemesher3D.cpp:72-74`) or a promotion of a plain faceted reference (`NativeRemeshRun.inc:304-326`) is an
   error: "ADAP_HAUSD per marker needs a mapped curved native reference." R4: name the exact site.
   A list together with a native boundary layer (a nonempty `ADAP_BL_MARKER` list, `CConfig.hpp:1379-1384`, which
   enables the first row of `native_bl3d`) is an error in `CConfig`: "ADAP_HAUSD per marker is not available with the
   native boundary layer yet."
6. Option text (`CConfig.cpp`, `config_template.cfg`): for the native 3D remesher the list gives the absolute geometry
   tolerance of each boundary (a check); every boundary must be listed; where two boundaries with different values
   meet, their common seam keeps its input points and edges.

### 2.2 The table and the canonical identity

Canonical identity: the COMPLETE configured list (every listed name with the bits of its value), sorted by name. It is
computed from `CConfig` alone, so every reader that has only the configuration (the ramp signature, the checkpoint
stage inspection) can compute it. Classes: number the distinct listed values in increasing order (class 0 = s).

The table: one small class built once per event in list mode, owned OUTSIDE every charged scalar layout (for example
by the native remesher beside `ReferenceState`; `BoundaryParameterState`, `MobilityPolicy` and the retained metric
family keep their `sizeof` and charged bytes; the table charges its own allocation only in list mode). It projects
the canonical list onto the reference marker order and verifies that every reference marker has a value (2.1 item 2).
Questions: `Strict()` = s; `Marker(m)` = T(m) for the reference marker index m (`Reference().Get(facet).marker`);
`Class(m)`. No `Session` / `CellState` / row field, no timer operation. In scalar mode the table does not exist and
the readers use their present inputs.

Rank agreement: at the parent, before the local core and before worker dispatch (`NativeRemeshRun.inc:771-810`;
R4: name the site), ONE allreduce (MIN and MAX) of a 64-bit word: 0 in scalar mode, else a nonzero hash of the
canonical identity. It runs in BOTH modes (so a scalar rank and a list rank cannot hang), and all ranks must give the
same word, else error "ADAP_HAUSD differs between ranks." This adds one collective call in scalar mode; that is
allowed because the calls and seconds of `communication` rows and the collective counters are report rows
(`AGENTS.md` section 2a); it adds no other row, no timer operation and no modeled bytes. The existing equality check
of A (`EngineSetup.cpp:21-24`) stays on s.

### 2.3 The frozen seam (list mode with different classes only)

Roles carry the class. The policy input `roles` (marker -> 0 symmetry / 2 other, `CNativeRemesher3D.cpp:75-77`) gets
the class in its higher bits: role = kind + 4 x class, with two helpers `RoleKind(role)` (= role % 4) and
`RoleClass(role)` (= role / 4) and one comment. Every reader of a role kind uses `RoleKind` (R4: list all readers of
`roles`, about 32 sites, and convert each that tests the kind, for example the check `role != 0 && role != 2` at
`CNativeAffineReference.cpp:2092`). With all classes 0 (scalar mode, equal lists) every role value, the policy, its
encoding, hash, memo key (`CNativeBoundaryParameters3D.cpp:256-262`) and the sidecar bytes are today's. Because the
policy is rebuilt and compared from angle and roles everywhere (the state constructor `:365`, `:845`, the local-core
hand-off `CNativeLocalSurfaceGeometry3D.cpp:937`, the worker payloads, the reader renumbering, the sidecar
`CNativeReferenceIO3D.cpp:291`), the classes then travel with no new field. `PrepareMobility` compares the
configured roles with a persisted policy as today (`CNativeRemesher3D.cpp:80-82`), so other classes give the present
error "Persisted curved mobility policy differs from ADAP_ANGLE/marker roles."
1. Seam. A protected feature edge whose two incident facets have markers of different classes is a seam edge; a
   reference vertex on a seam edge is a seam corner: in `BuildMobilityPolicy` (`CNativeAffineReference.cpp:2085-2150`)
   add this condition to the corner test (`:2126-2134`).
2. Seam points are fixed. Every MESH vertex whose binding lies on the seam (dimension 0 at a seam corner, or dimension
   1 on a seam edge, also when the input mesh already subdivides a reference seam edge) is fixed:
   `MobilityDimension` (`:2167-2180`) returns 0 for it, so no engine or core path moves or contracts it, and the
   removal guard (`CNativeBoundaryParameters3D.cpp:948-951`) refuses its removal.
3. Seam edges are kept. A mesh edge whose two endpoints are seam points and that lies on a seam edge is never split,
   flipped or retired, in the engine (the surface split paths, for example `CNativeGeometryBackend3D.cpp:87`,
   `CNativeBoundaryParameters3D.cpp:1684`) and in the local core (`CNativeLocalCore3DChanges.cpp:612`,
   `CNativeLocalCore3DCollapse.cpp:192`, `CNativeLocalSurfaceGeometry3D.cpp:558`). R4: list every path that can split,
   flip, retire or move a seam edge or point and give each the refusal; count the refusals (print-only).
4. Seam check. In the shared `BoundaryParameterState::Prepare` (`CNativeBoundaryParameters3D.cpp:907`), used by every
   publisher (the engine commit, `NativeRemeshRun.inc:1009`, and the BL `Apply`), and before each local-core
   installation of a replacement state (`CNativeLocalSurfaceGeometry3D.cpp:940`), compare the seam with the event
   input, by ids BEFORE any reader renumbering (`CNativeImport3D.cpp:2076`): each seam point's exact coordinates and
   binding; each seam edge's two endpoints; on each side of each seam edge, the marker and entity of the oriented face
   that holds it (not the third vertex of that face). A difference is the error "Native 3D adaptation changed a frozen
   marker seam." The engine publisher (`Directory::Publish`, `CNativeDistributed3D.cpp:1489-1590`) also compares the
   STAGED coordinates of every seam vertex with the frozen coordinates BEFORE its no-delta return (`PrepareParameters`, `:1591`, before the call of `Prepare` at `:1608`), so a
   replacement without parameter edits cannot move a seam point.
5. Keeper identity. `ContractionAllowed` (`CNativeGeometryBackend3D.cpp:96`, `CNativeAffineReference.cpp:2188`) treats
   a keeper of mobility 0 as a reference vertex: require `retained.dimension == 0` before that vertex lookup; a fixed
   dimension-1 seam point uses its own edge identity in any keeper compatibility check (and stays fixed for movement
   and removal).
On M6 with FARFIELD 1.0, SYMMETRY and WING3D 1E-4 the 64 circle vertices (y = 0, r = 100) and its 64 edges are the
seam; the wing root (WING3D / SYMMETRY, one class) is not a seam. The circle keeps its input edges (9.8 m); the metric
asks 16-18 m there, so no split is lost on M6. Limitation (option text): a seam between two different values keeps its
input points and edges.

### 2.4 Gate readers (the value of the marker)

Each site below compares an error with the tolerance or derives a gate bound from it. Use `Marker(m)` of the face's
reference facet, or of the entity:
- engine census: face error > tol, debt `max(0, e - tol)^2`, bins `e/tol - 1` (`EngineSelectionCensus.cpp:521-562`);
- trial `GeometryCounts` (`EngineTrial.cpp:524-559`); guidance excess and fine placement (`EngineTrial.cpp:303-326`,
  `:476`);
- prepared surface check (`EngineControl.cpp:64-73`); "already valid" (`EngineSetup.cpp:695`);
- smoothing face tests (`MeshOptimizer.cpp:1435-1454`, `:2526-2550`, `:2909-2912`);
- `EntityFaceError` (`CNativeBoundaryParameters3D.cpp:498-535`, with its query `:1584-1850`) and the atlas query
  under it: TWO inputs, the admission limit L = `Marker(m)` and the accuracy s. Use L for the outer support expansion
  (support radius + L), both box tests of the witness selection (`CNativeAffineReference.cpp:3024`), the inner witness
  candidate radius and early-pass threshold (`TriangleUnion::Upper`, `CNativeAffineReference.cpp:908`), the
  classification, every pass / fail comparison and the minimum valid cutoff. Use s for the tessellation resolution
  (its initialization, `CNativeAffineReference.cpp:2876`), the magnitude convergence gap and the scope s/8
  (`CNativeBoundaryParameters3D.cpp:1595`). Key a cached answer by both values when
  they differ. Carry the same split through the shared and the fallback final proof (`CNativeGeometryBackend3D.cpp:249`,
  `CNativeEngine3D.cpp:370-399`). When L = s (scalar mode, or a marker at s) the present code path runs unchanged;
- the final proof per entity, both directions (`CNativeBoundaryParameters3D.cpp:563-724`) and `ProveOwnedSurface`
  (`CNativeEngine3D.cpp:370-399`): (L, s) with L = `Marker(entity marker)`. The donor-envelope query of the source
  import (`GeometryCertificates` without a complete-proof request, `CNativeImport3D.cpp:478`, its bounds at
  `CNativeBoundaryParameters3D.cpp:658`) stays (s, s); keep the two results in distinct caches in list mode;
- SC1 certificates: the key's tolerance bits (`CNativeGeometryCertificates3D.hpp:56`) = `Marker(m)` of the face. Every
  writer (local core, committed trial, owned proof) qualifies the face against that same value, and the reader
  (`EngineCensus.cpp:254`, `CNativeBoundaryParameters3D.cpp:1818-1850`) resolves it from the same face. A face has
  one marker, so the face value and the entity value are the same number;
- local core entity face test (`CNativeLocalSurfaceGeometry3D.cpp:777-786`, `:942`);
- the two collapse vetoes: the engine filter `CollapseConstraintViolation` (`EngineSelectionQueues.cpp:445`, its face
  comparison `CNativeEngine3D.cpp:783`): L of the face; the local-core forward-sample screen of a child fan
  (`CNativeLocalCore3DCollapse.cpp:471-478`): 0.7 x L of the face;
- the local-core step-D split triggers (`CNativeLocalCore3DSurface.cpp`): BOTH face error tests, the midpoint test
  (`:798-812`, the comparison at `:808`) and the face trigger (`:860`): 0.7 x L of the face; BOTH feature-chord tests
  of a chord that is not a seam, the chord selector (`:744`) and the chord trigger (`:833`): 0.7 x the smallest L of
  its incident facets.
R4: list every other site that compares an error with the tolerance and decides accept / refuse; give it the value of
this section and write it in the report. A site that predicts work, sets an accuracy, a search radius or a step limit
is a strict reader (2.5).

### 2.5 Strict readers (s; each keeps its present input and branch in scalar mode)

The local core `Limit() = 0.7 tol` for the child thinness floor (`CNativeLocalCore3DCollapse.cpp:244-249`), the
reverse screen `ReverseWithin` (`:266`) and the other resolution floors (not for the forward screen and the triggers
of 2.4), the walk tol/8 (`CNativeLocalSurfaceGeometry3D.cpp:625`), the outward step
`h < tol` (`CNativeLocalCore3DSurface.cpp:381-387`) and the contact margin `3 tol` (`NativeRemeshRun.inc:1087`):
so the body-separation argument and the remote gathers stay as today (the exact pass-end contact check,
`CNativeLocalContact3D.cpp:366`, stays the safeguard). The work predictions and partition weights
(`EngineOwnership.cpp:183-203`, `NativeRemeshRun.inc:551`, `CNativeImport3D.cpp:1662`): pass s explicitly. The split
prediction (`EngineSelectionQueues.cpp:30`, its query `CNativeBoundaryParameters3D.cpp:1645`): (s, s) for the parent
and the child queries and s for the debt it predicts. `SplitPointLiftRequiresSplit` (`EngineTrial.cpp:2044-2050`),
`CollapseOutsideTolerance` (`EngineTrial.cpp:1879`), symmetry adoption (`CNativeImport3D.cpp:1490`), the wall-region
and legacy wall sites (`EngineSetup.cpp:780`, `EngineLegacyWall.cpp:949`, `CNativeBoundary3D.cpp:2276-2296`), the BL
readers, the transfers. In scalar mode the promotion `max(configured, declared)` (`NativeRemeshRun.inc:326`) and the
A / B handling (`CNativeBoundary3D.cpp:3497-3503`) are unchanged.
The BL first-row feedback (`SU2_CFD/include/adaptation/native_bl3d/BLEvent15.inl:251` and its nonplanar demand
check): (s, s) for its `SurfaceError` measurement and its present zero threshold for `GeometryGuidanceExcess(..., 0)`.
The marker-aware guidance of 2.4 is only for the engine callers named there.

### 2.6 Persistence and identity

- Copies A and B and the sidecar v9 header keep ONE value: s. The early return of `PrepareMobility`
  (`CNativeRemesher3D.cpp:82`) compares s, after the roles comparison of 2.3.
- Identity (the canonical identity of 2.2). The in-memory family (`CSolver.cpp:2636-2640`) gets NO new field: within
  one process the configuration and its list are read once and do not change (`ADAP_HAUSD` has no per-level form), so
  the scalar comparison stays sufficient there. The binary target checkpoint and the BL ramp signature do NOT change:
  they exist only for the native boundary layer (`CSolver.cpp:2653`, `CNativeTargetCheckpoint3D.hpp:85`), which a
  list rejects (2.1 item 5).
- Print once per adaptation event (print-only, under `options.timings`, R11): one row `NATIVE3D_TOLERANCE` with s,
  each marker name, index, value and class, the number of seam corners and seam edges, and the count of refused seam
  edits so far; in `NATIVE3D_FINAL_CERT` each entity's value beside its
  two distances.

No other change. Readable code (R9): the table class; the two-input query; no tolerance arithmetic copied at the call
sites.

## 3. Gates

Shared code change. Scalar mode must be exact; a list of equal values must be exact; then one capability run.

1. Scalar mode, the standing set of DEF1 section 3 with the DEF1 legacy pins (`AGENTS.md` section 2a): G1, G2 and
   GQ (exact prefixes as FP3-E), G3, G4, the shared-engine gates, and ONE G5 run S with all its standing checks and
   the isolated restart: the six payloads equal to the S reference in force (`N/FP3_E/accepted_reference.json`).
   Full OFF and full CORE: "not evaluated".
2. Equal list: ONE G5 run S, derived like check 1, where BOTH independently authenticated launch configurations (the
   full run and the isolated restart; DEF1 `run_full.py:60-90`) get one more change: the line `ADAP_HAUSD= 1E-4`
   replaced by `ADAP_HAUSD= (WING3D, 1E-4, SYMMETRY, 1E-4, FARFIELD, 1E-4)`. The receipts record and authenticate the derived
   bytes of both (the restart has `ADAP_LOOP= NO` and prepares no native reference, so it prints no tolerance row);
   the two full-run adaptation events show the `NATIVE3D_TOLERANCE` row. Exact: the six
   payload hashes (mesh, `.native_ref`, restart of both events) equal to the same reference, the seam corners and seam edges 0,
   and the protected-edge / corner counts of the `Native3D curved mobility` row equal to check 1. All other standing
   checks of check 1 apply with their thresholds (time, memory, coverage, join, proof faces). The table's own bytes
   are allowed in this run (memory is a threshold check).
3. Fixtures (test code, R3; normal `test_driver`; each fatal case in its own subprocess, assert the diagnostic):
   - configuration: list + NATIVE_CAVITY accepted after `SetMarkers`; `GetAdap_Hausd()` = s (native) and 0.01 (MMG);
     a missing boundary marker; a listed name that is not a boundary marker; a native list value above `1e100` (rejected) and `1e100`
     itself (accepted); with a unit-scale reference, s just below `1e-12` (rejected at native setup) and s = `1e-12`
     with a loose face that needs the atlas (accepted; the query completes); a reference with X = 1e-298 and s = 2e-310
     (rejected at native setup); a list with a nonempty `ADAP_BL_MARKER` (rejected); the DEF1 value errors and inheritance;
     2D native + list (`CheckSupport`); an unmapped reference with a list; scalar form unchanged;
   - the frozen seam on a small mapped reference with two markers and one shared feature curve that is NOT straight,
     one marker curved (non-planar) so that projected trials have a nonzero face error, and an input mesh that
     already subdivides one reference seam edge: with different values every seam point (corner and the pre-existing
     dimension-1 point) has `MobilityDimension` 0; a collapse that removes one is refused (the cut-corner case); a
     move of the dimension-1 seam point is refused; a contraction onto that point is refused through the keeper check
     with an edge id that equals a vertex id of the facet; a split of a seam edge is refused in the engine AND in the
     local core; a forced change of a seam point is caught by the seam check of the engine commit AND of the
     local-core installation; a forced move of a seam point with an EMPTY parameter delta through `Directory::Publish`
     is caught; a change of the third vertex of a face on the seam is NOT a seam change; with equal values the roles, the
     policy encoding and hash equal the scalar ones; a persisted policy with other classes gives the roles error; a
     policy with classes survives the local-core hand-off and a fresh sidecar decode bit for bit (round trip);
     `RoleKind` / `RoleClass` on each role value;
   - in-range loose collapses: a collapse on the loose marker whose child-fan error lies between s and L passes the
     engine filter (`EngineSelectionQueues.cpp:445`, with a nonempty wall-height policy or an extension component so
     that the filter passes its early return, `CNativeEngine3D.cpp:769`) and the local-core sample screen and reaches
     the classifier; a step-D face whose error (also a centroid-only error) lies between 0.7 s and 0.7 L is not a split trigger in
     either face test;
   - the donor-envelope query gives the same bounds with a list as with the scalar s;
   - a legal projected trial on a face of the loose marker whose certified post-projection face error lies strictly
     between the two values: it reaches `GeometryCounts` and passes; the same kind of trial on the strict marker with
     an error in the same range reaches `GeometryCounts` and is refused; the complete two-sided final proof of the
     accepted result passes with the entity values;
   - `EntityFaceError` with L > s: a face whose support lies between s and L from the model is found, kept by the
     inner witness crop and classified with L; L = s gives the present answer bit for bit;
   - SC1: a certificate qualified for a loose-marker face does not give a hit for a face of the strict marker; an
     entity-level pass cannot seed a census hit at a smaller value;
   - MPI (2 ranks), rejected by the agreement check without a hang: (i) the same s and classes but a different loose
     value on one rank; (ii) one rank scalar `1E-4`, the other a list with s = 1E-4.
4. A failed gate: R5 (one diagnosis, one correction, then revert and FAILED).

## 4. The capability run CL3 (no gate, no reference)

After section 3 passed. Copy the CL2 controller and report (your `N/CL2/`) to `N/CL3/` (test code, R3) and bind it
to the accepted TOL1 build. Configuration: the sealed `N/CL2/run/run.cfg` with ONE change:
```
ADAP_HAUSD= (WING3D, 1E-4, SYMMETRY, 1E-4, FARFIELD, 1.0)
```
Same binary as section 3, 6 ranks, the CL2 launch rules, watchdog 21600 s, the CL2 failure rules and labels with
"CL3". The report has the CL2 tables with the CL2 number beside each CL3 number, and per event also:
- accept / reject; on a rejection the rejected cells as in CL2 (q, J, surface points, markers);
- the `NATIVE3D_TOLERANCE` row; per entity the two final-proof distances and the tolerance (an event that stops
  before the proof vote has no `NATIVE3D_FINAL_CERT`: write "not evaluated"; on a proof rejection keep its message
  and the entity distances that the log has);
- the faces of each marker; the number of input farfield points kept; the circle vertices (points on both FARFIELD
  and SYMMETRY), the seam corners and edges, and the refused seam edits;
- the census of flat cells: cells with all four vertices on one marker and at least two faces on it, per marker,
  with the count below the gate and the minimum q;
- the fate of the CL2 cap (its four input points, s10 section 1): present / removed, and its q;
- the native seconds per phase (construction, quality with the optional cleanup, final proof) and the job peak;
- the shock measure with the unchanged script.
A failed CL3 check is a capability RESULT (it reverts nothing). A proven wiring defect (a wrong value in
`NATIVE3D_TOLERANCE`, a final-proof entity with the wrong tolerance) follows R5 on section 2, with the DEF1 rule for
one repeat run (DEF1 section 5, with "TOL1" and "CL3").

## 5. Report and labels

`N/TOL1/reports/TOL1_<build>.md` and `.json`: the R4 list (all sites of 2.4 and 2.5 with their kind), the touched
files, the gates of section 3, the CL3 report (or its link). Labels:
- "TOL1: native tolerance per marker; standing and equal-list checks passed" after section 3;
- then "CL3: loop run DONE ..." / "CL3: ... FAILED at event <k>, gate <n>" / "CL3: ... OPEN" as in CL2;
- or "TOL1: FAILED at <gate>" (reverted).
After the final acceptance only, add one bullet to `AGENTS.md` section 2a: "Since TOL1 `ADAP_HAUSD` takes a marker
list also for the native 3D remesher (every boundary with faces listed; each face uses its marker's value; where two
markers with different values meet, their seam keeps its input points and edges; all other readers use the smallest
value); the standing gates and references keep the scalar `ADAP_HAUSD= 1E-4`;
the references do not change." Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and the timings
handbook. Then STOP.
Rules R1-R13 hold. Expected time: a build, the gates (about 2-3 h), CL3 (about 1.5-2 h).

## 6. Not in this order

A repair of flat cells (CAP1, the next order: it is needed for the wing). A change of the optional metric cleanup or
of the stop rules. A loose value for SYMMETRY or WING3D in any run. A per-marker value in 2D or with the native boundary layer. A change of the
transfers, of the BL readers, of the contact margin or of the outward step. A second CL3 run (except the R5 repeat).

## 7. Inputs

- Studies (context, not orders): `reviewer_handoff/scripts/s10/study_quality_step.md`,
  `reviewer_handoff/scripts/s11/study_native_tolerance.md`.
