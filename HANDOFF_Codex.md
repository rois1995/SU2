# Codex handoff — current state, 2026-10-27

Continue until DC-17/DC-18 and ensuing defects are verified closed. Keep the local
[TODO](ADAPTATION_TODO.md) and tracked [progress companion](ADAPTATION_PROGRESS.md)
current. Do not mistake a local repair, checkpoint or passing fixture for full
adaptation acceptance. Permanent architecture/cadence/resource rules: [AGENTS.md](AGENTS.md).
Keep transfer-specific bugs, improvements and validation status current in the
[transfer operators ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md).

## Goal and direction

Accepted full ONERA M6 Euler adaptation, then RANS, through general metric/geometry
operators: q>=.20,J>=.05,edge<=1.8,h0 within10%,500k metric complexity (not tet count),
prescribed geometry/body/features, conservative transfer and restart. Curvature uses
ADAP_HAUSD in mesh units independently ofh0. No full M6 has passed acceptance.

Construction is validity-first: no q/J measurement/ranking/floors; exact positivity,
topology, authority membership/contact remain hard. Quality/BL-aware improvement
follows with unchanged final gates. Physical boundary coordinates may change.
Large complete vertex stars now move atomically across owners; original curved
chart restrictions remain unresolved. See [staged scope](Handbook/STAGED_NATIVE_ADAPTATION.md)
and [curved mobility](Handbook/CURVED_BOUNDARY_MOBILITY.md).

## Latest validation and next step


Implementation in progress: stage existing-point relaxation before topology
improvement/enrichment, with bounded shared work and accepted nested operation
counts. M6 shard comparison found 3,215 new interior points, no removed or moved
entry points, and unchanged physical bindings. All 947 commits were nested
optimizer results, hidden by the old reconnected counter. This change is not yet
built or validated and does not resolve cross-chart mobility.

Build18 M6 stage/session11197 is TERMINAL PASS:623.327s, pins unchanged, six
complete rank shards and four quality windows; original donors unchanged. The
diagnostic stop is private state, not exact continuation or final acceptance.
No owned job remains live; all main C++/tests/helpers stayed pinned.
Build18/session62205 PASS726.79s/933 unchanged pins. Focused serial stage_checks/
session45262 PASS10.5101s,6 cases/4419 assertions,unchanged pins. Six-rank
stage_checks6/session51405 PASS11.5294s,all pins unchanged. Host preflight saved
with34.77GB available RAM and no owned heavy predecessor. Build17/session53915 passed135.20s with933 unchanged pins.
Broad serial/session77363 PASS61.68s:179 cases/1,121,211 assertions. MPI6/session4734
PASS43.79s:156 cases on every rank, all pins unchanged. The first MPI launch97981
stopped on a serial-only fixture selection error; its receipt is preserved. Serial
geometry transfer tests stay in serial_full; MPI uses distributed counterparts.

Validated integration: strict coarsening checkpoints/rollback and support-based
collapse retirement, complete coordinate-star movement, shared normalized clipping
and nonfinite-volume handling, collision paging with empty ordinary removal lists.
Curved Euler/RANS transfer/reload, repeated curved lifecycle/restart, distributed
transfer/kernel/history/recovery checks pass. This is not full M6 acceptance.
[Transfer-only ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md).

Quality-stage measurement/capture implementation passed focused serial/MPI checks,
with admission/election and final gates unchanged. Quality-staging edits are now active.
Incremental diagnostics reuse the one-cell census, Add/Drop cache updates,
FirstHeightReport and SurfaceError results; no construction q/J, duplicate audit or
per-proposal collective. Rank-local traces/shards preserve original frozen donors,
matching private geometry and explicit unknown/partial state. Observer storage is
admitted; a failed observer cannot run again on exit. Optional diagnostic stop
saves observation_stop shards before electing its intentional exception.

