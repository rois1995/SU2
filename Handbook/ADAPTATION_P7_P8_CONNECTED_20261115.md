# P7/P8/M18/M20/M21 — connected implementation, validation pending

Current: **implemented and compiled; stopped at the first executed unit check.** The new P8 fixture aborted (SIGABRT/exit134) while serializing CFD state, before calling Remesh. Its steady setup did not allocate the time-history arrays read by the reused State helper. This fixture error was introduced by the implementation. Memory refusal/accepted-state retention and the other result gates are unverified; no production acceptance failure is claimed. No fixture repair/retry or later numerical check followed. All sampled owned handles, including the MPI daemon, are terminal.

Evidence: R/p7p8_20261115; R=integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57. Reviewer handoff/design snapshots, stage_contract.md, source_check.md, source pins and build receipts are retained there. F3/F3b remains complete with all six payloads equal P6c; no repeat of its corrected helper was needed.

## Whole-item source check: one list

1. P7's old query-count/face-count equality was a diagnostic mismatch (BP377–397, AR2014). Section2.0d-r replaces it: c1 owned proof-face SUM equals state surface size, c2 query totals31622/35291 are diagnostic, c3 exact outputs and all original gates. No additional EntityFaceError final-proof caller outside the certificate face loop was found. Canonical state-face ordering preserves query inputs.
2. M18's existing support_facets, facet_entries and facet_resolutions are not distinct use (AR1129–1140/2106/2223). New event-local facet bits and scope-key sets need persistent storage accounting/reset. Count use at the prepared-facet/scope service; answer-memo hits that bypass that service do not require its prepared pool. Mean bytes is sampled across actual facet builds, including refused retention; report build count with it. Account storage without adding geometry work units or changing pool quotas.
3. M20's rank-local private_work_used differs from the global SUM result (Eng3830/5539/5807). Emit rank-local accumulated window deltas beside busy_seconds, preserving the work budget and reduction.
4. M21 has no current-RSS reader or transition snapshots (runtime19–30, Eng3024). Add Linux statm current bytes and lifetime peaks before/staged/committed, beside known pending/outgoing/record/exchange bytes. Allocator/MPI overhead attribution remains a measured gap, not a claim that modeled buffers explain all RSS.

P7 subset coverage: Repartition restricts destinations to parts and conserves total cells (Partition341,382–390), and all cells carry their physical-face data. Only selected workers enter the engine (Rem533–534). Their owned faces therefore partition the state surface; the proof still elects errors and checks the global count. Imported/cached default certificate callers remain unchanged; component mode retains its existing part2 behavior; nonfinite bounds become infinity.

P8 correction: given-communicator peak SUM plus idle peaks captured before each worker section is a lower bound during subset execution. Start/post-barrier/end parent sums are exact; no parent collective occurs inside the worker engine. Collective memory failure uses the new stop value and clears unpublished result callbacks. Parent memory election is unconditional, including configuration disagreement, to preserve collective ordering. Existing per-rank modeled memory option is unchanged. RSS enforcement scope is the current Unix/Linux build; existing unsupported-platform peak helpers return zero.

## Implementation and acceptance

P7 adds OwnedFaceReverseBounds, reduced-value certificate input and owner proof reduction/counts. P8 adds the MiB option/default0, EngineOptions fields, RSS helpers, stage lower-bound and parent exact checks, memory stop/refusal, and native summary rows. M18 records event-local distinct cache needs/build bytes; M20 records rank work; M21 records ownership buffers/current and peak RSS. No A5/B5, cache quota, query precision, mesh/height/quality rule or budget change.

Ordered validation once: compile; serial units (existing affected geometry/timer cases plus new P7/P8/M18 cases); MPI2 P8/subset case; exact MPI6 first collapse; affected C492/481/current-target/2D1/2/4 safeguards; ONE full two-event workflow. Stop first failed result gate. P7 c2 and timing predictions are diagnostic and do not stop a passing result.

Full workflow gates: native<=5400s/event, peak sum<=19073MiB, exact reported job/process peak identity, lower bounds<=final exact sum, proof-face coverage, all six mesh/ref/restart payloads equal P6c, existing physical/transfer/continued-CFD and95%timer/1%join gates. M18/M20/M21 rows are retained for later measurement/reporting. No final acceptance claim until these checks pass; curved BL/RANS goals remain open.

## Ordered execution and first failure

| Step | Result | Seconds |
|---|---|---:|
| Whole connected source check | Complete; one diagnostic list above, no remaining result-gate contradiction after reviewer corrections | — |
| Initial compile | Failed: generalized World template deduced bool unsupported by MPI | 40.102 |
| Corrected full compile | Passed; original int interface restored, typed P7 reduction added | 910.140 |
| Final static-review incremental compile | Passed; worker-only fixture assertion and proof-vector storage reservation corrected before tests | 80.113 |
| Serial launch inside sandbox | MPI_Init failed on socket EPERM, before any test | 4.693 controller |
| (a) Executed serial units on host | **ABORT: first case,1passed assertion/1fatal assertion; stopped** | 5.471 controller / 1.38 process |
| (a) MPI2 subset / later unit cases | NOT RUN | — |
| (b) First collapse / C492 /481/target/2D safeguards | NOT RUN | — |
| (c) Full two-event workflow | NOT RUN | — |

Compilation required three invocations, rather than a single invocation; all three receipts/pins are preserved, and no numerical test was used to tune either correction. No result check was repeated. The socket-denied launch reached no tests. Source/binary pins remained unchanged during every executed build/test invocation.

Abort location: UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp:2874 calls State(mesh). That helper at86–90 reads current, time_n and time_n1 arrays. The new config at2868–2872 is steady and leaves the history matrices unallocated. C2DContainer.hpp:278 asserts bounds. addr2line maps the stack to State at line92. The first JobPeakRSS inequality assertion passed; the1MiB refusal and retention assertions at2881–2886 were never reached. P7 certificate units, M18 reset units and older geometry/timer units were not reached because this case ran first. No M6 call-site measurements are available.

Final CFD SHA4657124b61fb6e0c0bf731c0086851fe0020cb562067d6d811d04e698db450f5; test SHA57f44f9509bf073f5681efdaa1cb9c341567e575cd6ae0e868cb103923148164;1,544source/build pins unchanged. `first_failure.json`, `unit_execution_receipt.json`, `terminal_handles.json`, the two case directories and compilation receipts preserve the complete evidence. No fresh isolated restart or accepted BL/RANS capability is claimed.
