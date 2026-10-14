## Earlier validated checkpoints and evidence

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`; AdapNoExt remains untouched. Active branch: `codex/native-unsteady-performance`, fast-forwarded from `codex/native-post-rebase-metric-candidate`. Implementation checkpoint: `e8f0a237362f7fb060eb1272f2d174c5081b944c`; a subsequent handoff-only commit does not change the built source or suite pins. The final publication SHA is reported after remote verification.

**User preference: no further local MPI/CFD correctness or performance runs. Run correctness on the cluster.** The local correctness controller was stopped explicitly at the user request, during its first serial metric suite. No new prediction CFD case ran locally; the subsequent user-run cluster gate 581856 now passes completely. Preserve the stop receipt/logs. Only lightweight source, syntax, manifest and Git checks remain local.

The metric merge checkpoint `5cb87667c20f7c590fab00aa373c7c056a2daca1` has its completed 41-stage validation. The new configurable PREDICT history/filter, acceleration transport and native PREDICT support build successfully (302-step ABI rebuild), and the complete 20-stage native 2D PREDICT correctness gate passes on the cluster in job 581856, independently re-audited from saved output. Native FIXED_POINT remains unsupported. Noise remains zero, and sensor-only donor interpolation plus geometric BL queries, finer demands and fade are preserved. No communicator fix was needed for the earlier MPI timeout; four ranks had been confined to two CPUs.

Reusable user workflow after rebuilding both binaries: run `python3 integration_evidence/native_cluster_metric_comparison_v1/prepare_correctness.py`, then submit `qsub -pe mpi 4 integration_evidence/native_cluster_metric_comparison_v1/RunPredictCorrectnessSGE.sh`. The preparation captures the current source/test/input and binary hashes independently of frozen performance pins; intentional code changes are admitted only through explicit preparation after rebuilding. Each job saves its checkpoint with its results. It runs MPI1/2/4 and native Euler/RANS/CGNS/partial-restart checks sequentially with closed source-pinned inputs. Compute nodes need no Git. Download `ClusterResults/predict_correctness_JOB_ID/` and `ClusterResults/jobs/JOB_ID_predict_launcher/`. Require the validation JSON PASS, then review results before launching the separate serialized old/new performance comparison. See `integration_evidence/native_cluster_metric_comparison_v1/CORRECTNESS_README.md` and its README. Timings are still approximate on shared nodes; no exclusive allocation is requested.

Cluster job 581847 reached the first serial metric suite: 160/161 tests and 420254/420255 assertions passed. The sole failure was the new reference-connectivity assertion: MakeSimplexMesh deliberately supplies clockwise triangles, SU2 reverses them during geometry construction, and the independent reference was not reversed. The test fixture now applies the same geometric orientation correction before cell ordering; exact connectivity and predicted-field checks remain. A lightweight replay of the saved vectors proved all 576 triangles match after correcting 288 clockwise reference triangles. No local solver was run. The subsequent job 581852 passes the MPI1/2/4 rerun; see the newer result below. Evidence is preserved in `integration_evidence/native_post_rebase_metric_v1/cluster_correctness_581847_*`.

Cluster job 581852: all six metric/native MPI1/2/4 suites and all five negative guards PASS. Default-two Euler N4/M3 and four-frame fixture Euler N1/M1 pass independent mesh/original-P1/reference, positivity and both-history conservation audits. Four-frame Euler N1/M1 performs both remeshes successfully, then terminates at step 11 because MAX_TIME was omitted (default 1s) despite TIME_ITER=15 and dt=0.1s. The snapshot assertion fails correctly; original aggregate FAIL is preserved. A separately scoped offline audit of the two completed remeshes passes with qmin~0.583, Lmax<1.8 and history defects<2.77e-15. All logged Euler sensor/composed nodal transport residual counts are zero; this is not a BL gradation certificate. Evidence and report: `integration_evidence/native_post_rebase_metric_v1/cluster_correctness_581852/`; raw grids remain in `ClusterResults/predict_correctness_581852/`.

Correction: all seven reusable configs now set MAX_TIME=100.0 so TIME_ITER governs completion, and cadence assertion errors include expected/actual steps and stop-limit hints. No production code or frozen performance pins changed; source/suite checkpoint must be prepared again after pulling. Remaining full four-frame N1/M1, N2/M1, CGNS N4/M3, straight-wall SA RANS and partial-restart cases need the user-run SGE rerun. No local solver or benchmark was run; saved-output audits only. The matched old/new performance comparison is still pending aggregate correctness PASS.

Cluster job 581856 completes all 20 sequential correctness stages: six MPI1/2/4 metric/native suites, five negative guards, six native default-two/four-frame Euler/CGNS/straight-wall SA RANS cases and three actual adapted-mesh partial restarts. All source/suite identities and stage/checkpoint hashes match. Saved-data audits reproduced all six full cases and all three partial states/fallbacks, plus topology/marker/free-boundary geometry checks on 11 replacements. Across replacements qmin=0.5143406, Lmax=1.7950971, maximum both-history conservation defect=3.9791e-15; RANS relative first-height error<=4.4409e-16. No local MPI/CFD run occurred. Current correctness gate is complete; reusable infrastructure remains available for future changes.

Important remaining limit: all sensor nodal gradation residuals are zero, but the RANS composed sensor-plus-BL diagnostic after prediction reaches max ratio 2.73259 and 76 above-tolerance directed edges (terminal window); first two predictions report 1.28988/2 and 1.94235/54. Combined-field gradation is not certified despite passing mesh quality/length/height/transfer. Do not hide this in PASS or reintroduce coarse-cell wall-tensor spreading/hard-normal reset. Euler composed diagnostics have no residual edges. Native 3D, FIXED_POINT and converged aerodynamic robustness remain outside this gate.

Evidence/report: `integration_evidence/native_post_rebase_metric_v1/cluster_correctness_581856/`. Original inspectable grids/flow/state: `ClusterResults/predict_correctness_581856/`; CGNS grids under `four_cgns_n4m3/`, RANS grids under `plate_sa_four_n4m3/`, both at mesh steps 5 and 10. The performance candidate's 762 source pins and input hashes still match. Next: user-run serialized baseline/candidate N4 pilot, two adaptation events, modes pNO/M4 and pYES/M4/M3/M2, using the existing SubmitMetricComparison.sh; then repeats/longer runs after review. No speedup claim or local performance launch. See comparison README. Chronological pending/failure notes above are superseded by this completed gate.

Cluster submission compatibility fix: the first SubmitMetricComparison.sh call failed with `hold[@]: unbound variable`, because older Bash treats an empty optional array as unset under `set -u`. Required qsub flags now initialize a nonempty qsub_args array; optional dependencies append to it. Strict error handling and all source/input preflight checks remain. Extended fake-qsub regression PASS: 24 submissions without initial HOLD_JID and eight with an explicit initial hold, dependency chaining/order/receipts and invalid-source/path guards. bash -n PASS. No actual scheduler/CFD jobs were launched; local Bash is 5.0.17 and the legacy cluster interpreter was not available. Receipt: `integration_evidence/native_cluster_metric_comparison_v1/submission_bash_compatibility_validation.json`; previous receipt retained. Pull and repeat the same performance pilot command; no binary rebuild or correctness rerun is needed. The specific first-job array expansion error submitted no jobs.

Paired performance pilot 581886–581893 reviewed: baseline download `/media/rausa/4TB/SU2_Versions/SU2_Native_Previous/ClusterResults` has all 16 execution/collection cases PASS; candidate has only four N4/M4/pNO cases, each solver/test PASS then exporter abort on mismatched older ClusterResults/tools. Remaining candidate weighted M4/M3/M2 modes never ran. All 350 selected-data hashes verify; all 20 profile accounting records close. Independent saved-data audits PASS for all eight matched no-repartition cases (version-matching tools, temporary copies; no local CFD/MPI). Inputs and selected build/compiler/MPI/node fields match; conservative flow before first adaptation is bitwise identical in actual RANS/Euler. Candidate actual adaptation lifecycle 57.9356 vs baseline77.3303s RANS (-25.08%), 15.1835 vs17.7928s Euler (-14.67%). Metric ~63% lower; actual remesh +9.2%/+1.0%; outputs/trajectories and external contention differ, single repetition, no verified speedup. Frozen BL-to-Euler grid/commits identical, timing within0.1%; frozen Euler-to-BL grid/commits differ.

Remaining MPI imbalance: first candidate RANS private reconstruction max/mean=2.222 (17.338/7.804s). Validation rank-mean18.747s includes MPI waiting; do not interpret it as pure checks or sum rank maxima. Candidate RANS sensor residuals zero but composed nodal diagnostic reaches ratio318.696/up to15344 edges; combined-field gradation remains uncertified even though mesh/height/transport contracts pass. RANS grids: `ClusterResults/cases/581891_actual_euler_to_bl_n4_m4_pNO_r1/mesh_00200.su2` and `mesh_00400.su2`. Report/evidence: `integration_evidence/native_cluster_metric_comparison_v1/pilot_581886_581893/`.

Collection fix: export matching audit sources into CASE/tools, record their hashes, and write collection manifest after successful tool copy. Older shared bundles and exports remain unchanged. New fake-file regression PASS for version isolation, duplicate/missing data, and no premature COMPLETE manifest when an audit tool is missing. Baseline CPP/collector stays at e6995fbff5; current 762 production source pins unchanged. User should pull candidate and rerun the same serialized eight-job N4/M4/M3/M2 pilot; no rebuild or new CFD correctness run needed. Actual baseline cluster root is `/global-scratch/bulk_pool/arausa/SU2_Native_Previous`. Download ClusterResults only from each checkout and retain its tool layout. No local solver or cluster jobs were launched by Codex.

Completed paired pilot 581943–581950: all eight jobs / 32 execution-and-collection cases PASS. Verified 560 selected files, 160 candidate case-local tools, all 32 profile closures, and all 20 audit tools against baseline e6995fbff5 / previous candidate publication 93e7403366. Inputs/configs match across all 16 pairs; initial conservative flow matches bitwise across all eight actual pairs. Eight no-repartition numerical proofs reuse earlier independent PASS after exact relevant-input/source checks; remaining 24 modes await independent cluster postprocessing. No local CFD/MPI/build/heavy audit ran. Report and complete phase/hash evidence: `integration_evidence/native_cluster_metric_comparison_v1/pilot_581943_581950/`.

Candidate actual RANS remesh seconds M4/NO43.902, M4/YES35.125, M3/YES42.698, M2/YES46.788; all-adaptation58.048/57.337/56.786/60.734s. M4/YES adapted output7.152s offsets its remesh improvement; keep output and CFD repartition/transfer in lifecycle accounting. Euler remesh13.084/12.627/13.861/16.968s. Matched metric time decreases62–72%, but shared-node loads differ greatly (baseline RANS144 other processes on ag1 versus candidate48 on ag2), output/work differs across versions, and only one full matrix is available. Candidate Euler M4/NO CFD95.843s versus prior55.924s despite identical saved numerical audit inputs and binary. Frozen BL-to-Euler outputs are byte-identical for all four matched modes; no consistent kernel gain. Do not claim verified scaling/speedup or an optimum M.

MPI imbalance persists: first actual RANS private max/mean2.208 at M4/NO and2.440 at M4/YES, despite faster weighted remeshing. Sensor gradation residuals are zero in all candidate actual modes; composed RANS diagnostic still reaches ratio324.523 and15950 directed residual edges. Remaining combined-field gradation, weighted-mode numerical review, converged aerodynamics and native3D are not certified. Grids now under `ClusterResults/cases/581948_actual_euler_to_bl_n4_m4_pNO_r1/mesh_00200.su2` / `mesh_00400.su2` and weighted siblings; Euler581950 mesh steps100/200.

Reusable saved-result audit helper: `audit_metric_comparison.py` with `RunSavedMetricAuditSGE.sh` (one worker, sequential Python, version-matching tools, temporary copies, no solver/Git on compute nodes). Submit from candidate checkout: `qsub integration_evidence/native_cluster_metric_comparison_v1/RunSavedMetricAuditSGE.sh /global-scratch/bulk_pool/arausa/SU2_Native_Previous/ClusterResults integration_evidence/native_cluster_metric_comparison_v1/pilot_581943_581950/assessment.json`. Download `ClusterResults/saved_metric_audit_JOB_ID/` and matching `_saved_audit_launcher`; require32 numerical PASS. Fake-file dispatch/isolation/hash/rejection/failure-retention checks and shell syntax pass; actual audit job remains user-owned and unrun. No rebuild needed.

Discussion recommendation: improve measured initial balance before implementing live repartition. Instrument local reconstruction by rank/operation/spatial region and distinguish many movable expensive cavities from a single indivisible one; previous adaptation costs may inform the next initial partition. Mid-remesh repartition is feasible at a completed collective round/phase checkpoint, but elapsed work is known and remaining work only predicted. Prototype fixed active rank count, immutable original donor/geometry target, staged current-cell/directory ownership migration, retained identities/allocation counters and invalidated selection caches. Existing Repartition uses the CFD communicator/default failure/reductions and needs refactoring on M<N; Engine constructor snapshots donors from input, while live Cell.nodal_target already contains composed tensors. Neither recreating Engine from live cells nor calling original WorkWeights on them preserves the intended raw sensor field. No production dynamic-repartition changes or new goal activation were made.

Independent saved-output cluster audit581995 now reviewed PASS for all32 baseline/candidate cases (16frozen +16actual), all48 replacement meshes and64 BDF histories. Launcherexit0/emptyerror, driver/wrapper and assessment SHA match ecd2b0626f publication. Every separate audit report equals its aggregate entry; fresh lightweight review verifies560 data hashes,32 manifests and320 per-case audit-tool hash checks. All numerical results finite. No local solver/MPI/rebuild or numerical mesh-audit rerun occurred. ClusterRaw is not needed; original exports remain unchanged.

Candidate extrema across allmodes: qmin0.1821233856, Lmax1.7999999618, relative first-height error<=4.7083448e-12, frozen transported directional tensor defect<=7.0457733e-11 (tolerance1e-7), CLOSED-policy history conservation residual<=1.2143580e-12 (tolerance1e-10), positive flow states/nonnegative transferred SA. Geometry/reference/topology checks pass. CLOSED conservation includes independent changed-farfield-domain correction; actual tensor transport is not directly exported, while frozen tensors are checked. All24 previously pending weighted-mode numerical audits are now complete; earlier pending notes are historical.

Preserved full aggregate, review hashes/extrema and launcher evidence: `integration_evidence/native_cluster_metric_comparison_v1/cluster_audit_581995/`. Original reports: `ClusterResults/saved_metric_audit_581995/`. Existing simulation grids remain under jobs581948(RANS) /581950(Euler), with allfour partition modes. This audit reads saved output and establishes no additional timing/scaling gain. Composed BL gradation, converged aerodynamics/native3D remain unresolved. Next recommended engineering step is measured local reconstruction cost by rank/operation/spatial region and improved initial weights, potentially learned from preceding adaptation events; defer mid-remesh migration unless persistent movable hotspots justify it. No production code change or new goal activation was made.

Compact reconstruction-balance pilot prepared (2026-10-09): user authorized the
next cluster package and requests minimal saved data. Production now has opt-in
`SU2_NATIVE_BALANCE_PROFILE=YES`, with eight operation totals and at most32
expensive private attempts per worker (including failed reconstruction), wall/
process-CPU cost, cavity size/location and target query counters. It never changes
selection, target or publication decisions. NO/unset does not allocate profile
storage; candidate flag-consistency elections remain and count toward overhead.
Profile output is collectively checked, refuses existing readable filenames, and
is separately timed outside engine-adapt but inside full remesh cost. New unit
checks cover bounded rejected-attempt records and unchanged coupled MPI HEIGHT
reconstruction. C++ compilation/runtime remain pending the user-run cluster job.

Package: `integration_evidence/native_cluster_balance_profile_v1/README.md`.
Use existing candidate cluster checkout on `codex/native-unsteady-performance`:
pull, run `prepare.py --preserve-control` BEFORE rebuilding the test driver,
`./ninja -C build-native -j2 UnitTests/test_driver`, run `prepare.py`, then submit
`qsub integration_evidence/native_cluster_balance_profile_v1/RunBalanceProfileSGE.sh`.
Preserved control hash must be validated b8bb7bc0bf3ba701232513d0c4d0e812e3483cac605da6ecd13e1a4845b7971e;
copy stays inside build-native/balance-control. Do not replace old binary first.
Source pins for the reviewed control remain in this package. No CFD rebuild or
local solver run was performed; source/binary preparation is not a build proof.

One shared-node SGE four-slot allocation runs focused MPI1/2/4 units then16
sequential frozen Euler-to-BL/BL-to-Euler control/profile cases at N4/M4 NO and
N4/M4/M3/M2 YES. Mesh/geometry/height/tensor and timing-accounting audits must PASS;
paired adapted mesh +four transported tensor files must be byte-identical or the
campaign stops. Profiling min/mean/max private costs and committed operation
counts must close against existing logs. This measures private operation imbalance
and instrumentation overhead; it is not live repartition, general scaling, or a
composed-BL-gradation certificate. No per-query clocks are added. Existing actual
campaign still supplies CFD repartition/transfer/output lifecycle evidence.

Download `ClusterResults/balance_profile_JOB_ID/` plus
`ClusterResults/jobs/JOB_ID_balance_profile_launcher/`. Cases under cases/ hold
inspectable grids/tensors and complete frozen audit inputs. Tools stored once per
job; repeated verified content hardlinked within the job (use rsync -aH to retain
savings). No CFD timestep histories or ClusterRaw produced. Task-owned temporary
outputs removed only after verified compact export; failed copying/hash checks
retain the actual working directory. Successful unit-stage incidental geometry
is pruned after receipt/log verification; failures preserved. Previous evidence
untouched. Every working/retained testcase folder is printed by the controller.

Local fake-file package checks PASS: control preservation/checkpoint/path guards,
CSV/pair checks, deduplication/verified cleanup, all16-case orchestration, failed
unit gate and changed-pair early stop. Python AST/bash syntax and diff checks PASS.
No SU2/MPI/qsub/build was executed locally. New C++ tests and real cluster audits
are deliberately pending. Historical performance source pins remain unchanged;
use the new package rather than the old frozen source-pinned submission helper.

Cluster job582197 compatibility failure: scratch refused os.link with EPERM
while staging first frozen input (run.cfg), BEFORE all MPI/SU2 gates. This is an
export/filesystem issue, not a remeshing failure. Both staging and compact export
now use one link-or-exclusive-copy helper; known unsupported/refused/cross-device
link errors fall back to copying, while EEXIST/I/O/disk-full failures propagate.
SHA checks and verification-before-cleanup are preserved. Manifests record storage
method; validation records copy/hardlink counts. Fallback saves only selected files
but cannot provide inode deduplication savings on that filesystem. No C++ changes,
rebuild or original-control replacement needed. Pull, move old checkpoint with
mv -n to balance_profile_checkpoint_582197.json, rerun prepare.py and qsub the same
RunBalanceProfileSGE.sh. Preserve previous failed folders/launcher. README contains
the exact recovery commands. Original preparation receipt remains historical;
new link_fallback_checks.json records this fix's lightweight checks. Tests include
complete16-case simulated pilots with/without hardlinks, guarded existing output
and copy-failure retention; no local solver/MPI/build/qsub run.

Cluster balance pilot582199 reviewed (2026-10-10): allfocused MPI1/2/4 gates
(39 cases per rank, including both new profile tests), all16 frozen remesh/audit
cases and all8 byte-identical grid/tensor pairs PASS. Fresh review verifies324
selected case files, eight fixture files, ten matching audit tools, four archived
package scripts and810 local published compiled-source/Meson pins; logs/CSV
accounting, committed counters, saved geometry/height/tensor audit reports close.
Control stays validated b8bb7bc0...; candidate binary identity recorded. Runtime
profiling gate is now complete for these supported frozen2D cases. This does not
establish independent build provenance, unsteady CFD/transfer or3D/scaling proof.

Main hotspot is costly successful coordinated BULK_SPLIT near trailing edge,
not initial ParMETIS cost. Euler-to-BL M4NO private rank wall13.749/0.490/4.419/
5.464s (max/mean2.280), largest46-cell proposal2.253s wall/2.244sCPU and1,329,104
metric evaluations at x0.98330..1.00273,y-0.001678..0.003848. Retained11 joint
attempts allcommitted, total7.214s (bounded top32, not complete joint census).
WeightedM4 worsens private max/mean2.546 with19.605s busiest rank and8.626s
retained joint cost. JointPatch/SplitSeed repeated score/metric work and bounded
query cache deserve focused aggregate profiling/reuse; counts are not exclusive
interpolation CPU timings. ProcessCPU/private-wall~0.995. Round validation mean
13.675s includes MPI waiting, not pure validation computation. Working partition
only0.048..0.058s Euler-to-BL /0.101..0.107s BL-to-Euler. Preserve successful
reconstruction freedom and allraw-sensor/geometric-BL/height/geometry guards;
blindly limiting expensive proposals risks renewed TE stalls.

Control remesh times M4NO/M4YES/M3YES/M2YES: Euler-to-BL30.159/33.881/32.889/
41.914s; BL-to-Euler10.426/10.560/12.747/15.659s. BL-to-Euler weightedM4 improves
private imbalance1.510->1.225, showing workload-specific weights; its retained
hotspots are ordinary splits <=~7ms. One shared192-core node, fourrank pilot,
48 visible other compute processes. Changes in output/connectivity across modes
exclude fixed-work strong-scaling claims. Paired profile/control change-3.08%
to+1.28%, file output2.0..2.7ms; no isolated small instrumentation-overhead bound.
Optimize coordinated objective/query reuse before live migration, then remeasure
remaining movable imbalance. No further run needed solely to identify this hot
path; repeats required for quantitative performance claims after code changes.

Copy fallback succeeds332files; downloaded entirejob102,152,559bytes (~97.42MiB),
no staging objects/tempcase dirs, noClusterRaw/timestep solution history. Original
exports/failed582197 untouched. Evidence and reproducible lightweight review:
integration_evidence/native_cluster_balance_profile_v1/cluster_review_582199/.
Full logged coordinated phases: M4NO7.326s/45commits, M4YES8.684s/25,
M3YES2.108s/22, M2YES5.410s/23; those wall phases include allround scopes and
must not be equated with retained top32 private samples. BL-to-Euler logs zero
coordinated phase/commits.
Original grids under ClusterResults/balance_profile_582199/cases/.../
native_frozen_adapted.su2 (Euler-to-BL and BL-to-Euler, allworker modes). No local
solver/MPI/build/qsub or numerical mesh-audit recomputation, no production change
and no tracker resume. Previous pending profile-runtime notes are now historical.

The long-running goal tracker remains paused. Cluster runs are user-owned. Do not resume local solver checks or claim prediction validation/performance success before reviewing the cluster results. Earlier notes below are chronological history and may describe superseded pending work.

---

