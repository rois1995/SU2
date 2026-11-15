# P7/P8/counters — stopped at the first source contradiction

2026-11-15. No production edit, build or numerical run. The requested P7/P8/M18/M20/M21 connected change cannot satisfy P8 as written for the supported worker-subset configuration. Stopped during the source check, under AGENTS.md's instruction to report a contradiction without independently redesigning the proposal. No partial P7 or counter implementation.

## Preceding F3b correction passes

Section2.0b-r is complete: assert timer row root/rank agrees with its summary, copy old-cavity rows with `dict(row)`, run ONLY the corrected helper once on existing outputs. Exit0; complete_milestone_receipt.json passed; all six mesh/reference/restart payloads equal P6c. No build or numerical repeat. [F3b report](ADAPTATION_F3B_20261115.md).

## First contradiction: P8 stage-boundary whole-job memory sum

P8 item2 defines JobPeakRSS3D(comm) as SUM of process peak RSS over the job. Item3 requires it in engine observe_progress at every stage boundary, including ownership_staged. These requirements conflict with worker-subset execution:

| Source | Fact |
|---|---|
| config_template.cfg:2921–2924 | ADAP_NATIVE_RANKS=M<N is explicitly supported; CFD communicator remains unchanged. |
| Common/src/CConfig.cpp:6440–6441 | Validation rejects M>N, allowing M<N. |
| Common/src/adaptation/CNativeRemesher3D.cpp:523 | MPI_Comm_split excludes parent ranks >=M from the worker communicator. |
| Same file:533–534 | Only the M workers enter the engine, using World workers(comm). |
| Same file:667–674 | Nonworker ranks wait in a parent communicator barrier until worker execution returns. |
| Common/src/adaptation/CNativeEngine3D.cpp:1809–1817 | observe_progress runs inside the worker engine and uses its World. |
| Same file:3024 | ownership_staged invokes that worker-only observe_progress. |

JobPeakRSS3D(world.comm) at engine boundaries sums only M workers and omits N−M nonworker peaks. It is not the required whole-job sum. Passing parent.comm there would block the reduction because nonworkers are at the parent barrier, not participating in that reduction. Using the application communicator has the same mismatch. Parent-communicator checks at Remesh start/end are feasible, but do not implement the specified whole-job stage-boundary gate.

The all-six-worker M6 case would conceal this issue; passing it would not qualify the option for the existing subset-worker caller. No M=N restriction, idle-rank estimate, communicator service or other replacement design was introduced. Reviewer correction is needed for this configuration.

## Scope reached and checks not started

P7 source locations were inspected: BoundaryParameterState::GeometryCertificates at CNativeBoundaryParameters3D.cpp:414–449, entity finish at CNativeEngine3D.cpp:1973–1985 and legacy owned-cell GeometryPatch pattern at1995. The existing import caller at CNativeImport3D.cpp:479 must keep its behavior. P7 acceptance was not reached after the connected item's first contradiction.

P8 reaches config declaration/registration/validation/template, EngineOptions/EngineStop, runtime RSS helper, Remesh start/end, engine progress and refusal paths, plus serial/MPI tests. M18 reaches CurvedSearch/cache statistics; M20 private repair work/window reporting; M21 ownership-transition RSS/exchange reporting. None was edited or executed.

Ordered checks (a) units including P7 split-list equivalence and P8 MPI1/2/refusal, (b) exact collapse, (c) one full two-event M6 milestone are all NOT STARTED. No claim about new certificate equivalence, job memory enforcement or counter values. Existing 20 GB/time/physical gates and A5/B5 withdrawals are unchanged.

Evidence: R/p7p8_source_check_20261115/source_check.json, source_hashes.json and SHA-sealed reviewer handoff/design snapshots. R=integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57. Primary source review and the bounded read-only second opinion agree. No owned heavy job was launched.