Focused serial stage_checks/session45262 PASS10.5101s/6 cases/4419 assertions;
MPI6 stage_checks6/session51405 PASS11.5294s. The completed M6 stage capture shows
mixed debt movement over only248 rounds: size debt fell, q/J debt shifted slightly,
invalid-cell count rose. See the quantitative entry/stop table in
[quality-stage progress](Handbook/M6_QUALITY_STAGE_PROGRESS.md). This is not a
full convergence measurement or a reason to claim M6 acceptance.
Serial/MPI checks passed
constant-field/height debt, unchanged work/query/mesh, observer failure/storage
and serialization. M6 used the
bit-identical build13 archived sensor and unchanged configuration/geometry; it
stopped after four quality windows and checked complete six-rank shards, lengths,
donor hashes and synchronized trace rounds. This is diagnostic private state,
not exact continuation, full mesh acceptance or transfer. Python receipt self-check
passes valid cases and six malformed/truncated/provenance cases.
Design and diagnostic acceptance: [quality-stage progress](Handbook/M6_QUALITY_STAGE_PROGRESS.md).

Existing `staged57/build_checkpoint.py` and `run_cases.py` retain unique pinned
resource receipts. Broader selectors cover conservative transfer, recovery/history
and repeated curved restart. The named precision contract runs in the current
binary precision; no separate float-build qualification is claimed.

## Difficult target evidence and remaining capability

Full M6 build13/session70428 is TERMINAL REJECT:1863.87s wall,1802.45s native;
solver0/nativefalse, all pins unchanged. It reached repair, never coarsening:
774,314 private cells,275,115 cached invalid cells,1,240,202 repair demands after
112,024 private publications. Work1,218,977/19,414,976; time exhausted. Three cells
were unmeasured. Original CFD state retained. Sampled process-tree RSS maximum
4.616GB, individual0.877GB; neither establishes a whole-job peak.
`staged57/m6_runs/euler_build13/diagnosis.json` preserves six-rank evidence.

Coarsening alone cannot close M6. Local repair admission allows size progress OR
minimum normalized q/J progress, while whole-stage progress windows count accepted
operations rather than aggregate deficit reduction. This permits opposing debt,
but current counters do not prove it caused M6's timeout. Do not blame chart pins
without evidence. Capture/assess complete quality-stage progress before the next
connected correction; do not repeat another full timeout or add local variants.

Other open scope: changing-frame gradation21/9,393 violations (worst1.406), normal-only
M6 budget profiles above500k (best821,265+/-53,688), whole-field normalization,
curved BL support/chord/quadrature, FIRST_ROW surface-error demands, fitted seams,
automatic curved initialization, and complete solution-transfer/restart qualification.
Normal-only contract still binds; tangential growth outside protected band unanswered.

## Checkpoints and preserved history

Latest pushed `b6a976235f`: quality-stage diagnostics/capture WIP; its bounded
M6 diagnostic is now complete.
`f27662eb31` preserves passing broad serial/MPI integration receipts and the
quality-stage measurement design. `c659e03cf5` contains the corrected large-star
fixture and collision paging correction. Earlier
`6aee162837` contains coarsening closure and clipping correction/regressions. Code536025f6e4 contains
construction generations, fair repair service and complete-star movement. Earlier builds and local
repair witnesses do not validate current code. Details: [staged history](Handbook/STAGED_NATIVE_ADAPTATION.md),
[completion](Handbook/DC17_DC18_COMPLETION_2026_10_25.md), [curved source](Handbook/DC18_CURVED_SOURCE.md),
[validation47–54](Handbook/VALIDATION47_54_HANDOFF_HISTORY.md),
[transaction replay](Handbook/M6_TRANSACTION_REPLAY.md),
[independent review](Handbook/M6_REVIEW_RECONCILIATION.md),
[MPI/paper assessment](Handbook/MPI_AUDIT_AND_LOSEILLE_ASSESSMENT.md).

## Operations

Push authorization persists: origin rois1995/SU2, branch codex/native-3d-bl; no new
permission question. Git metadata requires escalated execution; use explicit paths.
TODO intentionally local/untracked. Leave unrelated dc11/dc15 logs and planning
files alone. `git status --short --ignore-submodules` avoids broken Eigen metadata.
One owned heavy job, nice19,one compiler worker,single-thread libraries,max6MPIranks.
Check actual host RAM/load and live handles first. Use 4TB compile-tmp:
`integration_evidence/build-integrated-v2/compile-tmp`, never root/tmp for builds.
External-SSD references are locally archived/hash-verified:
[reference archive](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
