# Native adaptation progress

Companion to [ADAPTATION_TODO.md](ADAPTATION_TODO.md), the acceptance ledger.
Updated: 2026-10-27. Detailed failures and provenance remain linked below.

## Current result

**DC-17 and DC-18 remain open. No full M6 adaptation has passed final acceptance.**

Construction is validity-first and q/J-free; metric/BL-aware improvement follows,
with all final q/J/size/height/geometry/transfer gates unchanged. The connected
coarsening closure, large-star movement, clipping correction and collision paging
fix passed build17 integration: serial179 cases/1,121,211 assertions (61.68s),
MPI6 all156 cases/rank (43.79s), unchanged pins. These include curved Euler/RANS
transfer/reload, repeated curved restart, and distributed history/recovery.
[Transfer-only ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md).


Build32/session33069 PASS78.14s/950 unchanged pins. Clean working-reference
reconstruction, coordinated shared paths and the native backend interface now
pass focused atlas_checks/session40070:45 cases/7561 assertions,38.79s,
unchanged pins. Actual two-event movement,16-event reconstruction with stable
normalized descriptors, nonseparating features, region history/holes, metadata
publication, native archive and source checks pass. Compiled-binary listing
verifies all14 intended new-file cases selected (45 total). Earlier test-tag
omissions and all failures remain documented; those earlier passes did not
qualify the omitted remapper cases.
Six-rank M6 build32/session90654 TERMINAL FAIL54.3039s, pins unchanged; largest sampled individual RSS392314880 bytes. Setup work174193 passed, but import rejected “Native3D physical triangle leaves its immutable oriented marker facets.” before meshing. Evidence: `staged57/cases/m6_stage_20261027T144634_2539145`. Full accepted mesh, broad serial/MPI, adaptation, transfer and restart remain unqualified.
The import failure is a missing reservation on read-only accepted-state face lookup.
Build33/session45827 PASS57.15s,950 unchanged pins. Focused atlas checks
PASS38.85s:45 cases/7610 assertions, including exact accepted lookup without a
reservation and rejection of altered coordinates/orientation/marker. M6 retry/session31910 TERMINAL REJECT190.56s with unchanged pins after814
private edits; unchanged trace-only metadata incorrectly invoked an empty
face-region proof. CFD state retained. Build34/session16636 PASS87.15s/950 unchanged pins. Expanded build34 serial_full/session69002 TERMINAL FAIL94.85s, pins unchanged: 226/227 cases and 1129578/1129580 assertions passed. Mapped native runtime curved Euler/RANS failed after 29 private edits at the unchanged atlas work budget 67207168. Accepted mesh, transfer and restart remain unqualified.

The fixed-reference compatibility probe now completes16 neighboring moves on both
a planar disk and a faceted M6 wing star with stable current capacities and
settled overlay counts. This qualifies representation/API scope only. Geometry
Central does not provide the required prescribed CFD graph recovery or movable
feature retirement through the inspected APIs; it is not selected as a complete
production network engine. The next connected candidate uses native shared-PSLG
reconstruction and a collective embedding certificate; independent canonical
edge proposals are never sufficient admission. See the
[architecture decision](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md) and
[probe measurements](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/reference_network_probe_v1/probe_summary.md).
No full accepted mesh, transfer or restart has been established.
Native build37 passed78.18s with1293 unchanged pins, including vendored Eigen.
Build38 passed111.93s/1293 unchanged pins. Its focused run failed8/53 cases
(7799/7807 assertions passed,24.94s, pins unchanged): region-boundary
comparison confused segmentation with geometry, and one new cubic fixture
swapped two controls. Build39 corrects both and the focused run passes. The connected candidate also adds owner closure:
metadata refresh must replace its owning volume rows so existing cell versions
and notifications invalidate dependent work. Donor-generation tracing found no
separate freeze/persistence mismatch. Build39 now passes the53-case focused run (8247 assertions,47.83s, unchanged
pins), including16 overlapping moves. Six-rank M6 build39 diagnostic is live;
full accepted adaptation/transfer/restart remains unqualified.

Build35 and build36 passed with 950 unchanged pins. Build36 serial_full/session68944 TERMINAL FAIL99.7804s, pins unchanged: 226/228 cases and 1129639/1129642 assertions passed; largest sampled individual RSS145432576 bytes. Evidence: `staged57/cases/serial_full_20261027T153436_2673844`. The overlapping-star test exhausted atlas work on event 5 (sixth move); curved Euler/RANS runtime again exhausted 67207168 work after 29 accepted private edits and 35 attempts. Retained path history is an architectural blockage not resolved by discarding interior cuts. Develop a connected strategy before MPI or M6 retry; accepted mesh, transfer and restart remain unqualified.


Build19 serial182 cases/1,121,344 assertions and MPI6 all159 cases/rank passed;
its four-window M6 diagnostic worsened early quality debt and did not establish
boundary freedom or convergence. This prompted the connected chart-mobility work.
[Stage comparison](Handbook/M6_QUALITY_STAGE_PROGRESS.md).

Build18/session62205 PASS726.79s/933 unchanged pins. Focused serial stage_checks/
session45262 PASS10.5101s/6 cases/4419 assertions; six-rank stage_checks6/session51405
PASS11.5294s. M6 stage capture/session11197 is TERMINAL PASS in623.327s: pins
unchanged, six complete rank shards, four quality windows and original donors
unchanged. Broad serial/session28247 and MPI6/session24105 pass. This validates quality-stage observations and
complete private-state capture; it does not establish full convergence, exact
continuation or final mesh acceptance. Focused checks compare observed/unobserved mesh,
work and field queries, independently check shape/height debt, and exercise
collective observer failure, memory admission and serialization. Then the exact
build13 frozen M6 field is captured through four quality-progress windows, with
per-rank original donors and matching boundary parameters. Python receipt checks
pass valid/empty shards and reject truncation, changed donors, missing workers and
mismatched rounds. The completed short capture shows mixed debt over only248
rounds; see [design and quantitative table](Handbook/M6_QUALITY_STAGE_PROGRESS.md).

The difficult M6 build13 run remains TERMINAL REJECT:1802.45s native,774,314 private
cells,275,115 cached-invalid cells and1,240,202 repair demands after112,024 private
publications. Coarsening never started. Solver exit0 was not adaptation success;
CFD state was retained. Its counters alone do not prove opposing quality debt,
boundary pins or parallel throughput caused rejection. The new capture addresses
that evidence gap before changing quality admission or coordination.

Latest pushed `17902da07b`: quality-staging WIP; build/focused checks pass, M6 diagnostic complete; broader serial/MPI integration passes.
`64953f3b69` preserves completed diagnostic receipts and enrichment diagnosis. `f27662eb31` preserves the
passing build17 integration receipts. Prior failures, fixes and limited witnesses
remain in [archived validation history](Handbook/VALIDATION_HISTORY_THROUGH_BUILD17.md)
and [staged implementation](Handbook/STAGED_NATIVE_ADAPTATION.md). Full M6,
curved chart mobility and the remaining metric/BL scope are still required.
External-SSD references are [archived locally](Handbook/ADAPTATION_REFERENCE_INPUTS.md).

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
