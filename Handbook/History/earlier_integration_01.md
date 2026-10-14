# Codex handoff — native integration

Updated 2026-10-08. Previous native integration and RAE repair goals COMPLETE.
Current branch `codex/native-unsteady-performance` (2D unsteady/cost checkpoint validated);
worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.


## Current goal: adaptation partition and execution

User activated and explicitly superseded the previous goal on 2026-10-08.
Authoritative objective: NATIVE_ADAPTATION_PARTITION_GOAL.md.
Start with contention-aware old/current fixed-mesh timing controls; then local donor
indexing, adaptation-cost weighted M=N partitioning and explicit M<=N execution.
Keep accepted CFD state/history on N ranks and transfer directly from that donor.
Tracker PAUSED at the user's explicit request while waiting for cluster runs.
Newly authorized cluster analysis and metric integration proceed as concrete tasks;
the tracker has not been recreated or marked complete. No budget was requested.
Fixed-mesh CFD controls started sequentially under native_unsteady_performance_v2:
cfd_control_euler_from_bl_{old_bare,current_timed,current_bare,old_timed}_v1.
60 BDF2 steps, 30 inner iterations, same 13386-point BL mesh, adaptation and metric
OFF. Source-pinned old/current binaries; 15-second quiet launch gate requires no
CFD/compiler job, >=5 available CPU equivalents on the 8-core host, CPU PSIavg10<=10.
Record /proc CPU busy fractions, PSI/load and owned rank affinity while running.
All four fixed-mesh controls terminalPASS: old bare54.237, current timed48.144,
current bare55.525, old timed49.690 seconds. Final density/momenta/energy values
are EXACTLY equal across all four, matching input/config hashes. CPU busy fractions
rise~0.83-0.98; CPU PSIavg10 reaches31.94 during controls despite launch gating.
No large old/current regression or GNUtime penalty appears; shared-host variation
prevents attributing historical/full-run phase changes or small timing differences.
Evidence: native_unsteady_performance_v2/fixed_mesh_cost_controls_v1.json.

