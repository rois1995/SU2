# Codex handoff — current state, 2026-10-28

Continue DC-17/DC-18 and related defects under [AGENTS.md](AGENTS.md). Keep
[progress](ADAPTATION_PROGRESS.md), local [TODO](ADAPTATION_TODO.md), and the
[transfer ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md) current.

## Goal and gates

Deliver accepted ONERA M6 Euler adaptation followed by the requested BL/RANS
workflow, including conservative transfer, BDF/history, publication and restart.
Final gates remain q>=.20, J>=.05, edge<=1.8, requested h0 within 10%, 500k
metric complexity, prescribed geometry, body separation, features/contact and
source/transfer checks. ADAP_HAUSD is an absolute mesh-unit length, independent
of h0. No full M6 adaptation has passed acceptance.

Construction is validity-first: exact positive cells, topology, geometry and
contact are mandatory; q/J measurement, ranking and quality veto belong to the
subsequent improvement stage. Boundary nodes may move on prescribed geometry;
scenario labels must not dispatch reconstruction.

## Current evidence and next decision

Build75 PASS1303.335s and corrected build76 PASS216.293s, pins unchanged. Serial replay76 PASS14.127s: a final-private-state edge proposal produces 2→4 positive cells, 1,528,800 operator work and geometry excess 9.454e-7→3.907e-9 with exhaustion false. A 100,000-work run refuses with zero output and exhaustion true; sticky multi-charge, divide-by-zero and NaN backend checks pass. This is not whole-mesh acceptance. Compiled filter audit76 failed before execution because approval-service initialization reported no space; collective-stop MPI and coupled legacy/32-event checks remain pending. All three owned handles are terminal; no jobs are running. The tracked-code WIP checkpoint is saved locally. Push was attempted but failed before execution because the approval service could not initialize; raw receipts and the serial harness remain local.

Build74 passed132.203s, 1293 pins unchanged. The combined legacy-normalization + 16-event neighboring-move regression compiles; runtime/filter checks and push are blocked by approval-service storage initialization failure. Local review checkpoint: `staged57/review_checkpoint74.{patch,json}` (not a Git commit).

Build73 PASS141.2348s with 1293 unchanged pins. Atlas checks pass 65 cases/18,668
assertions in 127.973s; repeated MPI6 publication passes 69.929s. These qualify
focused primitives, not the complete workflow. Mapped runtime
`staged57/cases/mapped_runtime_20261028T024037_335719` FAIL12.873s: curved
variants reached 17 splits, zero collapses, 18 split and 18 collapse passes,
one quality stage and unchanged work/arithmetic limits; flat completed 31 edits.
Mapped work trace `staged57/cases/mapped_work_trace_20261028T024728_354109`
FAIL12.827s. See [geometry cost](Handbook/GEOMETRY_VERIFICATION_COST.md).

M6 build72 `staged57/m6_runs/euler_build72_pair_lifecycle` TERMINAL REJECT after
1862.171s, solver exit 0, pins unchanged, nine owned processes gone. Two split
passes and one collapse pass produced 6,287 splits, 2,025 collapses, 44,861
geometry rejections and zero quality stages. Atlas work summed 369,648,840,116
across ranks. Sampled process-tree maximum RSS 3,529,699,328 bytes is not an
allocation peak. The case-local hash-backed summary preserves rank details and
inputs; no accepted mesh or transfer follows.

Current-source audit found no remaining feedback from face microknots into
persistent traces; entry normalization already replaces legacy shared paths with
incident-owner closure. Independent canonical paths still do not provide general
joint network recovery. The unbuilt endpoint-curvature veto was removed: it could
reject a face that passes the actual geometry certificate. Retain source73's rejection-only separation and shared coverage
validation, all existing final gates, and unchanged resource limits. The curved trace shows productive split passes interrupted by repeated unsuccessful
collapse work and then exhaustion; a geometric construction stall is not established.
Replay a remaining demanded edge with fresh work before choosing another construction
strategy. No new M6 run before that distinction is resolved. See [working-surface decision](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md)
and [metric-length design](Handbook/METRIC_LENGTH_SATISFACTION_DESIGN.md).

Other open scope includes changing-frame gradation, M6 complexity normalization,
curved BL support/chord/quadrature, FIRST_ROW geometry demand, fitted seams,
automatic curved initialization and full repeated adaptation/transfer/restart.
Private snapshots and passing fixtures do not close these obligations. Detailed
history and earlier failures remain in [progress](ADAPTATION_PROGRESS.md) and
[curved implementation](Handbook/CURVED_ATLAS_IMPLEMENTATION.md).

## Operations

Push authorization persists for origin rois1995/SU2, branch codex/native-3d-bl;
the current WIP checkpoint is local and incomplete. Git metadata requires escalated execution
and explicit paths. TODO is local/untracked; leave unrelated files alone. Run one
owned heavy job at a time, nice19, one build worker, single-thread libraries and
at most six local MPI ranks. Check load, available RAM and owned handles first;
use `integration_evidence/build-integrated-v2/compile-tmp` on 4TB. Preserve
unique source, binary, configuration, input, helper and resource receipts; resume
only after verifying provenance. [Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
