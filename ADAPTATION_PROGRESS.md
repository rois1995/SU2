# Native adaptation progress

Companion to [ADAPTATION_TODO.md](ADAPTATION_TODO.md), the acceptance ledger.
Updated: 2026-10-29. Detailed failures and provenance remain linked below.

## Current result

No complete curved CFD/M6 adaptation, transfer and restart workflow is accepted.
DC-17/DC-18 remain open. The selected foundation is SCOREC/MeshAdapt classified
cavities and transported nodal size fields inside SU2. Requirements, source-code
comparison, decisions and acceptance: [connected design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md).

Stopped at the user's request on2026-10-29. Their updated HANDOFF_Codex.md is
preserved unchanged. No owned job remains active. Latest build124 passes with1295
pins verified; serial45 cases/9611 assertions pass. MPI6 passes11/12 cases per rank;
only the two curved workflow COMPLETE assertions fail. Full curved/M6 adaptation,
conservative transfer/history and actual restart remain unaccepted; DC-17/DC-18 open.

Saved changes: classified boundary reconstruction and optimizer overlay refresh;
original-surface error replaces custom curvature guidance for geometry decisions;
geometry-aware quality selection remains separate from validity-only construction.
No budget or final gate changed. Latest smaller workflow has256 private cells,
q/J2/2,size22,geometry0 at4096 work, below its arithmetic limit. Fresh-work
continuations do not prove feasibility: changed-source diagnostic times out;
original-metric-source diagnostic refuses bootstrap before any operation.

Current results, replay limitations and exact evidence paths:
[stop checkpoint](Handbook/ADAPTATION_CHECKPOINT_20261029_STOP.md).

Supporting checks: projection20 cases/597 assertions; MPI6 focused8 cases/rank;
current-field122 cases; all10 DC18 geometry groups;28 filter selections. These do
not establish accepted curved/M6 adaptation, transfer/history or solver restart.
DC-17/DC-18 and the full BL/RANS goal remain open. No cap or final gate raised.
Detailed evidence: [projection findings](Handbook/ADAPTATION_PROJECTION_FINDINGS.md),
[selected design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md),
[transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md).


## Checkpoints

| Checkpoint | Change | Validation status |
| --- | --- | --- |
| `1a5745fe3e` — WIP | Preserve concurrent rejection messages; complete M6 transaction replay and fixed-vertex obstruction | MPI2 diagnostic regression passes; both private repairs fail strict shape. Surface placement is test-only, full M6 remains unaccepted. |
| `fdb6ad2683` | Persisted curved import and original-source runtime queries | Small cubic Euler/RANS transfer and SU2/CGNS reload pass at this baseline. |
| `79bfba6f8f` | Reuse collision searches, retaining exact contact checks | Connected checks pass; full M6 validation41 rejects. |
| `e398807a2c` | Reactivate curved demands; admit compatible concurrent proposals | 105 serial cases, MPI2 and all-worker runtime pass; full M6 validation43 rejects. |
| `c02010b08d` — WIP | Connect validity-only construction and bounded point placement | Validation44/45 failures preserved; not an accepted capability. |
| `3deed0ca3c` — WIP | Bounded topology repair and weighted radial integration | Validation46 fixes integration; cubic runtime still fails. |
| `4933586627` | Charge actual optimizer work and refund unused reservations | Validation47 serial passes; later six-rank qualification exposes further failures. |
| `895ab7274a` | Handle empty initial FVM graph ranks | Four-point import passes on six ranks. Distributed repair failures remain. |
| `7113fd984d` — WIP | Couple bounded candidate repair to strict construction admission | Builds; validation50 fails runtime and a fixed-cell-count assertion. Corrected in validation51. |
| `b214ff210b` — WIP | Retain one candidate for bounded repair after ordinary construction | Validation51 serial and six-rank flat runtime pass; cubic six-rank runtime remains incomplete. |
| `2a214b1058` — WIP | Geometry-first distributed conflict priority | Serial and priority/conflict tests pass; six-rank cubic runtime retains two geometry violations. |
| `4ffe4e507d` — WIP | Coalesce replicated concurrent edge demands | One-budget election passes; curved refinement completes, but subsequent coarsening exhausts remaining work. |
| `e9263060cf` | Order distributed proposals by geometric benefit and metric length | Serial 127 cases, MPI6 118/rank and all-worker Euler/RANS transfer/reload pass. Full M6 rejects at its time limit. |