Local donor index implemented using existing CADTElemClass(globalTree=false),
bounding-box LINE records only; exact uninflated box/padded triangle checks and
original cell-ID discovery/canonical patch ordering stay authoritative. Other
numeric builds retain original scan (production Native already requires primal64).
Pre-query IntersectionQueryBound is admitted against the dependency ceiling;
otherwise original allocation-free scan. Candidate arrays enter payload admission.
End-of-remesh candidate/full-scan counters and retained index bytes are reported.
Differential distributed donor-cover check includes translated geometry/reversed IDs.
Build v1 terminalFAIL: missing direct include for existing LINE enum; log/source
preserved. Added option_structure.hpp. Build v2 terminalPASS, archive/hash controller
session8569 terminal0. Appf3a03ef23612ac336c98386cf250626ea6343a82640f677774e714c5ee69811f,
test72116a8b7f5307afd9719f72091a9224557430cff7e4f617ab4e7617b45f6923.
Native donor index core MPI1/2/4 PASSES64cases/rank (session57936 terminal0).
Euler-to-BL index frozen MPI4 pilot terminalPASS53.292s whole test,52.759290s
remeshing;985k candidates vs719m full-scan equivalent,405272 index bytes maxrank.
Independent original-P1/geometric-BL/reference/height/topology audit PASS;
output byte-identical to pre-index pruning-cost-v2 mesh (44811commits unchanged).
Fresh pre-index same-work control terminalPASS58.186s whole test,57.673294kernel;
one pair gives8.52% kernel reduction, provisional pending repeats/host variation.
Private reconstruction max26.733index vs26.695baseline: imbalance persists.
BL-to-Euler index MPI4 pilot terminalPASS16.763s whole test;3.215m candidates
vs1.217b full-scan equivalent,1486072 index bytes maxrank.27814commits unchanged;
mesh byte-identical to pre-index. Independent original-P1/reference/topology audit PASS.
Two fixed-work repeats per binary/workload now COMPLETE (8 cases, allterminal0).
Euler-to-BL baseline kernels57.673294/57.619868, index52.759290/54.940094s:
mean6.5865% reduction. BL-to-Euler baseline40.695734/41.106110, index16.104937/
16.455077s: mean60.1965% reduction. All grids byte-identical per workload; operations,
private target-query counters and input/config/CSV hashes identical. CPU/PSI recorded;
shared-host results, not universal speedups. Summary and runnable checker:
native_adaptation_partition_v1/donor_index_fixed_work_comparison_v1.json and
summarize_donor_index_fixed_work_v1.py. Case folders include input.su2, frozen
sensor CSV, adapted SU2 grid and a labeled copy of the original source flow VTU.
Case-selectable independent auditor: integration_evidence/audit_native_frozen_case.py.
Frozen immutable contention-aware runner: run_native_frozen_contention_control.py.
All campaign sessions terminal0:34554,52092,63085,10923,80880,38500,68887,75915.
Weighted M=N helper and production wiring validated on two frozen workloads; M<N is pending. Implementation:
weighted cell dual graph through existing ParMETIS (N communicator), migrate raw
sensor-bearing cells unchanged BEFORE Engine construction. Work estimates use
separate temporary samples of actual geometric composition: CacheTarget overwrites
nodal_target with composed values, so never snapshot those as original sensor donors.
For M<N add explicit communicator arguments to native passive transport, World and
failure election; keep SU2's CFD communicator N unchanged. Subset failure handling
must return/elect on N safely (current SU2_MPI::Error barrier uses N, so calling it
inside only M participants can deadlock). Existing ReaderSlices::FromDistributed on
all N can redistribute output from first-M nonempty records; keep donor geometry,
solution and all histories on N until direct transfer to the new N-rank geometry.
2026-10-08 working-partition helper checkpoint: CNativePartition2D.hpp/.cpp,
registered in Common/src/adaptation/meson.build. Distributed cell-dual graph built
by edge rendezvous, sparse cell IDs mapped to independent graph ordinals; existing
ParMETIS balances two constraints (bounded static work predictor and cell storage).
Graph-only communicator excludes empty original owners. Single original owner
uses existing serial METIS, no replicated volume mesh in production. Raw full Cell
records migrate before Engine snapshot/cache composition; counts, weights and
unique recipient identities checked before publishing by map swap. Collective
migration admission rejection leaves the entire input unchanged. Graph storage
is O(local cells), outside the migration-specific ceiling; idx_t range checked.
Work predictor uses raw-sensor centroid complexity plus bounded actual-query
quality/length demand and wall-row allowance, never coarse-area integration of
wall tensors. Static predictor is provisional: measured private costs decide
whether it is useful. No reduction in actual RAE imbalance established yet.
Build v1 failed only Meson regeneration (Ninja not on PATH), retained log/source.
Build v2 PASSES after correcting controller environment, -j2, archive762sources;
app hash unchanged f3a03ef... because helper is not yet called by production.
Core MPI1/2/4 PASSES66cases/rank, including sole-owner, empty-rank and interleaved
ownership, exact complete wire-record preservation, balanced predicted work/storage
and one-rank budget rejection. Hidden malformed/nonmanifold graph check PASSES
expected collective diagnostic atMPI2/4, no deadlock. Evidence folders:
native_working_partition_build_v{1,2}, native_working_partition_core_mpi_v1,
native_working_partition_invalid_graph_v1. All owned job sessions terminal.
Next: opt-in ADAP_NATIVE_REPARTITION production config at M=N; export final reader
point tensors outside frozen remeshing timer and independently audit their
original-sensor-plus-actual-query-BL residuals. Frozen RAE comparisons include full
weight estimation/graph/partition/migration overhead; ownership changes work,
so these are whole-workload comparisons, not fixed-work speedups.
Weighted M=N production config draft is now ADAP_NATIVE_REPARTITION= NO by
default (native backend only). YES uses the helper before Engine construction;
weight/dual-graph/ParMETIS/migration timers and rank load tables are printed.
Protected first-row cells omit the full wall-row construction allowance.
Build v3 terminalPASS after full config rebuild (~305 targets, -j2, CPU PSI
recorded); appde0d98e736bebd4e66f1a0d5340e90f64f25878dbfc5ad15e42ad2d8a374a5fc.
Final-source core v2 terminalPASS66cases/rank atMPI1/2/4. New frozen helper exports
native_frozen_target_rank_*.csv after kernel timer/HWM; independent auditor now
supports exact frozen sensor CSV plus every final reader tensor. Relative
DIRECTIONAL tensor error is checked by whitening with the expected metric, so a
large BL eigenvalue cannot conceal a defect in the weak direction. Tolerance1e-7;
small runnable anisotropic weak-direction math check PASS. Actual transported-tensor audit now PASS on both weighted/default frozen workloads;
this does not establish final-source unsteady/restart residuals.
First weighted frozen Euler-to-BL attempt v1 TERMINALFAIL before partition/remesh,
0.3665s: work sampler omitted physical-component associations, so an ulp-rounded
wall-edge midpoint could not use the existing numerical containment envelope.
This is reproduced by the existing FieldPatch roundoff regression. Added the
same association loop as Engine initialization, with a focused WorkWeights
regression; NO containment limits, geometry or metric semantics relaxed.
Build v4 terminalPASS: app71f81287fefdc7dbb56980a5a3ad69f86400d21d4e347ea2f6481985f8b6ebba;
core v3 PASS67cases/rank MPI1/2/4, including rounded-edge work sampler.
Weighted Euler-to-BL v2 and same-source default control PASS remeshing and
independent grid/reference/first-height/final-tensor audits. Whole-process times
39.772/39.869s: no meaningful speedup in this matched pair, despite predicted
work balanced from15270/5303/24620/20617 to16906/15684/16937/16283.
Weighted/private max21.166s vs default22.278s; substantial actual imbalance remains.
Weighted commits49616 vs44811, so not fixed-work acceleration.
Weighted BL-to-Euler v1 and default control PASS; whole-process10.624/11.084s,
a small difference that needs repeats. Same-source summary:
native_adaptation_partition_v1/weighted_same_source_comparison_v1.json.
Transported metric audit found and fixed an independent Python projection bug:
length**2 reconstructed a rounded squared norm and selected the wrong adjacent
edge at an EXACT original wall vertex. Native uses direct dot(delta,delta).
Auditor now matches that projection; unchanged 1e-7 directional tensor tolerance.
Failed checker sources/report and diagnosis retained under Euler-to-BL weighted v2
rejected_tensor_auditor_sources_v1. Corrected audit_v2 PASS, maximum directional
defect7.091e-11. A regression retains the unequal-edge exact-endpoint tie case.
Independent runner supports --config/--ranks and copies labeled original flow VTU.
Subset wrapper/config originated in /tmp/native_subset_wrapper_draft_v1;
the applied/live-build state below supersedes this initial draft checkpoint.
Weighted production checkpoint committed locally:24558223c6. No push performed.
Reduced-rank implementation applied from /tmp/native_subset_transport_draft_v2
and /tmp/native_subset_wrapper_draft_v1, original/draft SHA verified first.
ADAP_NATIVE_RANKS=0 keeps all CFD ranks; M<N automatically weights/migrates raw
working cells to first M ranks. Explicit native passive communicator parameters,
World-local rank/size/election, M engine/diagnostics, sleeping MPI-3 N Ibarrier wait,
N ReaderSlices/Bindings/rejected writer return. Global SU2 CFD communicator remains
N; direct CFD/current/history solution transfer follows the existing driver.
Only collectively elected M failures unwind to a common N election; unexpected
rank-local failures retain fail-fast SU2 termination. MPI-2 fallback can spin.
Added scoped transaction/budget/failure regression and full backend affine
barycentric/conservative replacement tests for default, weighted N, M1/M2.
Rejected output/state-preservation regression now exercises M1/M2; config rejects
workers>N. Build native_subset_execution_build_v1 terminalPASS, app
dfe3b1f81368406829964ae5549a596062191964716e0e62fd8f05921176b529.
Core v1 stopped MPI1: test generator selected M1 twice while SAVE_AUDIT retained
the first rejected candidate; writer correctly refused overwrite. Failed case
retained, no native operator failure or CFD state loss established. Corrected
unique rank modes; build v2 PASS, same CFD app. Core v2 PASS83cases/rank at
MPI1/2/4 (test7e8372eaade722143259aa463e920d6cdc5a1ea856bcb8db36d61048f993001d).
Measured inactive rank maxCPU .02656s during5.18291s wall (tiny-core cases only).
Invalid-worker-count v1 exited .317s without hanging, but MakeConfig muted its
diagnostic; expected-error checker correctly marked FAIL. Replaced that hidden
fixture with unmuted CConfig constructor; build v3 PASS same application hash.
Invalid-worker-count v2 PASS expected M>N diagnostic atMPI4, no deadlock.
Frozen v3 test executable06c8a3ed6bc33cef9f4d08ae76bb86331ce7b077684b9aa3115f056579aa59be
(full762source pins), applicationdfe3b1f81368406829964ae5549a596062191964716e0e62fd8f05921176b529.
First frozen RAE Euler-to-BL N4/M1 launch in
native_frozen_rae_euler_to_bl_workers1_v1/runtime_np4; inspect
run_evidence.json for owned process. TERMINALPASS65.529s whole,63.907541s kernel,
56011commits/11285points; all independent geometry/first-height/P1+BL/final-reader
tensor checks PASS (max directional defect7.091e-11). Idle ranks maxCPU .334714s
for63.832s wall; low CPU use demonstrated, no cost gain versus earlier M4 controls.
N4/M2 Euler-to-BL frozen workers2_v1 terminalPASS45.866s whole,45.372772s kernel,
56392commits, independent full metric/height/reference audit PASS. Same-source
M4 workers4_v1 terminalPASS42.567s whole;49616commits, independent audit PASS.
M4 grid is BYTE-IDENTICAL to pre-subgroup weighted v2 grid. First same-source
Euler-to-BL trio favors M4 (M1/M2/M4 whole65.529/45.866/42.567s), no fixed-work
speedup claim. Comparison: native_adaptation_partition_v1/subset_euler_to_bl_comparison_v1.json.
M3 first frozen trial completed/audited PASS37.816s whole, followed by current-source
exclusive timing and actual N4/M3 vortex/SA lifecycle checks (see latest checkpoint).
Reverse workload, repeats and real unsteady RAE/restart remain pending.
No native/CFD source changes for either fixture correction. Frozen and small-case actual unsteady reduced-rank operation are demonstrated;
real RAE/restart validation and recommended worker-count total-cost improvement
remain pending. Authoritative native
sources stay pinned during build. Original drafts retained. Actual CFD trial
runner now includes CPU busy/PSI/owned-rank affinity before/during runs.
Next: finish frozen same-input M1/M2/N comparisons and independent full-target/height/reference/tensor audits,
actual unsteady/restart/current+history transfer controls and measured complete
costs. Final-source actual unsteady/restart and practical scaling limits remain
pending. No native3D/general affordability claim; goal ACTIVE.
No unrelated heavy job was running at activation; editor activity/CPU pressure
exists and must be measured during controls. One heavy job, MPI<=4, threads1.

