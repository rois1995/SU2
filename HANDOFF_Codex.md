# Codex handoff — current state, 2026-10-27

Continue until DC-17/DC-18 and ensuing defects are verified closed. Maintain the
local [TODO](ADAPTATION_TODO.md), tracked [progress](ADAPTATION_PROGRESS.md), and
[transfer ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md). Architecture, cadence,
resource and staged-construction instructions remain in [AGENTS.md](AGENTS.md).

## Goal and open obligations

Accepted full ONERA M6 Euler adaptation, then RANS, through general metric and
geometry operators: q>=.20, J>=.05, edge<=1.8, requested h0 within 10%, 500k
metric complexity (not tet count), prescribed geometry/body/features,
conservative transfer and restart. ADAP_HAUSD is in mesh units independently of
h0. No full M6 has passed acceptance.

Construction is validity-first: no q/J measurement, shape ranking or quality
veto; exact positivity, topology, authority/contact and body separation remain
mandatory. Quality/BL-aware improvement enforces unchanged final gates. Boundary
nodes may redistribute on prescribed geometry. Complete the distributed workflow
and solver/transfer/restart checks; a local fixture or private capture is not an
accepted mesh. See [staged scope](Handbook/STAGED_NATIVE_ADAPTATION.md) and
[curved mobility](Handbook/CURVED_BOUNDARY_MOBILITY.md).

Open work includes changing-frame gradation (21/9,393 violations, worst 1.406),
normal-only M6 budget profiles above 500k (best 821,265 +/- 53,688), whole-field
normalization, curved BL support/chord/quadrature, FIRST_ROW surface-error demands,
fitted seams, automatic curved initialization and transfer/restart qualification.
Normal-only contract binds; tangential growth outside the protected band remains
unanswered. Preserve all final gates and avoid scenario-specific dispatch.

## Live work and validation

Build33/session45827 PASS57.15s, 950 unchanged pins. Focused atlas checks pass
45 cases/7610 assertions in 38.85s, including accepted lookup without a reservation
and rejection of altered coordinates/orientation/marker. These focused results do
not qualify an accepted M6 mesh or whole workflow. The prior build32 M6 diagnostic
failed at import before meshing; its evidence and the intervening fix are recorded
in [implementation history](Handbook/CURVED_ATLAS_IMPLEMENTATION.md).

Six-rank M6 build33/session31910 TERMINAL REJECT190.56s, pins unchanged. It passed
import/source preparation and made814 private edits, then rejected “Empty reference
coverage replacement.” Original CFD mesh/solution retained; no quality-entry
snapshot, so the diagnostic wrapper correctly fails. Evidence: `staged57/cases/m6_stage_20261027T145416_2560816`.
Trace-only unchanged metadata incorrectly invoked a nonempty face-region proof.
Build34/session16636 PASS87.15s/950 unchanged pins. The guard skips that proof
only when both face lists are empty; changed traces still require all incident
faces. Expanded build34 serial_full/session69002 TERMINAL FAIL94.85s, pins unchanged: 226/227 cases and 1129578/1129580 assertions passed. Mapped native runtime curved Euler/RANS failed after 29 private edits at the unchanged atlas work budget 67207168. Accepted mesh, transfer and restart remain unqualified.

Build35 and build36 passed with 950 unchanged pins. Build36 serial_full/session68944 TERMINAL FAIL99.7804s, pins unchanged: 226/228 cases and 1129639/1129642 assertions passed; largest sampled individual RSS145432576 bytes. Evidence: `staged57/cases/serial_full_20261027T153436_2673844`. The overlapping-star test exhausted atlas work on event 5 (sixth move); curved Euler/RANS runtime again exhausted 67207168 work after 29 accepted private edits and 35 attempts. Retained path history is an architectural blockage not resolved by discarding interior cuts. Develop a connected strategy before MPI or M6 retry; accepted mesh, transfer and restart remain unqualified.

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
Build38 is validating the connected candidate plus publication owner closure:
metadata refresh must replace its owning volume rows so existing cell versions
and notifications invalidate dependent work. Donor-generation tracing found no
separate freeze/persistence mismatch. No runtime pass is claimed yet.

Latest pushed checkpoint: `a61e28bf58` (WIP; new connected prototype unbuilt). Cross-chart design, retained failures and
validation: [curved atlas implementation](Handbook/CURVED_ATLAS_IMPLEMENTATION.md).
The native [geometry-backend interface](Handbook/GEOMETRY_BACKEND_PLAN.md) is
implemented and exercised; Open CASCADE/importers remain planned. Cubic geometry
and existing archives remain authoritative. Detailed build13–32 evidence,
checkpoints and links: [validation history](Handbook/VALIDATION_HISTORY_THROUGH_BUILD32.md).
The capture analyzer supports legacy/schema8 payloads; synthetic checks pass.

The broad filters now include the previously omitted barycentric and connected
runtime/history/veto cases. Build36 compiled selection verified all56 required cases:228 serial,205 MPI.
Serial failure is recorded above; MPI was not repeated after that failure.
Use existing `staged57/build_checkpoint.py` and `run_cases.py` with unique source,
binary, runtime/input/helper and resource receipts. Preserve completed evidence;
do not repeat blind timeouts or add local fallbacks without addressing the blockage.

## Operations

Push authorization persists for origin rois1995/SU2, branch codex/native-3d-bl;
no new permission question. Git metadata requires escalated execution and explicit
paths. TODO is intentionally local/untracked; leave unrelated dc11/dc15 logs and
planning files alone. `git status --short --ignore-submodules` avoids broken Eigen
metadata. Run one owned heavy job at a time, nice19, one build worker, single-thread
libraries and at most six local MPI ranks. Check host RAM/load and live handles;
use 4TB `integration_evidence/build-integrated-v2/compile-tmp`, never root/tmp for
builds. External-SSD inputs are archived and hash verified: [reference archive](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