`72c4684264` preserves the repair-accounting evidence and links its checkpoint.
Documentation-only commits may follow an implementation checkpoint.

## Evidence and remaining failures

| Campaign | Result | Consequence |
| --- | --- | --- |
| Full M6 validation43, MPI2 | Rejected at1800s:26,567 private edits;218,099 cached invalid cells;238,305 intermediate shape refusals. | Scheduling alone does not complete construction. Original CFD state is retained. |
| Validation46 | 21/22 focused cases pass. All selected integration cases pass, including positive sphere weights and second moments within the65536-point cap. | Cubic Euler/RANS still exhausts work4096. |
| Validation47 | 12 focused cases/2,183 assertions; connected serial125 cases/1,082,524 assertions pass. | Actual work accounting fixes serial runtime. MPI6 then aborts on a four-point sphere import with empty graph ranks. |
| Validation48 | Empty-rank reproducer passes on all six ranks. Connected MPI passes114/116 cases per rank. | Cubic runtime exhausts4096 with3 active workers. With all six active, affine runtime stalls with1 bad cell and cubic runtime exhausts4096. A collapse-filter counter fixture also fails. |
| Validation49 diagnostic | Whole-mesh topology leaves the affine bad cell; one insertion/flip removes it. Cubic45 bad cells become34 with topology and88 with full interior repair. | Larger late repair alone is insufficient. Diagnostic meshes are not runtime acceptance. |
| Validation50 | Build passes; focused14/16 cases,2,227/2,233 assertions pass. | Early candidate optimization starves ordinary joint placement:69 calls/0 accepted. Valid star repair creates32 cells; its fixture assumes12. |
| Validation54 | Focused 4 cases/31 assertions and all-six-worker 11 cases per rank pass. | Flat and cubic Euler/RANS complete with transfer/reload; curved total work 1,975/4,096. Connected serial127 and MPI6 118 cases/rank pass; full M6 rejects at its time limit. |
| Validation53 | MPI6 passes 9/10 cases per rank, including one-budget shared-edge election. | Curved refinement completes at work 4,066; coarsening exhausts its remaining 30. No accepted workflow yet. |
| Validation52 | Serial 16 cases/2,532 assertions pass; MPI6 passes 8/9 cases per rank. Flat transfer/reload and priority checks pass. | Cubic improves to zero shape/size failures and two geometry failures; work limit remains unchanged. |
| Validation51 | Build and16 focused cases/2,532 assertions pass. All six flat runtime and collapse-filter checks pass; cubic runtime rejects. | Retain one candidate for bounded repair after ordinary construction. Preserve all final gates; replace the star fixture's count restriction with an explicit volume check. |

The connected implementation reuses swaps and interior insertion, retains candidate
boundary/parameter bindings, reserves separate new-node identities, and refunds
unused work immediately. The global work limits and final gates are unchanged.
The collapse-filter fixture keeps its known local witness on two ownership groups
while every MPI rank participates; its original filter and mesh assertions remain.

Validation53 reuses duplicate-edge election for concurrent refinement before
import and work charging. The one-budget shared-edge regression passes on all six ranks. Refinement now
completes every mesh gate at work 4,066, but coarsening exhausts the remaining 30
units. The rejected private mesh has 209 cells and zero shape/size/geometry
violations; transfer/reload is still not reached. The documented coarsening
completion contract remains in force. Next: carry the full serial demand ordering
through distributed conflicts, including geometric benefit and metric length.

The short M6 CPU sample shows substantial MPI collective waiting alongside
active candidate reconstruction and donor queries. This is a five-second sample,
not whole-event timing. The full counters show 76,739 shape refusals and only
115 validated optimizer calls out of 2,113. Preserve the failed evidence and
review the difficult-region construction before adding further local fallbacks.

## Next acceptance steps

1. Resolve the focused runtime failures; verify final geometry, quality, volume,
   solution transfer and reload.
2. Pass connected serial and six-rank MPI qualification, including all-worker
   concurrent curved transactions.
