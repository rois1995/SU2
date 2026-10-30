# Paused M6 handoff — 2026-10-30

The user explicitly authorized a context-reset stop and then paused the goal.
This overrides only this transition's preserve-healthy-run instruction in the
[verbatim next task](ADAPTATION_INTEGRATED_NEXT_TASK.md). Do not implement that task
or launch another workload until the user resumes. No implementation changes were
made during this handoff. Preserve build157's construction improvements.

## Code and verification

- Branch `codex/native-3d-bl`; implementation `f104557fa6`; prior evidence7417458c53.
  Final handoff HEAD is titled `Save paused M6 handoff and integrated cache ownership task`.
- Initial geometry-aware ownership uses separate metric/count/geometry constraints;
  geometry prediction is heuristic, not the exact2017paper formula. CFD ownership
  is distinct. There is no in-event/major-phase migration yet.
- Bidirectional splitting uses finite surface then interior lanes, an immutable
  entry frontier, full fresh reservations and versioned atomic publication.
  Surface claims retire surface neighbors; interior entry releases pass claims.
  Late surface reactivation resumes the surface lane without reseeding descendants.
  Collapse and standalone operators retain their intended policies. Full target
  behavior is still unqualified; do not infer correctness solely from that intent.
- Build157 terminal PASS147.295s,1296pins. Serial fixture set PASS11cases/
  5306assertions,7.955s. MPI6 PASS18cases/rank,8.922s. Build156's single serial
  failure compared Directory snapshots across the intentional pass-active state
  change;157fixed the assertion. No production correction was needed for that failure.
- At handoff, independent final hashes cover1397distinct files: run1396pins,
  build1296pins, serial1386pins and MPI1387pins. All unchanged. No C++/test/helper
  diff exists against the implementation checkpoint. No fresh tests were needed
  or run for the documentation-only handoff.
- Older broad curved COMPLETE workflow assertions remain unqualified; the focused157
  pass is not a full regression pass. The [2026-10-29 checkpoint](ADAPTATION_CHECKPOINT_20261029_STOP.md)
  preserves those historical failures and replay limitations; no old job remains live.

## What was stopped and preserved

