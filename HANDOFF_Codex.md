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

## Live work and validation

**Cross-chart mobility implementation is active and not integration-validated.**
Build20 is terminal setup failure (3.09s, pins unchanged): Meson could not locate Ninja; the runner now records NINJA explicitly.

Checkpoint `7e41461874` saves the tested primitive. Exact arithmetic passed the standalone `staged57/atlas_arithmetic1` run: four cases,
308 assertions, 17.90s including compilation, unchanged pins; its job is terminal.
Build21/session45516 is live: one low-priority build worker, 944 pinned inputs.
All C++ and validation helpers are frozen. Geometry, parameter/publication,
remapper, source-envelope and operator/lifecycle wiring are implemented for
first compilation; runtime qualification remains open.
See [implementation contract](Handbook/CURVED_ATLAS_IMPLEMENTATION.md).
The latest pushed primitive checkpoint is `7e41461874`; connected changes remain WIP.
The staged topology analyzer now recognizes legacy and schema 8 parameter payloads;
its synthetic header, appendix and truncation self-check passes. The `atlas_checks`
case and broad-campaign tags are prepared for the connected build, not yet run.

 Build19 broad serial/
session28247 PASS59.55s:182 cases/1,121,344 assertions. MPI6/session24105
PASS37.58s:159 cases on every rank. Both preserve all pins. Receipts live under
`staged57/cases/serial_full_20261027T103743_1836407` and
`staged57/cases/mpi6_20261027T103917_1840976` in
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1`.

M6 comparison/session40354 TERMINAL PASS613.220s with unchanged pins and complete
six-rank shards. Every entry owned/donor/parameter shard is byte-identical to
build18. At the same four-window stop, cells682,193 versus680,869, q debt6559.099
versus6481.093, J11290.032 versus11193.364, nearly identical size debt892329.681.
Nested490 moves include seven replaced physical entry points. Relaxation executes,
but the short difficult-case result is worse. Do not tune another local ordering
variant or repeat a blind full timeout; address connected cross-chart geometry.
This remains private diagnostic state, not M6 acceptance or an infeasibility proof.

Latest pushed `17902da07b`: staged existing-point relaxation, topology improvement,
then enrichment, sharing existing bounded work; nested operation counts accrue
only to published repairs. Build19/session53468 PASS729.749s,933 unchanged pins.
Focused quality_checks/session37382 PASS9.470s,10 cases/4539 assertions. Initial
sandbox launch89913 stopped before tests at MPI initialization; receipt retained.
Build19 serial/MPI integration, transfer and restart pass. Full M6 acceptance
remains unqualified.

Pushed `64953f3b69` preserves build18 diagnostic receipts. Its six-rank capture/
session11197 completed in623.327s with unchanged pins/donors and all complete shards.
Over248 quality rounds, cells grew664,744→680,869, q debt rose0.76%, J debt fell0.30%,
size debt fell1.28%. All947 commits came from nested optimization, hidden by the
old reconnected counter. Shards prove3,215 added interior points, no removed/moved
entry points and unchanged physical bindings. This supports correcting insertion
before smoothing; it does not establish a new aggregate gate or M6 infeasibility.
Current comparison stops after four quality windows, preserves private snapshots,
and is neither exact scheduler continuation nor accepted adaptation.
[Design, receipts and interpretation](Handbook/M6_QUALITY_STAGE_PROGRESS.md).

Build18 observer/capture focused serial and MPI6 checks pass. Build17 broad serial
passes179 cases/1,121,211 assertions and MPI6 passes156 cases/rank, with unchanged
pins. That integration covers coarsening rollback, complete coordinate stars,
clipping/collision fixes, curved Euler/RANS transfer/reload, history and restart.
It does not validate the changed build19 workflow. Detailed evidence and earlier
failed fixtures/launches: [validation history](Handbook/VALIDATION_HISTORY_THROUGH_BUILD17.md)
and [transfer ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md).

Use existing `staged57/build_checkpoint.py` and `run_cases.py`; retain unique source,
binary, runtime/input/helper and resource receipts. The difficult-case
comparison is complete; address the missing geometry capability and do not
repeat blind timeouts or accumulate local fallbacks. Run broader integration at
the connected milestone, preserving every final acceptance gate.

## Difficult target evidence and remaining capability

Full M6 build13/session70428 is TERMINAL REJECT:1863.87s wall,1802.45s native;
solver0/nativefalse, all pins unchanged. It reached repair, never coarsening:
774,314 private cells,275,115 cached invalid cells,1,240,202 repair demands after
112,024 private publications. Work1,218,977/19,414,976; time exhausted. Three cells
were unmeasured. Original CFD state retained. Sampled process-tree RSS maximum
4.616GB, individual0.877GB; neither establishes a whole-job peak.
`staged57/m6_runs/euler_build13/diagnosis.json` preserves six-rank evidence.

Coarsening alone cannot close M6. The current staging change addresses evidenced
enrichment ordering; original-chart boundary mobility is separately missing.
Cross-chart movement requires shared feature/coverage/edge-trace semantics through
publication, source reconstruction and restart, not removal of one guard.
[Geometry capability and affected callers](Handbook/CURVED_BOUNDARY_MOBILITY.md).

Other open scope: changing-frame gradation21/9,393 violations (worst1.406), normal-only
M6 budget profiles above500k (best821,265+/-53,688), whole-field normalization,
curved BL support/chord/quadrature, FIRST_ROW surface-error demands, fitted seams,
automatic curved initialization, and complete solution-transfer/restart qualification.
Normal-only contract still binds; tangential growth outside protected band unanswered.

## Checkpoints and preserved history

`f27662eb31` preserves build17 integration; `c659e03cf5` fixes the large-star
fixture/collision paging; `6aee162837` contains coarsening closure and clipping;
`536025f6e4` contains construction generations, fair repair and complete-star moves.
Earlier builds and local
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
