# Codex handoff — current state

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Full Goal3 **ACTIVE/incomplete**; Goal2 base c43a8a3907ea.
Geometry/source checkpoint is published on `codex/native-3d-bl`; see the
current Git HEAD. Cleanup receipt:14d1bbc9de. Historical Cartesian lifecycle
qualification is workflow_v7_n4; current retained-cell build and general scope remain OPEN.
[Full requirements](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
Leave AdapNoExt, foreign ClusterResults, dependencies and foreign jobs untouched.

## User priority and current job

Cartesian annular checkpoint6f0 passes N1M1/N2M2/N4M2/N4M4 complete RANS/SA/
current+BDF/transfer/SU2+CGNS/real restart with independent finite-hole/full metric/
height audits. Those receipts remain historical qualification; the first-row seed
is now changing and needs affected compatibility checks before current-source claims.
[Qualified evidence](integration_evidence/native_3d_bl_v1/native_height_star_v1/README.md).

V2 arbitrary-plane initial adoption passed8serial cases/3981assertions and an
independent exact source/reference/geometry audit in affine_initial_adoption_v1/
controls_v1_n1. Euler/RANS one/two disconnected components under one marker retain
identical original sensor traces; narrow-gap positive-cell boundary collision rejects
atomically. Auditor rounding/orientation mistakes were corrected without CFD reruns;
old auditor/failure and the executed binary are preserved.

The difficult oblique annulus_v1_n1 adopted geometry and normalized its target but
rejected first-row construction after2work items. Exact replay shows its prescribed
normal point INSIDE the old tetrahedron; all four background proposals fail cell/fan
containment. This is operator blockage, not a geometry or budget infeasibility proof.
A positive four-child insertion witness is not full metric/quality acceptance.

Connected follow-up removes the unnecessary background: insert normal-height points
into retained wall tetrahedra, retain already-valid rows, and keep full-field seeding/
repair/final acceptance separate. Geometry-only seeding no longer imports/ranks metric
background proposals. All actual field, height, geometry, transfer and quality gates
stay unchanged. Source is IMPLEMENTED/UNQUALIFIED, building in49032/PID1155618, nice19/
CPU1/j1. Validator17245/PID1157699 waits for terminal build, then controls_v2_n1,
independent initial audit and difficult annulus_v2_n1 sequentially; stops on failure.
Check job_handle_v2.json/build_provenance.json/validation_job_v2.json before acting.
No C++ edits while pinned; no new physical CFD advance, MPI or M6 attempt.
[Current receipt and limitations](integration_evidence/native_3d_bl_v1/affine_initial_adoption_v1/README.md).

1. Spherical repeated RANS lifecycle validation/push: **DONE**, reuse accepted cold.
2. Explicit audited planar symmetry reference, including wall intersections and
   original donor consistency: **IN PROGRESS**.
3. Complete controlled geometry/metric/height/MPI/transfer/restart workflow first;
   then M6 with geometry and budget changes documented separately.
4. Measure accepted complete spherical cycles, including updates/transfer/rebuild.
5. Global uncertainty calibration/allocation, interacting layers,2D and subsequent
   viscous accuracy remain required. Neither accepted M6 nor affordability is proved.

Current new scope is [general symmetry geometry](Handbook/GOAL3_GENERAL_SYMMETRY.md):
both dimensions, arbitrary orientation/position, finite multi-body/hole/disconnected
regions, simultaneous junctions, shared Euler/RANS geometry and complete MPI
transfer/persistence/restart/exterior workflows. Cartesian and one-hole checks do
not complete it. Preserve full goal and all gates.

Qualified affine core (39c3d264f2) retains arbitrary2D/3D charts, finite multi-hole/
nonconvex/disconnected domains and complete chords/faces. Source/backend scope is
now [documented here](Handbook/GOAL3_AFFINE_RUNTIME.md): native-ref v4, original-P1
support and collective source validation are qualified. Private affine projection
routes are implemented, not an accepted oblique proposal/lifecycle qualification.
Keep input-adoption versus mapping-error contracts separate. General junctions,
affine-wall BL and2D runtime/exterior remain OPEN; the Cartesian annular construction
is accepted in the four scoped lifecycles above.
Disk cleanup recovered23.40GiB across two verified archival batches; [receipt and restore](Handbook/DISK_SPACE_CLEANUP_2026_10_18.md).
Current build and all3D evidence preserved; archived old executables need decompression.
Root/tmp nearly full from foreign work: leave it untouched.

Accepted inspectable grid:
`integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/annulus_v10_n1/symmetry_annulus_bl_budget_20_m1_written_SU2.su2`
(CGNS in the same folder). All accepted MPI cases are in annulus_v10_n2/n4 and annulus_all_v10_n4;
old MPI rejected mesh is in annulus_v9_n2 with `_rejected`.
Serial metric-build7.879s/native5.732s/replace.01757s are contended correctness
receipts, not affordability. Composed gradation remains18violations/max4.599;
final uncertainty is heuristic and did not improve monotonically with refinement.
[Construction history](Handbook/GOAL3_ANNULAR_COORDINATION.md).

## Accepted spherical repeat — retain evidence

Actual N4/M2 initialized RANS/SA/current+BDF repeat:5440->3768tets,783nodes,
4511/6144work,88.499native/89.390fixture seconds,344committed edits. Fresh exact
geometry/height/62804changed overlap pairs, conservation1.87e-14, transported sensor0,
SU2+CGNS/reload PASS. Native full composed metric scope retained; no independent
spherical tensor or converged viscous/affordability claim. Serial30/1769, MPI2/4
and independent current strict/opt-in N4M2/new-output restart controls pass.
[Accepted result and provenance](Handbook/GOAL3_REJECTION_DEPENDENCIES.md).

Grid folder:
`integration_evidence/native_3d_bl_v1/sphere_rans_lifecycle_v1/n4_m2_dependencies_v1/update`.
View `event_written_SU2.su2` or `event_written_CGNS.cgns`.
Reuse accepted cold producer n4_m2_v2/construct8->5440; do not repeat it.

## M6 geometry decision — no unchanged retry

Old frozen M6 SYMMETRY edge211--3481 has distinct exact facet planes; only one
interior binary64 point, then primitive children. Composed parent12.796/left11.572
exceeds1.8; sensor-only parent.325. Exact lattice proof, fixed-accuracy metric
measurement. No larger cavity/rank count can resolve this under its old contract.
[Preserved certificate](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1/README.md).

User now explicitly prioritizes a declared planar symmetry authority. M6 input
audit: max4.59885e-12 deviation fromY=0,210wing/64farfield junctions,21367affected
positive-orientation screens; symmetry annulus Euler0/274border edges. A plane
must retain that finite hole. This audit establishes no global embedding, adapted
mesh or transfer of the old off-plane field.
[Current design and audit](integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/README.md).

## Other open work and permanent rules

- Opt-in BL budget contract remains active: firstheight/protected/finer sensor,
  original P1/actual-query composition/tangent/support/fade unchanged. Saved M6
  budget300k selects lambda.875,295068+/-5769 heuristic/integration-limited;100k
  estimated sampled-profile conflict. [Policy](Handbook/GOAL3_BUDGET_AWARE_BL.md).
- Enabled analytic-budget preflight rejects provider routing before any trial
  in.346s, NOT a budget/construction result. Preserve raw receipt/source capsule;
  defer its fix under geometry-first priority. [Failure](integration_evidence/native_3d_bl_v1/budget_curved_interacting_v1/README.md).
- Full goal,2D policy/global error/composed gradation/viscous/performance gates
  remain. [Outcomes](Handbook/SUGGESTION_OUTCOME_LEDGER.md),
  [future physics](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
- Follow [AGENTS.md](AGENTS.md): substantial connected capabilities/difficult case
  early; provenance-aware resumable evidence; focused checks before broad milestone
  checks; no full suite per tiny edit, cap/fallback ladder or unnecessary CFD run.
  Announce full case folders, preserve failures/all gates, push checkpoints.
- One heavy job; nice19/build-j1/libraries1/MPI<=4, monitor actual host RAM/load,
  check live handles. No cluster requests/submissions. Avoid root/tmp compiler use.
- [Current height-star implementation, pinned build and replay](integration_evidence/native_3d_bl_v1/native_height_star_v1/README.md).
  Optional Opus5.5/medium review21708 timed out124 without a final report.

[Prior detailed handoff](Handbook/HANDOFF_BEFORE_PLANAR_SYMMETRY_2026_10_18.md).
