# Codex handoff — current state

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Full Goal3 **ACTIVE/incomplete**; Goal2 base c43a8a3907ea.
Leave AdapNoExt, foreign ClusterResults/dependencies/jobs untouched.
[Full requirements](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).

## Current work — mapped boundary parameters, NOT production-enabled

New parameter geometry is implemented: boundary-only support/version/scalars,
exact unrounded coordinate bounds/orientation, minimum incident mapping error,
complete finite-facet coverage/feature chains and bounded atomic audit refusal.
Node/Cell/OwnedCell layouts, existing config and remeshing activation stay unchanged.
All original facet edges are protected in this first representation; it may still
obstruct construction. No general discrete coarsening or curved/intersection claim.
[Current implementation and full next reach](Handbook/GOAL3_MAPPED_PARAMETERS.md).

Focused isolated geometry controls pass8cases/2861assertions; constructor admission/
valid-interior correction passes1case/26assertions. Independent rational oracle
checks120coordinate bounds/120orientation signs (60exact zeros) across1e-300..1e300.
Existing affected cancellation/subnormal/segment/saved-M6-midpoint controls pass
3cases/287assertions. Only3changed units compiled, qualified support archives reused;
no full suite, CFD rerun or MPI campaign. All handles terminal; no owned heavy job.
[Sealed evidence](integration_evidence/native_3d_bl_v1/mapped_faceted_geometry_v1/validation_summary.json).

The FULL integrated executable is still b232f4acbf source scope, SHA256
`25d4e87bfee8b5ad11336a459e943bf7e1c0ffad2969028554ab98743f377d21`.
Current source is NEWER: new shared affine-point arithmetic is scoped-tested but
not full-integrated/MPI/lifecycle qualified. Separate geometry executables and source
capsules remain in mapped_faceted_geometry_v1/core_v1_n1/core_v2_n1 and oracle_v1.
Explicitly preserve this qualification distinction; no new mapped adapted mesh.

Prior b232 checkpoint passed source9/affected19cases per rank at MPI1/2/4,
all12exact source-domain audits with bitwise field identity, and the complete serial
Cartesian RANS transfer/SU2+CGNS/restart audit. Reuse its exact old-source scope;
[details](Handbook/GOAL3_JOINT_AFFINE_SOURCE.md). Source grids are NOT CFD grids.

## Next complete capability

Connect declared mapped body/finite-feature bindings, simultaneous junctions and
actual original-source coverage to operators, transfer/persistence and real restart.
Use existing carriers/topology/sidecars; no new physics/testcase dispatch. Current
joint source shell covers only its admitted numerical affine-mapping band. Affine
BL walls, mixed analytic joints, general physical envelopes,2D runtime and complete
oblique/exterior/multiple-body lifecycles remain OPEN.
[Mapped feature contract and exact obstruction](Handbook/GOAL3_MAPPED_FEATURES.md).
[Source-policy comparison and rejected wide collars](Handbook/GOAL3_SOURCE_ENVELOPE.md).

The full [general symmetry scope](Handbook/GOAL3_GENERAL_SYMMETRY.md) stays active:
arbitrary2D lines/3D planes, finite disconnected/nonconvex/multiple-hole domains,
multiple intersecting bodies, stable simultaneous junctions, shared Euler/RANS
geometry, N=M/M<N, state transfer, SU2/CGNS/reference persistence/actual restart and
an exterior workflow. Do not close it on a Cartesian annulus or source primitives.

Saved oblique annulus exact body/farfield edge1--9 has **no interior binary64 point**
and original-sensor length2.249645>1.8. Valid plane adoption/direct single-wall seeding
cannot fix that literal-body obstruction. Do not retry unchanged or weaken gates.
Saved M6 symmetry edge211--3481 likewise has a strict representability obstruction.
The wide1e-4 source-collar experiment folds on two actual M6 faces despite12/12
probe coverage. The actual mapping envelope, not maximum HAUSD, needs admission.

Preserve general common-background wall construction: the broad sequential-point
prototype missed an oblique two-wall counterexample. Current restored shared ctor
and one-wall/one-tet direct insertion pass the connected affected controls. Historical
23-case prototype evidence does not qualify a general replacement or adapted mesh.

## Qualified accepted grids and reuse

Historical complete Cartesian N1M1/N2M2/N4M2/N4M4 lifecycles at6f0bdfabceeb pass
independent finite-hole/height/full-metric/flow+SA+BDF/real SU2+CGNS restart audits.
b232 serial compatibility independently passes as recorded above; current-source
complete MPI lifecycles have not been rerun.
[History and inspectable grids](Handbook/GOAL3_ANNULAR_COORDINATION.md).
`integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/annulus_v10_n1/symmetry_annulus_bl_budget_20_m1_written_SU2.su2`

Accepted spherical repeated RANS N4M2:5440->3768tets/783nodes,4511work,88.499native
seconds/344edits, conservation1.87e-14, original sensor/SU2+CGNS/restart PASS.
Reuse accepted cold8->5440; do not rebuild it. These are contended correctness
receipts, not affordability or converged viscous qualification.
[Accepted repeat](Handbook/GOAL3_REJECTION_DEPENDENCIES.md).
Grid `integration_evidence/native_3d_bl_v1/sphere_rans_lifecycle_v1/n4_m2_dependencies_v1/update/event_written_SU2.su2`.

## Remaining goal and operating rules

- Opt-in budget contract stays: firstheight/protected/finer sensor/geometry,
  original sensor-only P1, actual-query BL and fade. M6 budget300k selectslambda.875,
 295068+/-5769 heuristic/integration-limited;100k sampled-profile conflict is not
 exact infeasibility. [Policy](Handbook/GOAL3_BUDGET_AWARE_BL.md).
- Global uncertainty calibration/allocation, interacting layers/2D extension,
 composed gradation18violations/max4.599, viscous accuracy and complete accepted-event
 performance remain open. [Outcome ledger](Handbook/SUGGESTION_OUTCOME_LEDGER.md).
- [AGENTS.md](AGENTS.md): substantial connected capabilities/difficult case early;
 focused risk checks then affected milestone checks, no full suite per tiny edit,
 cap/fallback ladder or unnecessary CFD run. Reuse evidence only with valid binary,
 source/config/input/helper pins; preserve failures and independently completed stages.
- Announce full new case folders, update these linked handbooks and push checkpoints.
 One heavy job, nice19/build-j1/threads1/MPI<=4; check host RAM/load and actual handles.
 No cluster requests/submissions. Compiler scratch stays on4TB, not nearly full/tmp.
- Optional Claude Opus5.5 medium/high is authorized. Abstract high review completed
 with tools off/no code/data; reasoning input only, not correctness evidence.
- [Cleanup](Handbook/DISK_SPACE_CLEANUP_2026_10_18.md):27.19GiB recovered across64
 verified archives; about46GiB4TB free, root/tmp~490MiB foreign. All scientific
 evidence/current builds/foreign work preserved. Archived old binaries need restore.
- [Future SST/transition/nonideal transfer questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
 Three-element meshes at `/media/rausa/4TB/Validation/ThreeElementAirfoil/30P30N/Meshes`;
 located, no new adaptation in this checkpoint.

[Prior detailed state](Handbook/HANDOFF_BEFORE_MAPPED_PARAMETERS_2026_10_19.md).
