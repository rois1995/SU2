# Native adaptation progress

Companion to [ADAPTATION_TODO.md](ADAPTATION_TODO.md), the acceptance ledger.
Updated: 2026-10-26. Detailed failures and provenance remain linked below.

## Current result

**DC-17 and DC-18 remain open. No full M6 adaptation has passed final acceptance.**
Validation54 passes the full six-worker flat and cubic Euler/RANS workflows,
including conservative BDF2 transfer and SU2/CGNS reload. All 11 selected cases
pass on every rank. The curved workflow uses 1,975 of 4,096 work units: 1,750 for
refinement and 225 for coarsening. The flat workflow uses 105 total. Full demand
priority now precedes ownership in shared selection, dependency reservations and
peer contact selection; independent proposals still commit concurrently.

The focused priority tests also pass (4 cases, 31 assertions). Connected serial qualification passes 127 cases and 1,082,802 assertions.
Connected MPI qualification also passes: 118 cases on each of six ranks.
Full M6 Euler6 rejected at the native time limit: 24,972 private edits, 427,852
private cells, 235,717 cached invalid cells and 235,227 queued edges. It reached
six simultaneous commits, but no mesh or transfer was accepted. Original CFD
state was retained. Driver exit zero does not change that result.

Required gates remain q ≥ 0.20, J ≥ 0.05, metric edges ≤ 1.8, first height within
10%, geometry preservation, the configured 500k metric-complexity target,
conservative transfer and restart. The target is not a literal tetrahedron count.
Curvature uses `ADAP_HAUSD` in mesh units, independently of h0.

Work budgets count attempted proposals and repair trials, not generated cells.
The 4,096 limit quoted below belongs to the small regression workflows. Automatic
production work is bounded by `max(4096, 64 × initial cells)`; time, storage,
local-cavity and sampling limits separately bound resource use. These limits do
not replace the mesh-quality gates. Exhaustion means the search is unfinished,
not that the requested quality is impossible.

The supplied MPI audit request and Loseille/Alauzet/Menier paper were assessed
against the current code and validation54:
[assessment and next decisions](Handbook/MPI_AUDIT_AND_LOSEILLE_ASSESSMENT.md).
Native already uses a separate weighted tetrahedron partition; M6 enabled it.
The paper's current/target metric mismatch model and hierarchical interface
recovery are candidates to evaluate, not implemented fixes. Its published quality
outliers do not establish our strict acceptance gates. No new benchmark or C++
change was made for this assessment; DC-17/DC-18 remain open.

Follow-up diagnostic55 is terminal (2026-10-26): see the linked
[decision package](Handbook/M6_DIAGNOSTIC_DECISION.md), with construction/MPI/weight
reports and raw receipts. Original coordinates/global IDs and all 61,330 metric
tensors round-trip exactly. Weighted partition reduces proxy max/mean 4.699 to 1.08449;
offline current-formula sum matches production in aggregate only. The replica has
no 1024 clipping (maximum 591); production per-cell weights were not exported.
Matched work-2048 six-rank replays have 476 private edits/879 shape refusals and no
accepted mesh. Optional PMPI records 18.74–26.00 s/rank inside MPI over 62.72 s, with no
trace overflow; useful compute/wait decomposition and true instrumentation overhead
remain unresolved. Independent original-P1 replay reproduces 16 positive-volume shape
refusals among 28 captured patches. No repaired accepted witness exists. Production
code/gates unchanged. Next: the specific complete-context transaction experiment in
the decision package, not another full-M6 timeout or partition API replacement.

The supplied independent review of `f5e9456f06` is now
[reconciled and preserved](Handbook/M6_REVIEW_RECONCILIATION.md). Corrected the
exemplar table to show failed-cell and full-patch q/J separately, and relabeled
the 1.428 timing quotient as hypothetical accounting rather than a speedup ceiling.
The next replay starts with edge23754–24333 only and requires exact pre-transaction
context before repair. Local transaction acceptance and final-patch edge acceptance
are distinct. No production fix, new numerical validation or capability closure is
claimed by this documentation checkpoint.

Diagnostic56 capture is terminal and passes (MPI6, native72.287s, launch80.475s;
no accepted mesh). It preserves the full303,428-cell current mesh at round4 and
70 selected transaction events. The initial split and first shape failure reproduce
exactly; a fresh directory confirms dependency versions/reservation and external
contact. Maximum integrated candidate edge1.59836 passes; q=.183972/J=.042790 fail.
The transaction uses all12 growth iterations, then13 reconnections. Independent
checks verify722 dependency records against global ownership. Captured geometry
parameters agree across all ranks. Lossless deduplication keeps the capture below
25MiB; the initial archive-size refusal is retained. Full construction-call replay
and one bounded fixed-skin connectivity repair are being implemented from these
saved inputs, without another MPI prefix. No construction-policy change or DC closure.

## Checkpoints

| Checkpoint | Change | Validation status |
| --- | --- | --- |
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

- [Current handoff](HANDOFF_Codex.md): active handle and immediate next action.
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