All evidence below is under
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/` (R).

Attempt: `R/m6_runs/euler_build157_split_lanes50k` (A).
Driver546640; time546743; timeout546744; mpirun546745; solver ranks546769,
546772,546774,546776,546779,546781. Commands/cwds of all10were verified before
signalling **only mpirun546745** withSIGTERM. All10subsequently disappeared.
The driver terminal status is solver exit1 at4248.932842s, reflecting the deliberate
interruption, not a completed feasibility test or exhausted5400sbudget.

Authoritative compact summary: `R/checkpoint157_handoff_stop.json`.
Stop, ownership, terminal-handle and final-pin receipts:
`A/handoff_stop_20261030/{stop_receipt,terminal_handles,final_pin_receipt}.json`.
That directory preserves pre-stop and terminal logs/resource/history snapshots
and rank stdout/stderr. Original A/run.log, rank_output, resources.jsonl,
stage_assessment.json, native_assessment.json and workflow_assessment.json remain.
Completed-event preservation found **zero** complete events and no adapted_mesh
outputs. There is no accepted adaptation/transfer/restart result to resume.

The source mesh/reference and cycle0 CFD output remain local with provenance:
61330input points,303359input tetrahedra; m6.su2 and its native_ref are symlinks to
pinned original inputs. `restart_flow_adap_00000.dat` is the baseline solve on the
input mesh, not an adapted/private restart. Do not pair it with private cells.
The production run did not export the private721311-cell mesh or full frozen
field; exact in-event continuation is unavailable.

## Latest private construction evidence

Last complete stage: length_split_exit, generation21, adaptation elapsed4079.165879s,
global_work99994. Private cells721311; private point count was not independently
exported/countable. Accepted private edits:91811splits and2602collapses. Unique
oversized surface/interior edges0/0, unique edge debt0, max edge1.7999916802780385.
Size-bad cells0; incidence debt5.665e-14is cancellation residue. Geometry-bad
cells3131; geometry debt0.00041821313719506365. **Quality stages0, q/J unmeasured.**
Zero reported q/J-bad cells therefore does not mean acceptable shape.

Coarsening matched155exactly:292694cells,6078proposals, same geometry/cache work,
but1726.614s vs1180.126s (46.3% slower execution). Host contention differs; this is
not an isolated performance benchmark. Generation1 accepted41131splits,40452in
50.08s interior lane, reducing unique debt11956.7->1706.4. The current coordinated
construction is useful and must be retained. By generation21 metric size closed
privately, but geometry closure still consumed construction and quality never ran.
No claim that this interrupted run would necessarily exhaust its budget follows.
Whether residual geometry debt requires more refinement or tighter conservative
certification remains unresolved; no geometric impossibility has been established.

Scope telemetry at last complete stage, summed across ranks:2412hits,29994misses,
22031retention refusals. Aggregate upper-query time7326.83rank-seconds; nested scope
construction5584.49rank-seconds. Generation12had1055scope misses, all refused
retention, with151.00scope-build rank-seconds inside212.35upper-query rank-seconds;
19145prepared-facet hits/16misses. This distinguishes reusable facet data from
repeated scope preparation. It does not prove plain LRU will improve the target.
A bounded late-coarsening five-second sample shows78.49%inclusive Allreduce,
17.10%surface union and12.92%TriangleUnion construction; percentages overlap.

A/resources.jsonl gives sampled summed process-tree RSS peak21.664GiB at4218.198s.
Per-rank prepared pools are about3.208GB accounting limits, not equivalent to RSS.
Later working mesh growth, allocator retention, immutable authority, transient
query buffers and non-cache state are not separately resolved yet. Actual memory
pressure/leak was not established. Last recorded host headroom was adequate; stop
was explicitly for handoff. Preserve raw profiler data locally; its hash/context
are recorded, but generated profiler binaries are excluded from Git.

Other bounded diagnostics: A/lane_assessment.json, lane_cost_assessment.json,
surface_closure_assessment.json, progress_1450s.txt and profile_1650s*.txt/context.
Some live assessment snapshots predate the final stage; the final stage_assessment
and checkpoint157_handoff_stop.json are authoritative for stopped state.

## Known gaps and next integration risks

- Dedicated neighboring-edge lane fixture sets candidate_batch=1. Existing MPI
  bidirectional tests use MaximumCandidateBatch but their cumulative parallel
  assertion can be satisfied during collapse. Add split-specific concurrency.
  Actual M6 generation1made more than one split per interior round on average;
  that observation does not replace the regression assertion.
- construction_stall_handoffs is incremented but not printed/test-triggered. The
  three-generation near-zero debt rule permits slow positive geometry progress
  without reserving quality time. Do not tune it blindly or weaken final gates.
- Selection::UpdateRepairs in bidirectional quality_only mode omits geometry-only
  invalid cells. Forcing quality alone may leave that debt and immediately return
  to construction, resetting windows/history. Geometry closure and handoff must
  be assessed together; this handoff implements neither change.
- Initial balancing only; no phase-aware ownership change, directory/queue/routes
  rebuild or transition peak accounting exists. The new specification requires a
  real migration-executing focused case, not merely predicted balance statistics.
- Per-host aggregate cache budget/category accounting/replacement is not implemented.
  Cold entries stay after saturation. Exact key identity, active shared references,
  cumulative work and sticky failures must survive bounded replacement.
- Census time is not separately measured; histogram has3bins; child ratio is a
  sparse maximum over available child certificates, excludes spokes. Latest child
  ratio count450against91811splits; it is not a comprehensive placement measure.
-213late interior boundary discoveries by generation21. Their count alone does
  not justify deferral: generation2interior union time rose only0.48aggregate
  seconds despite103discoveries. Preserve complete expansion/height checks.
- Ordinary production M6 observers do not serialize full private state/donors.
  Existing test NativeStageCapture in native_transaction_capture.inc streams
  owned/donor/parameter shards and rejected_exit; it is not exact continuation
  (queues/composition/identity not serialized). TransactionView can add a selected
  cavity. Reuse those hooks if target failure diagnosis requires them; never
  reconstruct the target from an unaccepted mesh to claim exact replay.

## Reproduction after explicit resumption

From repository root; R is the path defined above. Commands below are instructions,
not jobs launched during this handoff. Check host resources/owned handles first.
Never overwrite an existing build/attempt receipt. Next unused build is158.

```bash
python3 integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build_checkpoint.py 158
python3 integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/run_m6.py build158 euler_build158_cache_phase50k 2
```

To reproduce unchanged157without rebuilding, substitute build157and a fresh attempt
name in the second command, only after verifying its manifests. Do not launch a
full old-code campaign solely for benchmarking; reuse the saved155/157evidence.

Build helper invokes nice19 Ninja-j3 for UnitTests/test_driver and SU2_CFD, using
`integration_evidence/build-integrated-v2/compile-tmp` on4TB as TMPDIR, with
OMP_NUM_THREADS=OPENBLAS_NUM_THREADS=MKL_NUM_THREADS=1. Recorded exact build argv/env
are R/build157_command.json. Build157source pins/status/completion remain beside it.

Exact focused-test argv/env,600stimeout and provenance are in:
- R/cases/split_lanes157_serial_20261030T140736_535078/command.json.
- R/cases/split_lanes157_mpi6_20261030T140832_538548/command.json.

Serial filters: `[NativeSplitLanes3D],[NativeGenerationProgress3D],
[NativeConstructionPassClaims3D],[NativeConstructionSweep3D],
[NativeBidirectionalLength3D],[NativeStageProgress3D]` (join without whitespace).
MPI6 adds `[NativeConcurrentComposed3D],[NativeConstructionHandoff3D],
[NativeMappedPublication3D]`. Use existing run_cases.CASES registration and main()
for new named checks to retain pins/resources instead of ad-hoc unrecorded runs.

Exact sequential reproduction of the focused filters after building (use a new
build identifier if158is already occupied):

```bash
for mode in serial mpi6; do
  python3 - build158 "$mode" <<'REPRO'
