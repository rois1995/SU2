# Adapt cut — phase B, 2026-11-17

Phase B passed its prescribed end gates on build558, with source identical to checkpoint `daf75b14e2`. This is the phase-B report, not final N7. The user-ordered stop follows commit/push and terminal-handle verification; phase C was not started.

## Scoped cuts

| Step | Commit | G1 serial / MPI2 / MPI4, s | G2 stop / sealed, s | Source pins |
|---|---|---:|---:|---:|
| B1 | `c8fd7ff400` | 791.819377 / 14.076732 / 17.073050 | 358.315913 / 366.571845 | 1752 |
| B2 | `76b2ad61db` | 793.915226 / 14.066556 / 16.223246 | 368.626787 / 376.929529 | 1765 |
| B3 | `ce25219f04` | 780.160772 / 13.539927 / 14.536724 | 317.640179 / 325.481453 | 1776 |
| B4 | `c0486d57da` | 765.089244 / 13.584711 / 13.561882 | 317.442030 / 325.206035 | 1777 |
| B5 | `2a05da05ca` | 766.075553 / 14.503078 / 14.556237 | 317.596742 / 325.356386 | 1778 |
| B6 | `de723482c7` | 796.648625 / 16.245114 / 32.141896 | 422.579359 / 431.041191 | 1779 |
| B7 | `aa3e78c866` | 799.466036 / 16.047393 / 17.009736 | 358.986603 / 367.124557 | 1780 |
| B8 | `daf75b14e2` | 782.588145 / 13.597257 / 15.629537 | 364.021415 / 372.503794 | 1781 |

Each G1 cohort matched its case/failure table with zero changed cases. G1 is the written identity gate; it preserves the accepted-base failures. The [baseline failure table](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/end_B_preparation/G1_baseline_failures.md) lists all17 failed case/rank records. Exact assertions are retained in its JSON companion and the raw reports. MPI launcher codes and successful-assert totals are observations under C2.1.

**Resource correction:** the original B6/B7/B8 G2 runs above are numerical-counter passes, not R1-compliant runs. An unawaited draft renderer used another core, so seven owned heavy processes ran against the limit of six. The original session ID had been discarded; actual PID4076260/start42326144 and sandbox parent4076259/start42326143 are preserved and terminal. The old preflight classified Python as an outside observation. The corrected preflight accounts for our Python helpers. One strict G2 run on the unchanged integrated build558 passed under the six-process bound. Earlier component evidence is preserved with explicit R1 amendments; it is not relabeled compliant.

## End gates and corrected integrated qualification

| Check | Result | Numbers |
|---|---|---|
| Corrected integrated G2 | PASSED | 316.078817s stop / 323.937712s sealed; work90,809,869; counts480/473/346/263/469/464 |
| G3 | PASSED | 617.688712s; 1170 splits; 1113 common edits exact; adjusted entry work5113 in both |
| G4 1 / 2 / 4 ranks | PASSED | 6.492431s / 6.504021s / 6.514544s |
| GQ | PASSED | 1083.237551s stop / 1091.436227s sealed; third quality window on all six ranks |

G2/GQ preserve all old integer, modeled-storage and per-row timer-call fields; Census calls have zero delta. G3 uses the FIRST N14 G3 reference, all1113 common edits exact and minimum336 splits; the withdrawn P6c first336 identity is absent. Every unchanged physical G3 gate passed. These stage checks do not establish an accepted BL mesh.

Observed prefix rank-HWM sums: corrected G2 6,440,038,400B and GQ 13,999,366,144B. These are prefix observations, not full-workflow P8 peaks. No performance improvement is claimed from code movement or unequal contention.

## Own-helper causes and corrections

