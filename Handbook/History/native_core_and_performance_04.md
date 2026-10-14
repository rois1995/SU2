## Complete-star retry notifications and exact collapse filtering — 2026-10-11

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; source changes based on b49f089b43.
Goal1 remains ACTIVE. Goal2 is not authorized to start before all Goal1 gates pass.

Implemented bounded authoritative changed-vertex notifications to old/new star
owners, a local owned-cell vertex index and directed blocked-collapse index, and
exact zero/negative surviving-cell orientation filtering. After successful atomic
publication, affected blocked directions are reactivated even when their edge
cells did not change. Rejection preserves owned/directory state and notifications.
The filter is disabled for invalid fields so private repair still runs. Missing
local witnesses still require full distributed dependencies/private validation.
No acceptance/metric/geometry/round-budget changes; no 2D/MMG/config changes.

Actual complete68-object/774-member MPI/MMG/CGNS build PASS in
coarsen_changes_build_v1. Binary SHA256:
ed708d6b70e89e36e6403974009c36b0d9b00ec5456db0a52a314cb5cbc74907.
coarsen_changes_mpi1_v1/mpi2_v1/mpi4_v1 PASS71cases per rank, with independent
12/24/48 mesh frames,6/12/24 current+BDF pairs and actual SU2/CGNS round trips.
Serial MMG2D/3D PASS48assertions/two cases. Complete remote-star, unchanged-edge,
rank-veto/budget rejection/mode-mismatch and exact-zero-volume tests are included.
coarsen_changes_lifecycle_mpi1_v1 PASS six actual OFF/ON metric/Hessian/CFD
replacement controls, with original-P1/geometry/BDF/output/exclusive accounting.
coarsen_changes_n4_n4_m2_v1 and both coarsen_changes_restart_* folders PASS three
adaptation events with independent exact geometry, metric, state/output audits.
Restarts are SU2N1M1 and CGNSN4M2. Contended one-CPU correctness, no speedup claim.

Frozen n8 coarsen_changes_n8_n1_m1_v1 remains FAIL, now at cycle1. All five original
parameter/environment/config/mesh/sensor snapshots match the previous rejection.
Cycle0 now completes: refine457accepted; coarsen428attempts/272accepted/156rejected,
295exact orientation exclusions,120reactivations;2489cells/590points, no pending
edges or cached invalid cells. Coordinate connectivity and marker comparison
confirms its accepted actual SU2 grid equals the old rejected private grid,
ignoring numbering. Independent cycle0 geometry/original-P1 q/J/integrated-edge
and actual SU2 checks completed; the v1 wrapper failed at the wrong default
sensor metadata comparison. first_cycle_audit_v2.json recovers those pinned
completed stages and passes the correct rotating-sensor/transported metric/BDF
checks: tensor error0,maximum state relative residual3.899327321487061e-16.
The v1 failure and recipe remain intact; no tolerance changed or CFD rerun.
The full campaign remains FAIL. Subsequent coarsen_changes_trace_n8_v1 was an isolated current-source trace with
three engine hooks, separate epochs/private exports and no production mutation;
see the terminal result above before launching further work. Published code checkpoint c743c12784e6cd09452874887e8b7b3d10c4e837.

Cycle1 coarsening exhausts512rounds:429accepted/83rejected,545private cells, all
545cached valid,323pending directions;1883orientation exclusions/988reactivations.
The accepted cycle0 CFD mesh/solution is retained. Last proposal failed frozen
q/J (.16126001018/.04295752091); this does not classify all83rejections. No new
private cycle1 mesh export or whole-mesh certification yet. Goal1 still needs
second-event larger completion/diagnosis, broader limits and repeated cluster
performance, instrumentation overhead and allocator/wait/traffic calibration.

