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

Architecture/publication checkpoint: `ca60c623c0` (WIP); build39 corrections
and focused receipts are saved in its follow-up checkpoint. Native build39 passed75.18s with1293
unchanged pins. Focused atlas_checks `staged57/cases/atlas_checks_20261027T173019_3050687`
passes53 cases/8247 assertions,47.83s, unchanged pins. This includes16 alternating
overlapping-star moves with per-phase descriptor/bit-width/work/storage bounds,
actual curved operator movement, collective embedding rejection, owner publication,
source and archive checks. It does not qualify an accepted mesh or full workflow.

Six-rank M6 build39 diagnostic is TERMINAL INTERRUPTED: session96782,
`staged57/cases/m6_stage_20261027T173120_3053606`, wrapper -15/1295.38s,
unchanged pins. **Instrumentation confound:** initial timed GDB left a worker
stopped, explaining later vote/release skew. No native MPI mismatch is established.
All owned descendants are verified gone; see its `DIAGNOSTIC_INTERRUPTION.md` and
`termination_receipt.json`. Do not reuse this as M6 acceptance or a native stall.
Run a fresh M6 without attach after connected workflow checks.

Build40 passed36.15s/1293 unchanged pins. Numeric cost case
`staged57/cases/cost_checks_20261027T175644_3119990` passes16 moves and its
independent summary:7 points/10 faces,10–16 cover pieces,10–15 trace pieces,
54–55 bits,4.29–11.15 million arithmetic work,5.76–6.27MB admitted peak,
0.60–0.66s per private move. Returning phases satisfy descriptor/work/byte bounds.
This is small private surface-move scope, not full-event throughput.

Build40 broad serial `staged57/cases/serial_full_20261027T175719_3121798`
TERMINAL FAIL102.73s, unchanged pins:234/235 cases,1130211/1130213 assertions.
Curved Euler/RANS mapped runtime still exhausts67207168 arithmetic work after29
private edits/35 attempts, with8 geometry-debt cells and no metric/height debt.
The identical whole-workflow failure is not resolved by the neighboring-path fix.
Do not raise limits or claim capability completion. MPI broad and wider workflows
are still due after this serial blocker is understood.

Build41–45 diagnostics identify why curved runtime queues no geometry edges:
all457 build45 split probes throw before scoring (133 boundary points passed to
interior-only binding,324 transient projection face identity refusals). Build45
is terminal PASS141.20s; focused certificate test1 passes, test2 has a private-view
fixture setup error. Runtime replay fails17.91s unchanged pins/gates. Build46 passed90.19s; focused93 assertions/2 cases PASS6.96s. It
corrects canonical boundary strata and transient forward-association containment
and error queries. Mapped replay FAIL13.98s:318/342 probes score,9 geometry-progress
edits,22 accepted/30 attempts,31 queued edges at unchanged work67,207,168.
Fresh six-rank M6 build46/session32081 is LIVE without debugger attach; keep
pinned sources/helpers unchanged. Compiled filters PASS237serial/214MPI/55atlas
with all intended cases, including both new tests and separate wide workflows. See
[verification diagnosis](Handbook/GEOMETRY_VERIFICATION_COST.md). No accepted
mesh or full workflow has been established; broad and fresh M6 remain due.

The [architecture review](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md)
compares Gmsh, CGAL, Mmg, Omega_h and Geometry Central. The narrow fixed-input
Geometry Central probe completes16 moves on planar and faceted M6 stars, but
cannot supply prescribed CFD graph recovery/movable feature retirement through
the inspected APIs; no production dependency is added. Native edge proposals are
admitted only through one shared reference PSLG, disjoint face disks, positive
maps and exact region replacement. Eigen replaces custom PCG. Cover publication
now requires replacement/versioning of owning volume rows. Donor freezing and
accepted-generation persistence tracing found no separate mismatch.

Preserve earlier failures: build36 overlapping stars exhausted work on move6;
curved Euler/RANS exhausted67207168 after29 private edits. Build38 failed8/53
focused cases: unchanged boundary subdivision was mistaken for changed geometry,
and one new cubic fixture swapped controls. Build39 corrects these without
relaxing the exact chain or embedding gates. Full history/provenance:
[curved implementation](Handbook/CURVED_ATLAS_IMPLEMENTATION.md),
[older validation](Handbook/VALIDATION_HISTORY_THROUGH_BUILD32.md).

Compiled build38 filter audit selects all intended cases:235 serial (66 required),
212 MPI (52 required),53 focused (29 required). Build39 changes no case names or
filters; rerun the inexpensive binary audit before broader regression. The helper
now also requires mapped runtime and audits the single M6 frozen-field case.
Separately schedule the existing solved Euler, primitive runtime and resolved
Euler/restart workflows: they are not selected by current broad filters. Use the
existing `staged57/build_checkpoint.py`, `run_cases.py` and
`audit_compiled_filters.py` with unique receipts. Preserve successful stages and
failures; do not raise limits or add alternate path fallbacks after a stall.

The [geometry interface](Handbook/GEOMETRY_BACKEND_PLAN.md) remains backend neutral;
original cubic authority and schema8 archives remain authoritative. OCCT/importers
remain planned. No full M6 adaptation, transfer or restart is accepted.

## Operations

Push authorization persists for origin rois1995/SU2, branch codex/native-3d-bl;
no new permission question. Git metadata requires escalated execution and explicit
paths. TODO is intentionally local/untracked; leave unrelated dc11/dc15 logs and
planning files alone. `git status --short --ignore-submodules` avoids broken Eigen
metadata. Run one owned heavy job at a time, nice19, one build worker, single-thread
libraries and at most six local MPI ranks. Check host RAM/load and live handles;
use 4TB `integration_evidence/build-integrated-v2/compile-tmp`, never root/tmp for
builds. External-SSD inputs are archived and hash verified: [reference archive](Handbook/ADAPTATION_REFERENCE_INPUTS.md).

Current complete-workflow cost diagnosis and shared certificate design: [geometry verification cost](Handbook/GEOMETRY_VERIFICATION_COST.md).
