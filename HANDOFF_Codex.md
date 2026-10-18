# Codex handoff — current state

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Full Goal3 **ACTIVE/incomplete**; Goal2 base c43a8a3907ea.
Geometry/source checkpoint is published on `codex/native-3d-bl`; see the
current Git HEAD. Cleanup receipt:14d1bbc9de. Cartesian current-binary lifecycle
qualification is workflow_v6_n4; arbitrary runtime scope remains OPEN.
[Full requirements](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
Leave AdapNoExt, foreign ClusterResults, dependencies and foreign jobs untouched.

## User priority and current job

Affine source/backend batch now passes serial12cases/3974assertions, MPI4 and
independent75-stencil/source-ID/codec/equality audit. Current Cartesian N4/M2 full
lifecycle independently PASS. All owned jobs TERMINAL; no new CFD advance or M6.
Read [backend scope and open full-goal requirements](Handbook/GOAL3_AFFINE_RUNTIME.md).
No accepted oblique lifecycle; simultaneous junction collars and affine-wall BL
composition explicitly refuse. Preserve full geometry/2D/exterior scope and annular
coordinated-construction requirement. Do not declare general geometry complete.

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
affine-wall BL,2D runtime/exterior and annular coordinated construction remain OPEN.
Disk cleanup recovered23.40GiB across two verified archival batches; [receipt and restore](Handbook/DISK_SPACE_CLEANUP_2026_10_18.md).
Current build and all3D evidence preserved; archived old executables need decompression.
Root/tmp nearly full from foreign work: leave it untouched.

Annulus_v4_n1 reproduces work8192 rejection, saves154privatecells/55nodes plus
original field/reference and exact last edge1--5 trial. Input flow/SA retained.
View `integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/annulus_v4_n1/symmetry_annulus_bl_budget_20_m1_rejected.su2`.
Independent same-field manual witness204tets/84nodes PASSES: q.305/J.0959/L1.740,
height0, finite hole/positive volume/embedding/links, complexity20.000195. It is NOT
native adaptation or transfer. This controlled target is feasible, native cold
construction is blocked under current work ceiling. Reconsider coordinated point
distribution/connectivity, never raise caps or retry M6 unchanged.
[Complete scope/evidence](Handbook/GOAL3_PLANAR_SYMMETRY.md).

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

[Prior detailed handoff](Handbook/HANDOFF_BEFORE_PLANAR_SYMMETRY_2026_10_18.md).