3. Reassess full M6 under the agreed constraints. Accepted Euler adaptation precedes
   the remaining RANS/BL integration and qualification.

Other open requirements: changing-frame gradation, M6 complexity normalization,
curved BL support/chord/quadrature consumers, FIRST_ROW surface-error demands,
fitted-plane seams and automatic curved initialization. The normal-only relaxation
contract remains in force while tangential relaxation outside the protected band
is an unanswered question.

## Detailed records

- [Current handoff](HANDOFF_Codex.md): latest validation and immediate next step.
- [Curved construction and failure history](Handbook/DC18_CURVED_SOURCE.md).
- [DC-17/DC-18 completion handbook](Handbook/DC17_DC18_COMPLETION_2026_10_25.md).
- [Validation47](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation47/summary.json).
- [Validation48](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation48/summary.json).
- [Validation49 diagnostic](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation49/summary.json).
- [Validation50](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation50/summary.json).
- [Validation54](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation54/summary.json).
- [Validation53](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation53/summary.json).
- [Validation52](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation52/summary.json).
- [Validation51](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation51/summary.json).

The user approved pushing checkpoints to `rois1995/SU2`, branch
`codex/native-3d-bl`, on 2026-10-26. Push succeeded through `e9263060cf` on
[the fork branch](https://github.com/rois1995/SU2/tree/codex/native-3d-bl).
Validation53 is checkpointed and pushed with its incomplete workflow result.
Validation54 passes focused, all-worker and connected serial/MPI checks. Its
implementation remains pinned for the pending full-M6 campaign.

Current complete-workflow cost diagnosis and shared certificate design: [geometry verification cost](Handbook/GEOMETRY_VERIFICATION_COST.md).

Build46 PASS90.19s, focused temporary-association/geometry-certificate checks PASS93 assertions/2 cases. Mapped replay FAIL13.98s:318/342 probes score,9 geometry-progress edits,22 accepted/30 attempts,31 queued edges at unchanged work67207168. Zero-queue defect fixed; complete generation still fails. Fresh six-rank M6/session32081 was subsequently interrupted without debugger. Compiled filters237serial/214MPI/55atlas include all required cases.

M6 build46 intentionally interrupted908.19s without debugger, unchanged pins; no stage receipt, all6 workers active, all9 descendants verified terminal. Setup/source receipts retained; no native-failure/acceptance inference. Build47/session71724 now builds the connected lazy geometry-demand scheduling and accepted certificate ownership change, including first-activation proof and pending-association provenance. No gates/budgets changed.

Build47 PASS1210.22s. Lazy current-demand queue reaches49 accepted edits/17 geometry progress, but actual mapped workflow FAIL12.96s atunchanged67m arithmeticwork; a phase-reset emptyselection is not finalacceptance. Fourfocusedchecks initially114/116assertions; trace reversal was normalizing malformedendpoints. Build48 PASS48.15s, validatesintervals before reversal; all4focusedtests PASS9.88s. Profile/rawstate153cells90boundaryfaces0explicitcovers/traces; repeated-edit/publication/source/archive checks followed. Fullworkflow/M6 remainopen.

M6 build48 terminal1853.13s/nativeelapsedstopduringmetricupdate, unchangedpins; diagnosticdriverexit0butwrapperFAILmissing4qualitywindows. Retainedqualityentry667,993privatecells shows198,390sizebad/7,090geometrybad/64,554qualityinvalid and1.17mrepairfeatures. Noacceptedmesh/transfer; allownedprocessesgone. Non-stoppingDWARFsample identifiesgenericValidateCoverage/SplitAtlasEdges and repeatedboundarymovepreparation. Nextconnectedphase/generationdraftstillunapplied; no budget/gate changes.

Phase/generation change applied: build49PASS99.18s/1293pins; mappedruntimeFAIL13.85s atunchangedlimits with82accepted/26geometryprogress/1qualitystage and136optimizationtrials. Arithmetic67m exhaustedbeforelogical4096; both recorded, noacceptedreturn. Build50/session94342 updatesobsolete construction-handofftest toproductive descendantgenerations/qJ-free construction. No gate/budgetchanges. M6entry archives hashverified7files<95MiB each, private/notexactcheckpoint.