Reusable SGE selection updated from69to71cases;31lightweight package/parser tests
PASS. coarsen_changes_package_checks_v1 audits actual41/121/359 unit ENGINE rows,
six lifecycle rows and four failed n8 runtime rows: finite integral new work
counters, nested timers inside selection/publication, and globally closed sent/
received notification records. Two collector-only failures (row-name assumption,
relative-path serialization) are retained separately; C++/MPI tests did not fail.
Old terminal receipts keep their original auditor pins; use this checkpoint's
accounting collector for the added fields. Rebuild and prepare a NEW closed SGE
bundle after updating; no cluster job launched and no request tonight.

See integration_evidence/native_3d_core_v1/COARSENING_CHANGES_VALIDATION.md.
The historical sections below describe their own earlier source checkpoints.

## Coarsening dependency gap and actual n8 rejection trace — 2026-10-11

Working repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-transport-accounting, predecessor06d0a859d9.
Production/test/build/tool code and qualified driver remain unchanged from d509.
Goal1 is ACTIVE. The user authorized starting Goal2 only after all Goal1 gates
pass; cluster runs are deferred until tomorrow. Do not call Goal1 complete from
these local experiments or proceed to Goal2 with missing performance evidence.

A source-matched selector/kernel reproduction now confirms a real full-star
retry gap. In coarsen_dependency_replay_v4, the original spatial P1 sensor has
xx=7 at interior vertex4 and0.25 diagonal tensors at all other original nodes.
Collapse4->0 fails the frozen J>=.05 gate withJ=.048765984909417068. A valid
neighboring face2->3 reconnection changes the removed vertex's complete star
while leaving the candidate edge star/coordinates/measurement unchanged. The
same collapse then passes (global3-cell result q=.68723,J=.40825), but the current
selector leaves it blocked. Independent exact geometry/overlap/skin/volume,
centroid metric and analytic original-P1 edge checks PASS; the target and gates
are unchanged. This is a local selection/kernel defect, not a distributed fix
or an accepted whole-engine campaign. Small inspectable SU2 grids are retained.

coarsen_trace_n8_v1 replays the actual frozen n8 failure using all68 configured
test objects and exactly three diagnostic-only engine insertions in an isolated
object. Production build stays untouched. All170 compared integer workload
counters and five baseline input/config/state files match. Adaptation remains
FAIL:512 attempts/272 accepted/240 rejected,2489 private cells/91 pending edges.
There are231 unique rejected directions, zero identical full-star repeated
rejections and nine repeats after changed dependencies. The trace shows120 missed
reactivation events affecting53 blocked directions when their removed-vertex
stars change without touching candidate edges; it does not prove any of those
n8 directions became admissible or that fixing invalidation completes n8.

Independent exact rational witnesses confirm183 geometric rejections:176 zero
and seven negative replacement determinants; all183 interior-point positive
controls pass.56 other rejections introduce oversized edges; one leaves an
incident finite reference facet. This points to a safe early orientation filter
in addition to complete dependency invalidation, not an unchanged-retry cache.
Any filter must retain the invalid-field repair path, use current exact local
counterexamples, and invalidate across all changed removed-vertex dependencies,
including remote edits. Charge index/notification work and memory explicitly.
A local absence of a counterexample cannot certify a complete distributed star.

refine_private.su2 and coarsen_rejected.su2 are retained in coarsen_trace_n8_v1
for inspection (3552/2489 tetrahedra). They are private diagnostic exports, not
accepted CFD/CGNS outputs or independently certified full mesh/field results.
The original failed campaign and accepted CFD/BDF rejection checks remain.
Full study: integration_evidence/native_3d_core_v1/COARSENING_DEPENDENCY_REVIEW.md.
Next: bounded full-star invalidation and conservative early geometry filtering,
MPI1/2/4 atomic/stale/cross-rank tests, then unchanged-target larger campaigns
and measured complete cost. No local performance/scaling claim.

## Disk-quota troubleshooting moved to a separate handoff — 2026-10-11

See [BEEGFS_QUOTA_HANDBOOK.md](BEEGFS_QUOTA_HANDBOOK.md) for the full evidence,
confirmed actual build path, primary references, unresolved diagnosis and a
new-session prompt. The user requested this topic be handled separately from
adaptation. Current cluster build completion remains unverified. Do not resume
quota troubleshooting as part of the adaptation goal unless the user asks.