import sys
from pathlib import Path
build, mode = sys.argv[1:]
root = Path("integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57").resolve()
sys.path.insert(0, str(root))
import run_cases
base = ("[NativeSplitLanes3D],[NativeGenerationProgress3D],"
        "[NativeConstructionPassClaims3D],[NativeConstructionSweep3D],"
        "[NativeBidirectionalLength3D],[NativeStageProgress3D]")
extra = ",[NativeConcurrentComposed3D],[NativeConstructionHandoff3D],[NativeMappedPublication3D]"
name = "split_lanes_handoff_" + mode
run_cases.CASES[name] = (1 if mode == "serial" else 6, base + (extra if mode == "mpi6" else ""))
sys.argv = [str(root / "run_cases.py"), build, name]
run_cases.main()
REPRO
done
```

M6 helper pins config/runtime/input manifests, uses6MPI ranks, nice19, libraries1,
5400snative budget/event,14400souter timeout,12288MiB native working-storage
ceiling/rank, maximum-work0(auto-derived bounded work),2requested events. It uses
one CFD iteration per cycle withCFL1.0: functional continuation, not converged CFD.
Config A/run.cfg is byte-identical to155's50k config. Original input sources are
oneram6_coarse_v1/Coarse.su2 and validation22/m6_authority.json.native_ref under the
same evidence tree. Do not change targets/tolerances/budgets to claim improvement.

Euler complexity50000; later RANS500000. Strict final geometry1e-4, q>=.2, J>=.05,
maximum metric edge1.8, requested first-height constraints and body/feature/topology
checks remain. q/J measurement belongs to improvement, not validity construction.
Require both accepted events, conservative admissible transfer, published meshes
and native_ref, actual CFD continuation and isolated disk restart. The helper's
workflow result is supporting evidence; independently inspect artifacts/state.

## Integrated next task and BL compatibility

Follow the complete linked user specification: coordinated per-host bounded
geometry-cache allowance/retention plus phase-specific ownership estimates and
quiescent transitions at coarsening->splitting and construction->quality. Include
migration, directory/selection/routes/frozen-field reconstruction, cache warming
and overlap memory in benefit/admission. Keep frozen definition/identity, IDs,
versions, global allocation epochs and cumulative budgets; do not restart the
engine or redo conservative CFD projection merely for ownership migration.
Preserve useful cache entries without migrating whole caches or adding phase/rank
to mathematical query keys. No repartition at each lane/generation.

BL compatibility applies now: preserve actual wall constraints/geometry/clearance,
complete expanded cavities, metric attributes, first-layer dependencies and
mandatory height checks. No Euler-only assumptions, fixed walls, permanent column
requirements or solver-label reconstruction modes. Reuse affected existing
NativeFirstLayer3D/NativeCurvedBL3D/NativeConcurrentComposed3D and joint-wall checks
for shared interfaces; these do not establish full BL capability. After accepted
Euler, qualify curved walls, junctions, MPI interfaces, subsequent operations and
redistribution, then implement measured BL work estimates/policies before RANS.
The saved RANS WING3D request is h0=2e-6,growth1.2,thickness.01,SA; inspect its
config/provenance anew after Euler. No accepted actual M6 BL/RANS loop exists.

One owned heavy job,2–4build workers, bounded six-rank local campaigns, libraries1,
resource receipts and4TBtemporary storage remain mandatory. Push reviewable
checkpoints to the user-authorized rois1995/SU2 branch codex/native-3d-bl. Canonical terminal text logs, progress, resource/pin/process receipts and compact
assessments are checkpointed. Redundant snapshot copies and raw binary profiler
data remain local with hashes in the evidence manifest; no generated binaries
are committed. Existing unrelated campaign/old build log modifications were
excluded from this checkpoint.
