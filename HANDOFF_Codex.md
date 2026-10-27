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

## Live work and next validation

ACTIVE: build15/session90655,933 frozen source pins,one nice19 worker; all main
C++/tests/helpers are FROZEN. No other heavy job. Build14/session8407 passed594.86s.
Connected_fix/session39461 is terminal FAIL:25.65s,77/80 cases,18,339/18,502
assertions, unchanged pins. Curved remeshing completed; RANS transfer failed.

Integrated coarsening correction: strict checkpoints, net cell reduction across
removal+quality closure, unchanged-support attempt ledger, finite-queue rollback,
byte-accounted dormant Directory, persistent blocked directions. It restores exact
owned cells/versions/parameters; collateral directions return. Real resource limits
still reject. Flat and mapped height-constrained rollback variants pass; stretched
variant and corrected large-star fixture await validation. No construction q/J floor.
The old isolated copy in `dc17_18_completion_v1/coarsen_closure_worktree` is stale;
do not copy it over main.

Actual saved6×128-cell transfer replay isolates NaN intersections in shared
ConvexClipping.hpp: raw overlap NaN, split overlap.97888950039/lost.02111049961.
Root's normalized signed-distance interpolation restores overlap1 and lost3.6e-16.
Polygon and Polyhedron share CutFraction; nonfinite overlap volumes now fail
explicitly. Reproducer/receipts: `staged57/transfer_coverage`. Three clipping regressions are integrated. All agent sources were released before
build15; no agent may edit pinned files or start another job. Full diagnosis:
[transfer clipping](Handbook/TRANSFER_CLIPPING_UNDERFLOW.md).
Two Distributed fixtures are corrected:80-cell input must actually have quality
debt; orthogonal stretch/pullback metric replaces invalid sheared first-height setup.
Phase-census imports no longer debit an absent update timer.

Next: finish build15, connected_fix, then appropriate
broader serial/MPI6 qualification. Existing `staged57/build_checkpoint.py` and
`run_cases.py` save unique pinned/resource receipts. Connected selector includes
coarsen closure and clipping regressions; broad selectors include conservative
projection/transfer tests. Keep compiled sources/helpers frozen until owned job
handle is terminal. No unqualified success claims or relaxed conservation checks.

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

Latest pushed `6aee162837`: coarsening closure, clipping correction and regressions,
with failed-workflow/replay evidence; integrated validation is pending. Code536025f6e4 contains
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