- B1: an extra successful-assert-total condition in the wrapper was not C2.1; corrected post-only, preserving raw counts and case/failure identity. No numerical repeat for that helper correction.
- B4: a copied unused reference_build convenience link pointed at a nonexistent old directory. The candidate installer copies only needed reference/helper aliases; setup was corrected before launch.
- B5: draft aliases, nested routing, helper declaration order, enum/type bindings and the initial definition counter were corrected before application. Full causes/corrections and actual-body proofs are in B5_preparation/proposal/PRIMARY_DRAFT_REVIEW_LEDGER.md and its final review receipts.
- B7: draft nested begin_length_generation and external surface_ready calls needed member routing; fixed before production and checked by separate old-name scan. The G2 directory retained a556 suffix but all source/binary pins bind557; the run was preserved and the unused B8 suffix corrected before launch.
- B8: the draft S11 optional worker lacked its successful nullopt return; three S13 stats aliases required result aliases; new call formatting was corrected. The final actual-body/terminal/binding proofs and compilation passed. See integrated candidate CORRECTIONS_LEDGER.md/PRIMARY_REVIEW.md.
- End-B materializer, G3 and G4 setup helpers wrongly required an optional alive field in the sealed primary-controller row. Each now rejects explicit alive state and retains mandatory actual /proc PID/start terminal checks. All no-launch stubs passed; no production/gate/tolerance changed.
- G3 draft stub originally captured Popen without raising its stop exception; fixed before its first stub execution. Its inner receipt initially implied an outer exec outcome; corrected to inner case_exit_code plus a separate actual primary terminal receipt.
- The first B8 renderer stalled in quadratic SequenceMatcher; its wrapper discarded the yielded session ID and a sandbox-local process scan missed it. The successful renderer used a linear inverse, but the first process was not awaited. The primary preserved host identities and terminated only that verified orphan; the active G3 numerical job was never stopped. Resource correction and corrected G2 receipts are in R3_orphan_generator and the B6/B7/B8 R1 amendments.
- The corrected G2 and original GQ monitoring controllers received SIGTERM for unknown reasons while their solver sessions remained live. The same numerical runs were adopted using the unchanged controller loop/postlude and gates. GQ used a tracked detached supervisor that waited for its monitor and recorded actual exit status. No solver restart occurred. Original solver wait statuses after reparenting are unavailable and remain null; no zero exit status is invented. See R3_G2_controller_recovery and R3_GQ_controller_recovery.

## Found during the cut — retained, not fixed

Ignored import_source requests; uncaught construction-deferral throw; receipt-sort ties/input order; callback presence not globally guaranteed by its public API; measured ownership cost and retained geometry can affect partitioning; World counters omit direct Allreduce calls. The whole-item source check remains `P/source_check.md`. Its pre-N14 reference readings are superseded only by the accepted N14 reference adoption and the6.12 10:10 G3 correction.

## Readability and written exceptions

Actual-head input hashes and explicit12/9/31-method bindings were checked. The lexical inventory detects153 private definitions (including defaulted/deleted members) and23 private lambdas, all within80/15 lines. B6 maximum60, B7 maximum68, B8 maximum73. Full formatted new files passed their formatter checks; the code compiled. The inventory is lexical and its parser limitations remain explicit; it is not a compiler proof.

EngineLegacyWall retains the written length exception when moved at C9. Adapt must reach80 lines at phase D, not at the end of B. Existing Selection and MeshOptimizer bodies are outside this cut, not new exceptions. No phase-C or N6 work was started.

| Ten longest detected functions | Formatted lines |
|---|---:|
| Adapt | 4998 |
| MeshOptimizer::SmoothBoundary | 419 |
| MeshOptimizer::Smooth | 320 |
| MeshOptimizer::CoupledBoundary | 232 |
| Selection::Add | 169 |
| MeshOptimizer::BoundaryFlip | 156 |
| MeshOptimizer::EdgeRemoval | 137 |
| OptimizedRepairPatch | 108 |
| MeshOptimizer::ConeReplace | 108 |
| MeshOptimizer::Run | 102 |

## Workflow qualification and stop

The last full two-event qualification remains N14/N15 build552, checkpoint0c2cb5c460: full4738.250709s; native events2568.277632/1672.244847s; exact P8 process-peak sums14,054,744,064/17,397,911,552B; six payloads exact against accepted references; isolated restart236.822017s. Full-run, transfer, restart, geometry/quality/height and budget gates were not relaxed. No additional G5 is prescribed at end B. Current build558 has phase-B scoped/end-gate qualification, not a new full-workflow run.

Commit/push phase B, update progress/HANDOFF/HowToProceed, save actual terminal handles and verify no owned job, then STOP and wait for the user. Phase C/D, N6 and final N7 remain deferred. No8.5ab or BL/RANS capability claim.