Coarsening source inspection: rejected directions are erased from the queue;
cell Drop/Add can requeue incident edges after edits. There is no demonstrated
unchanged-proposal retry defect. A directed collapse admits the complete removed
vertex star, while retry invalidation currently follows incident edge changes;
changes elsewhere in that star need a specific adversarial MPI test before a
cache/queue fix. Any new early orientation filter must use exact counterexamples,
track complete dependencies and charge index/communication memory and cost.
Do not replace full-star checks with edge-only admission or weaken frozen gates.
Goal1 remains active; n8 bounded completion and measured performance remain open.

## Termination diagnostics and first n8 size limit — 2026-10-11

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; predecessor
5b9e453c28d42806e012aae54beda324b67c3513. Core remains
6b3fefd039bfc3d1937b8add7d21d0ea41344bc9; AdapNoExt is untouched.

Five constant-cost local counters now distinguish cached invalid cells from
unfinished candidate work on every engine exit. They are emitted in existing
NATIVE3D_DETAIL rows. No operator, guard, target, budget or queue policy changed;
no production loop/collective was added. C++ tests and the independent lifecycle
parser cover successful/rejected exits and corrupted counters. Historical rows
without these fields remain readable but provide no termination-state evidence.

Actual full68-object/774-member configured MPI/MMG/CGNS build PASS:
termination_build_v1, binary SHA256
29b583f1bdd17a54051c0c66f997683b7f20f3c410042dfb5402b640bfe34a76.
termination_mpi1_v1, termination_mpi2_v1 and termination_mpi4_v1 each PASS69 cases
per rank. Independent bridge audits PASS12/24/48 frames,6/12/24 current+BDF state
pairs and6/12/24 actual SU2/CGNS output/readback checks. Serial MMG2D/3D PASS48
assertions/two cases. package_checks_v19 PASS30 lightweight parser/mocked tests,
Python3.8 AST and Bash syntax. All source/object/archive/tool pins remain stable.
termination_lifecycle_mpi1_v1 also PASS: six actual metric/Hessian OFF/ON
controls remesh/replace geometry/solver/BDF and write meshes. Its independent
audit checks six successful engine rows with all new counters. Larger accepted
restart and MPI2/4 lifecycle evidence remain from the preceding checkpoint.

First n8 wedge N1M1 practical-limit pilot and diagnostic rerun both FAIL in
cycle0. Input405 vertices/1536 tetrahedra/512 boundary triangles; frozen targets
64/16/64, aspect1.5/spatial variation0.125, q>=0.20/J>=0.05/L<=1.8,512 rounds and
noise0 unchanged. Refinement457 accepted edits ->3552 private tetrahedra.
Coarsening512 attempts/272 accepted/240 rejected ->2489 private tetrahedra,
2489 cached measurements, zero cached invalid cells,91 queued candidates and
zero pending repairs. Last obstruction: edge642,297, cell74251. The queue is
required to be empty for completion, so the whole event rejects at the budget.
C++ current/BDF/reference/no-callback preservation checks pass before the single
failing COMPLETE assertion. No independent accepted-mesh audit/later cycle ran,
and no rejected private mesh was exported. Cached validity is not certification.

Preserved original FAIL folders transport_limit_n8_n1_m1_v1/v2 and PASS diagnostic
review termination_limit_review_v1 under integration_evidence/native_3d_core_v1.
The review checks receipts/hashes, identical input/config/source fields and 160
equal integer work/communication counters. Input envelope_input.su2 and
envelope_s0_initial.json are inspectable. Do not soften guards or increase rounds
to relabel these campaigns. Next: bounded/scalable coarsening completion and
blocked-proposal handling, followed by independent workload validation.

Cluster MMG lib64 discovery now works in the supplied log. Its static archive
needs SCOTCH from /global-scratch/bulk_pool/aero/utils_gcc10/SCOTCH_INSTALL;
README now includes mmg_scotch_root and runtime lib64. Cluster SU2 compile/link
and campaigns remain unverified; no qsub was submitted. User quota output shows
47.94GiB free, while shared aero flash-pool headroom is only about133MiB;
a transient group exceedance plus enforcement refresh is a plausible explanation,
not a confirmed Meson-caused quota incident.