## Superseded goal evidence: frozen-target MPI cost and optimization

Superseded as the working objective; its evidence remains available.
Committed checkpoint: aa73b62a773af472815bbc7c8b77187f8e763aee, branch
codex/native-unsteady-performance. Completion index: integration_evidence/
native_unsteady_performance_v1/lifecycle_cost_checkpoint_v3.json (32 actual
completed cases,748 pinned artifacts). Checkpoint document snapshots are beside it.

Next goal is same-frozen-sensor/geometry MPI1/2/4 remeshing scaling, profiling
private interpolation/selection/communication imbalance, then improving the
measured bottleneck with all main metric/geometry/height/rejection gates intact.
Reuse the production backend via opt-in NativeFrozenAirfoil2D in the existing
CNativeAirfoil2D test file; no separate mesher implementation. Fixture CSVs copy
original double donor sensor samples exactly by coordinates. Geometry/BL is
composed by the actual backend. Test-only replicated CSV lookup and artifact
mesh gathering are outside measured remeshing. Different rank counts may still
produce different accepted meshes; do not call these fixed-work efficiencies.

Fixtures: integration_evidence/native_frozen_scaling_v1/{plate_smoke,
rae_euler_to_bl,rae_bl_to_euler}. Small frozen BL smoke passes MPI1/2/4.
Both real frozen campaigns are terminalPASS atMPI4/2/1, timeout300s per job.
Euler-to-BL: integration_evidence/native_frozen_rae_euler_to_bl_v1/runtime_np{1,2,4},
remesh88.957/79.947/51.568s, points11285/11757/12252, commits56011/56028/44811.
BL-to-Euler: native_frozen_rae_bl_to_euler_v1/runtime_np{1,2,4},
remesh62.066/36.112/23.800s, points7795/7778/7964, commits28623/28596/27814.
Independent original-P1/geometric-BL q/length and topology audits PASS all six.
Both MPI4 grids exactly reproduce actual unsteady first meshes (coordinates,
connectivity). Different rank counts perform different work; no fixed-work speedup
claim. Summary: native_frozen_scaling_v1/frozen_scaling_summary_v1.json.

