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
no full suite, CFD rerun or MPI campaign at that historical checkpoint. Current build is listed below.
[Sealed evidence](integration_evidence/native_3d_bl_v1/mapped_faceted_geometry_v1/validation_summary.json).

Two mapped prototypes have linked through official dependencies. Latest tested
binary76ffde56417555462279bd12c30b7d75ed7a02c6801d53b1019c895852b16e5a
is preserved before the next rebuild. Realized/private-scope1case/25assertions,
publication1case/38assertions, reader Euler/RANS1case/38assertions and independent
exact normal-dot209samples (171filtered/38exact) PASS. Uniform Euler completes
6->58tets, conservative BDF2 transfer and SU2/CGNS geometry reload. Uniform RANS
also constructs58tets but its state comparison fails because the new fixture
prescribed nonzero no-slip-wall momentum; corrected fixture is rebuilding.
No accepted oblique BL or full mapped real-restart workflow is yet established.

Prior b232 checkpoint passed source9/affected19cases per rank at MPI1/2/4,
all12exact source-domain audits with bitwise field identity, and the complete serial
Cartesian RANS transfer/SU2+CGNS/restart audit. Reuse its exact old-source scope;
[details](Handbook/GOAL3_JOINT_AFFINE_SOURCE.md). Source grids are NOT CFD grids.

## Connected integration — annular lifecycle batch building

Boundary-only accepted/current skin, private bindings, staged collective
publication, engine isolation, mapped source envelopes, N/M return, actual reader
numbering and native_ref v5 are connected. `linked_v1`/`linked_v2` preserve complete
source/build/resource receipts; reader_v1_n1 and runtime_v2_n1 preserve failures.

Current batch: oblique annular BL fixture declares matching mapped authority before
metric/original-donor freeze; rejected-state observer receives the actual private
geometry (all callers updated), preserving rejected cells/reference and trial
forward bindings. Its mapped refusal check verifies accepted metadata unchanged.
Uniform Euler/RANS fixtures now save BDF states/projection receipts; RANS momentum
is affine in z and zero on its configured no-slip wall. Transfer tolerance unchanged.

All4changed interfaces pass syntax_v3. Official13-step build active via
`build_guard_v2.py` (tool8820/Ninja1890502), one nice19 CPU/-j1,4TB scratch,
bounded time and sampled memory. Do not edit pinned C++ or start another heavy job
until the actual handle is terminal. [Exact qualification and limits](integration_evidence/native_3d_bl_v1/mapped_boundary_lifecycle_v1/README.md).

Next: corrected complete uniform lifecycle and matching-private-state rejection,
then actual oblique annular BL before broader affected MPI. Preserve exact failure
if construction stalls; reconsider missing construction/seam capability instead
of another local fallback. Full solution-file restart, exterior/multiple-body/2D
and affordability remain OPEN. Opus5.5/high420s timeout gave no review qualification.

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
- [Cleanup](Handbook/DISK_SPACE_CLEANUP_2026_10_18.md):35.62GiB cumulative recovered,
 including8.43GiB from18 historical2D hard-link groups/55paths. About55GiB4TB free,
 root/tmp~490MiB foreign. All archival handles terminal; prior builds/3D binaries,
 scientific evidence and foreign work preserved. Archived old binaries need restore.
- [Future SST/transition/nonideal transfer questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
 Three-element meshes at `/media/rausa/4TB/Validation/ThreeElementAirfoil/30P30N/Meshes`;
 located, no new adaptation in this checkpoint.

[Prior detailed state](Handbook/HANDOFF_BEFORE_MAPPED_PARAMETERS_2026_10_19.md).