Goal1 remains ACTIVE. Repeated cluster performance/instrumentation overhead,
larger accepted envelope, full MPI wait/traffic/vendor/allocator calibration
remain open. Sequential one-CPU nice19/thread1 checks are correctness evidence,
not sustained speedup/scaling/affordability. No owned numerical/build job is live.

## Larger frozen-work/restart qualification and MMG lib64 fix — 2026-10-11

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; predecessor
5a258c8325383eda1fd7e96d2235dba365c42277. AdapNoExt is untouched. Production
C++/test fixtures remain identical to475820f366366f471e0cdb50e362d7c5e572c328;
core branch remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

Two actual n4 frozen-work cases (N1M1 and N4M2) and persisted SU2 N1M1/CGNS N4M2
restart cases now PASS. Strict unchanged analyze_reference_admission.py verifies
26 byte-identical mesh/source/current+BDF/readback JSON files against the
same-rank baselines and245 closed phase rows. Independent geometry, original-P1,
current/BDF, output/reload and corruption controls pass. Targets and q/J/L guards
are unchanged. These sequential one-CPU contended correctness runs qualify the
larger/restart gates; they do not establish scaling or sustained speedup.

Evidence in integration_evidence/native_3d_core_v1:
- reference_admission_n4_n1_m1_v1 and reference_admission_n4_n4_m2_v1
- reference_admission_restart_su2_n2_n1_m1_v1
- reference_admission_restart_cgns_n2_n4_m2_v1
- reference_admission_regression_v1 (aggregates actual configured MPI1/2/4 and
  serialMMG logs/pins, without rerunning; retains original serial collector FAIL
  and the separately verified successful postprocessing receipt)
- REFERENCE_ADMISSION_VALIDATION.json and reference_admission_rank_phases.csv

Those numerical checks completed before the following build-discovery-only
change. The cluster MMG prefix contains include/mmg/libmmg.h and lib64/libmmg.a;
SU2 previously searched only lib. meson.build now searches lib and lib64 for
combined MMG and explicit SCOTCH prefixes. Required-library checks and the
existing MMG2D/3D compile/link guard are retained. No numerical code/option change.
mmg_lib64_discovery_v2 PASS: exact production dependency block, real static MMG
and SCOTCH copies under lib and lib64, plus empty-MMG-archive rejection. v1 logs
retain the missing-Ninja/collector-path failure; v2 uses the existing actual
local Ninja and resolves the collector path. No full SU2 rebuild or cluster
execution is claimed for this discovery patch.

Cluster: pull this checkpoint on codex/native-3d-transport-accounting. Keep
mmg_root=/home/aero/arausa/global/Libs/mmg-5.8.0/install (do not append lib64).
The old Meson KeyError for options comes from a saved cmd_line.txt missing its
options section: preserve that directory and setup a fresh timestamped build
with existing compiler/MPI/additional flags and --prefix="$(pwd)" if desired.
Build UnitTests/test_driver, then freeze a fresh prepared bundle using that build
directory. README contains preparation/submission commands. Actual cluster
compile/link and campaigns remain unverified; no qsub was submitted here.

Goal1 remains ACTIVE. Current MPI1/2/4 and larger frozen-work/persisted-restart
numerical gates are qualified. Remaining: repeated cluster performance and
instrumentation overhead, n8 practical limits, full MPI wait/traffic/vendor and
allocator calibration. No curved geometry, robust3D BL, full unsteady CFD or
CFD affordability claim. No owned numerical/probe process remains live.

## Contention-authorized MPI correctness and cluster audit gate — 2026-10-11

Working branch: codex/native-3d-transport-accounting, predecessor
ca700f7d686804cf7df7d8eaade73bfa65562e95. Production C++/test fixtures/build
options remain identical to475820f366366f471e0cdb50e362d7c5e572c328; core branch
remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. No numerical guard/target change.