Saved-profile details authoritative v2:8526 native samples with MPI API frames,
8035 PMPI_Allreduce; includes waiting/utilities and truncated stacks. v1 falsely
counted native ReferenceState in ComputeMetric arguments and is retained. New
profile_native_stack_details.py selftest guards path-name/argument-type mistakes.
Preflight consolidation was IMPLEMENTED, passed48corecases/rank atMPI1/2/4
and single-malformed-rank negative controls atMPI1/2/4. Six frozen real trials
produce byte-identical meshes and identical commits/operations/queries/transport
counters to baseline, but show no speed benefit (mostly0.1-2.8%slower). REJECTED
and original World::exchange restored. Evidence and complete trial source retained
in native_frozen_scaling_v1/preflight_rejected_experiment_v1.json; archived app
native_preflight_build_v1, SHA2560ef465018efac316041a7317dc47da05de7d68421fea8f312a219c8f74cbec67.
Linux per-rank peakRSS through remeshing (driver/CSV setup included, artifact
gather excluded) is now recorded by the hidden frozen benchmark. Trial peaks:
RANS~44/53/66MiB at4/2/1, Euler~57/83/139MiB at4/2/1. These are process resident
highwater measurements, not requested-byte admission limits or remesher-only memory.

Standard unordered_map trial passed48corecases/rank atMPI1/2/4 and malformed-peer
control. All six frozen real meshes/counters are byte-identical to baseline.
RANS1/2/4 costs88.003/76.064/48.994s; Euler66.338/36.070/23.626s.
Mixed result preserved in native_frozen_scaling_v1/std_hash_trial_comparison_v1.json.
A repeated immutable serial Euler tree baseline costs65.810s versus62.066s
original, exposing~6% timing variation; no established serial hash regression.
Control: serial_timing_control_v1.json. Standalone real-coordinate warm lookup
probe favors exact-bit hashing but is not a full mesher result.

Exact-bit hash trial passed48corecases/rank atMPI1/2/4 and native gates atall
six frozen workloads, but FAILED strict byte reproduction: RANS276/531/304 lines
and Euler1line per rank differ at roundoff scale. Cause is not established; no
independent-audit reuse or fixed-work speedup claim. REJECTED in favor of the
simpler validated standard hash. Complete source/binaries/grids/logs retained in
native_query_bits_hash_build_v1 and bits_hash_rejected_experiment_v1.json.

Containment pilot passed48corecases/rank atMPI1/2/4. MPI4 frozen pilots
pruning_np4_v1 cost47.946s Euler→BL and21.851s BL→Euler; native gates/operation
counts pass. Coordinates differ at rounding scale, identical to the rejected
exact-bit trial meshes; independent audits and timing repeats are required.
Pilot archive native_query_pruning_build_v1 and pruning_pilot_v1.json retained.

Final candidate BUILT: standard hash plus exact-only donor containment pruning
and whole-private-operation timing. Rank min/mean/max selection, reconstruction,
tracked collectives and largest transaction are reported once per remesh with
three packed end reductions; no per-query clocks. Tracked collective timings
include waiting and exclude validation elections, so are not additive wall cost.
Archive native_query_pruning_cost_build_v2 has app/test and760production pins.
CoreMPI1/2/4 passes63cases per rank including metric composition controls.
Six frozenMPI4/2/1 workloads complete, with independent original-P1/geometric-BL,
height/reference and topology audits PASS. Immutable paired MPI4 controls give
Euler→BL52.134→48.287s (-7.38%), BL→Euler23.882→21.793s (-8.75%).
Serial Euler64.496s sample excluded due possible housekeeping overlap; clean
immutable repeat54.083s. Do not use the excluded sample for timing comparisons.
MPI4 RANS private reconstruction ranges0.618–23.477s per rank, largest private
transaction2.823s; substantial imbalance remains. Costs are not additive phases.
Follow-up controls/audits session99294 terminalPASS; log/tmp/native_final_measurement_followup_v2.log.
Actual CFD session50942 terminalPASS (log/tmp/native_final_cfd_v4.log).
Fresh Euler,SA,SST,CGNS BDF2 cases each finish two adaptations and independent
solution/history/mesh audits PASS. RAE RANS600 and Euler300 also complete two
adaptations and independent frozen-field/geometry/height/state-history audits PASS.
Actual RANS elapsed800.925s: CFD683.475, metric43.7732, remesh72.6062,
replacement0.503051 (transfer0.237451 included), adapted output0.0743186.
Actual Euler elapsed226.030s: CFD160.708, metric5.27348, remesh59.0476,
replacement0.309975 (transfer0.162664 included), adapted output0.0394626.
Both substantially slower than historic checkpoints, including CFD BEFORE adaptation;
cause remains unresolved. RANS adaptation/CFD17.11% is not evidence of software
improvement because CFD became slower; Euler remains40.24%. Controlled kernel
improvements7.38/8.75% remain separately supported. Fixed-mesh controls pending.
Actual cases: integration_evidence/native_unsteady_performance_v2. Fresh SU2 4→2
and CGNS-input restarts are prepared but NOT RUN YET. No code relinks planned.
User resumed on2026-10-08; tracker returned blocked despite current progress.
Work is authorized/resumed and there is no present technical blocker; only user/
system can reset tracker status. Finish required validation before marking complete.
Trial runner now wraps MPI in GNUtime for local-process-tree highwater memory,
including launcher; not aggregate rank memory or remesher admission limits.
One heavy job, ranks<=4, threads1, builds-j2; no relinks while campaigning.
Previous goal superseded by NATIVE_ADAPTATION_PARTITION_GOAL.md; no general affordability or native3D claim.