The user explicitly authorizes correctness-only MPI runs despite CPU contention.
Retain actual host observations, available-memory admission, one owned sequential
job, bounded CPU affinity, nice19/yield/thread1; defer local performance claims.
This instruction overrides the previous CPU-idle gate for correctness checks.
Original resource-refusal evidence stays intact. The shared execution contract
now records this distinction. No foreign process was stopped or modified.

Full configured68-object native/reference/2D+3Dbridge selection PASS at MPI2 and
MPI4,69 cases per rank, using the same pinned serial475820 production binary and
774 archive members. Independent bridge audits PASS: MPI2,24frames/12statepairs/
12actual SU2CGNS round trips; MPI4,48frames/24statepairs/24round trips. Both cover
all M=1..N, two solution-transfer policies and three refine/coarsen/refine events,
exact embedding/volume/markers, original-P1 metric gates and current/BDF states;
each independent audit rejects its five corrupted controls. All source/object/
archive/tool pins unchanged; rejected/accepted transport totals close on each
rank. These are one-CPU contended correctness runs, not timing/scaling evidence.

Retained case folders:
integration_evidence/native_3d_core_v1/transport_mpi2_correctness_v2
integration_evidence/native_3d_core_v1/transport_mpi4_correctness_v2
Inspect bridge_barycentric_m*_s*_written_SU2.su2 and matching CGNS, source/state
JSON, independent_audit.json, transport_rows.json and validation.json.

The SGE correctness runner previously executed the bridge assertions but omitted
the existing independent audit. It now requires audit_geometry_bridge.py to
PASS after each MPI1/2/4 selection before progressing. Failed audits preserve
logs and stop later cases/campaigns. Package checks_v18 PASS29 lightweight
mocked/parser controls including the injected MPI2 audit failure. Python3.8 AST,
Bash syntax, scoped whitespace and unchanged production/fixture/build options
PASS. This script-only update needs no solver rebuild; prepare a fresh bundle
after pulling. Never modify an already frozen/submitted bundle. Existing jobs
remain useful, but their bridge outputs need separate independent review.
The README also explains that --prefix controls installation; the bundle copies
the build-directory test driver and requires no local /tmp files.

Goal1 remains ACTIVE. Current small MPI1/2/4 regression is now qualified; larger
n4 fixed-work/persisted-restart qualification, n8 practical limits, repeated
cluster performance/instrumentation overhead and full wait/traffic/vendor/allocator
calibration remain open. No curved geometry,3DBL/full unsteady or CFD affordability
claim. No owned process is live after both terminal PASS results; no qsub was
submitted by this session and no downloaded native3D campaign is present.
Do not merge the isolated transport branch into core before the larger gates.

MPI2 maximum current/BDF relative error: 6.0360605402784081e-13; MPI4: 8.471971137321857e-13. Transported-tensor maximum error:0 in both.

Overnight operating instruction (2026-10-10): the user is unavailable to launch
cluster jobs. Do not ask for launches tonight. Prepare closed scripts/evidence
collection if needed and report pending commands when asked tomorrow. Continue
small sequential local checks with real host-contention checks; push each
validated checkpoint and verify the remote SHA. A terminal capacity failure
cannot be guaranteed to auto-resume; preserve the handoff for recovery.

## Updated cluster batch authorized — 2026-10-11

User can now launch cluster runs and asks for exact commands. Use updated branch
codex/native-3d-transport-accounting, current code/tool checkpointdbd93a2dc26f56a233ebefa8ab494991ef52a14a;
core remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. Build the complete68-object
normal driver with MPI/MMG/CGNS/tests, MMG prefix
/home/aero/arausa/global/Libs/mmg-5.8.0/install and preserved library paths.
Freeze a fresh timestamp-named bundle, then submit the seven serialized default
SGE campaigns plus --limits (eighth n8 practical-limit job). No mpiP dependency
is needed for this batch. Correctness/lifecycle PASS gates precede timing/limits;
submission does not establish success. Do not alter old/running bundles.
The optional deep-MPI campaigns remain separate until an MPI-compatible mpiP
library plus license and real loading checks are available.

transport_mpi2_onecpu_v1 is TERMINAL: fresh one-CPU gate refused BEFORE launch
(load1=8.20 >=7 and selectedCPU3 minimum idle0.395 <0.50). Both MPI ranks would
have shared one actual CPU at nice19/thread1/yield; total CPU allocation was
reduced rather than claiming two idle CPUs. Original two-CPU refusal is retained.
All build/source/binary/68object/774member/tool pins remain unchanged. Only three
host-observation subprocesses ran; no solver/MPI/audit launched. No local timing
or new numerical result. This folder contains runner/gate/provenance/refusal,
not a mesh generated by a successful run.

No owned process is live; no qsub submitted by this session. Until actual cluster
job state/results are available, user launch intent is not a verified wait.
Goal1 ACTIVE with the original scope. Current MPI2/4, larger frozen and persisted
restart qualification, repeated performance/size limits, full wait/traffic/vendor/
allocator calibration remain open. Handoff/findings/evidence preserved; AdapNoExt
and foreign work untouched. Case outputs will be announced by the existing runner
under ClusterResults/native3d_<job>_<mode>_n<ranks>, with launcher logs and compact
mesh/source/current+BDF/audit artifacts only.

## Checked verbose MPI report analysis — 2026-10-11

NativeIntegrated / codex/native-3d-transport-accounting; predecessor
7b3c607b111ad1472acfe08137277c5af5457e79. No production C++/fixture/target/guard
change. Core remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

Fresh actual-host idle samples0.1607031/0.1132666/0.0343965 and load1=7.33
refused any numerical MPI launch this turn. Four foreign Python ranks each
used approximately99% CPU. No owned heavy process or qsub was started; do not
interpret intent/state files as a live cluster wait.

The optional profiler runner now requires a complete verbose report. Shared
mpi_profile.py checks all rank assignments and times, declared stack/rank-record
counts, exact callsite API-count closure and reported time closure with explicit
display-precision bounds. The top-twenty table is never used as a total.
Legitimate M<N callsites on only the worker ranks are accepted. Source stacks
stay attached; no inferred phase/window/communicator identity. Analysis exports
mpi_api_profiles, whole-process per-rank totals, per-function/callsite rank work,
observed wrapped-API counts and MPI-time max/mean. These are not pure wait,
wire traffic, MPI work inside a particular remesh window or complete API coverage.
The print threshold is explicitly0; report generation and parent overhead remain
separate paired measurements. Normal campaigns remain unchanged.

package_checks_v16 passes26 preliminary lightweight checks. Review caught a
generator-consumption issue in accumulated display bounds; corrected before
publication with a dedicated control. Final package_checks_v17 passes28
mocked/parser controls (1.499s unittest,2.074s wrapper), AST/Bash/diff and
production/fixture/build-option equality to475820. It retains a labeled synthetic
four-rank report/summary and pinned upstream format source identity8a2ae9e9458.
No actual mpiP sample is available in that upstream tree. These checks do not
validate actual loading, MPI2/4 numerical behavior or profiling cost.

Goal1 remains ACTIVE: current MPI2/4, n4/persisted-restart qualification, repeated
cluster overhead/accepted-work/scaling, n8 practical limits and full wait/vendor/
allocator calibration remain open. Retain original resource/collector failures.
Do not merge the isolated transport branch into core before MPI qualification.
User core checkout, AdapNoExt, foreign submodules/results and old frozen bundles
are untouched.

## Optional calibrated MPI campaign preparation — 2026-10-11

NativeIntegrated / codex/native-3d-transport-accounting; predecessor
78f6800a153812c05fbf239cdac5a1d4271faebd. No production C++, numerical fixture,
target or guard change. Core remains the user's6b3fefd039bfc3d1937b8add7d21d0ea41344bc9
cluster checkpoint. Do not merge the transport branch before MPI qualification.

transport_mpi2_v1 was refused BEFORE MPI launch: three idle samples included
0.2993068683 below the frozen0.30 gate, load1=6.41. No rank started. This is
resource-admission evidence, not a numerical failure or a live wait. A later
host sample remained busy; no repeat/local timing/cluster job was launched.
Original refusal validation and host evidence are retained.

Optional mpiP support freezes a shared library plus matching complete license
inside a new closed bundle. Three appended SGE campaigns are serialized and
gated by same-package correctness and N1/2/4 lifecycle PASS. Application-only
preload leaves the launcher/auditors untouched. Native detail remains ON; each
M=N/M<N choice has a warm-up and three alternating profiler OFF/ON pairs and
unchanged exact mesh/source/current+BDF/readback hashes. Missing/incomplete
all-rank reports fail and preserve uncompressed evidence. Offline analysis
distinguishes parent overhead from process overhead including report finalization.

package_checks_v13 preserves the initial mocked collector failure (empty recipe
directory); v14 passed22 checks after correction. Final-source package_checks_v15
passes23 checks (0.962s unittest,1.290s wrapper), AST/Bash/whitespace and
production/fixture/build-option equality to475820. Actual mpiP library
loading, dependency compatibility, callsite attribution and overhead remain
unverified; this host lacks libunwind/BFD development files. No dependency was
installed, no binary rebuilt, no profiler/MPI/CFD/qsub launched.
mpiP API statistics do not measure pure waiting, wire traffic or chronological
phase windows; full traffic/wait/vendor/allocator calibration remains open.

Goal1 ACTIVE. Pending current MPI2/4, frozen n4 and persisted-restart qualification,
n8 practical limits and repeated cluster evidence. No curved geometry/3DBL/full
unsteady/CFD affordability claim. New package README has deferred commands; keep
the user's existing core checkout and frozen/submitted bundles unchanged.

## MPI coverage lower bound and actual serial lifecycle — 2026-10-11; checked

Evidence/tools checkpoint a2f8b2f1c7c13b9b45b3221d3ee6bb87b364b6ea pushed to origin/codex/native-3d-transport-accounting; remote SHA verified. Production remains475820f366. Core remote remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

NativeIntegrated / codex/native-3d-transport-accounting; predecessor7398f47f076e19c7ff80f57f4044a3ea4a14bb06.
No production/test-fixture/target/run-package change. Core remains6b3fefd039b.
Source-pinned coverage audit of40 logged engine invocations proves at least
25984 collective API calls:16573 wrapper calls plus9411 directory elections.
At least36.22% of known collective-call count is absent from the wrapper counter;
this is not elapsed-time attribution, network traffic or a scaling result.
Successful failure elections already have a one-reduction fast return.
Three lightweight counter/schema/corruption controls pass. Ledger and limits:
integration_evidence/native_3d_core_v1/MPI_COVERAGE_REPORT.md.

transport_lifecycle_mpi1_v1 PASS:462 assertions in existing case; six actual
metric/Hessian/remesh/repartition/rebuild/current+BDF/output controls, independently
audited6frames/21closed wallCPU stage records/21nested records and complete
current transport scopes. Same built binary/source as475820f366, all pins
reverified. One nice19/thread1 MPI rank after fresh idle/load gate; no concurrent
owned job. Existing fixture repetitions are correctness controls on this busy
host, not a local performance certificate. No CFD time integration or MPI2/4.

Actual case/result folder:
integration_evidence/native_3d_core_v1/transport_lifecycle_mpi1_v1.
Inspect lifecycle_m1_d{0,1}_r{0,1,2}_mesh_00001.su2 and matching .json/.cfg/
.native_ref plus lifecycle_audit.json and rank_profiles.json.

FullGoal1 ACTIVE. Still need MPI2/4/M<N/frozen n4/persisted restart, repeated
cluster and n8 practical limits, plus calibrated full MPI/vendor/wait/memory
coverage. No owned job live or qsub submitted. Never change a running core bundle
or merge this separate feature into core before broader qualification.
AdapNoExt, foreign submodules/ClusterResults and original failure evidence untouched.

