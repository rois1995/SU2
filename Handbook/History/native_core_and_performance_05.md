## Isolated native3D transport accounting — 2026-10-11; built and serial checked

Implementation/evidence checkpoint475820f366366f471e0cdb50e362d7c5e572c328 pushed to origin/codex/native-3d-transport-accounting; full remote SHA verified. origin/codex/native-3d-core remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. Subsequent publication-note commit changes documentation only.

Working branch codex/native-3d-transport-accounting starts at pushed core
6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. codex/native-3d-core remains the user's
cluster checkpoint. Three disjoint import/parent/worker World-wrapper totals
capture attempted work on ordinary rejection, including previously omitted
separate-import traffic. Bridge/engine detail reports per-call deltas; directory-
election API counters are separate nested data. No new runtime clock, MPI call,
query or numerical operator change. Direct/vendor MPI and actual waiting still
need calibration. Shared audits check closure and label legacy coverage.

transport_build_v1 PASS:14 actual nice19/j1 configured build steps, full68 objects/
774 thin members. transport_serial_v1 native10189assertions/69cases and serial
MMG48/2cases PASS; independent bridge12frames/6states/6SU2CGNS round trips PASS.
The collector then failed on a missing helper sys.path: preserve that FAIL receipt.
transport_serial_audit_v2 recovers by checking the same retained results/pins
without numerical rerun, SERIAL_AUDIT_PASS.26 non-CGNS fixtures byte-identical to
prior serial evidence; CGNS binary containers differ but exact signatures/readers
pass. Max state error2.7000623958883732e-15. package_checks_v12 is the final-source
19-check lightweight repetition PASS (0.480s unittest /0.778s wrapper).
Report: integration_evidence/native_3d_core_v1/TRANSPORT_ACCOUNTING_REPORT.md.

Current native result folder:
integration_evidence/native_3d_core_v1/transport_serial_v1/native_serial.
Includes default_native3d.su2 and bridge_barycentric_m1_s*_written_SU2.su2 /
_written_CGNS.cgns with corresponding original-P1/current+BDF audit data.
These are small manufactured correctness cases, not practical CFD grids.

The feature matrix preserves all original numerical guards/gates and requires
current transport records. MPI2/4/n4/persisted-restart qualification and repeated
cluster/n8/memory calibration remain pending. FullGoal1 ACTIVE. No owned job is
live; no local timing campaign or cluster job was launched. AdapNoExt, foreign
submodule/results and original failure evidence remain untouched.
Do not merge this feature into core before broader MPI qualification.

## Cluster MMG prefix and SGE runtime search paths — 2026-10-11; package checked

NativeIntegrated / codex/native-3d-core, predecessor1ba501f9dfc97b17688186cf56cca0c49f152818.
User build-native failed locating libmmg with default mmg_root=/usr/local. Older
downloaded582358/581950 metadata has enable-mmg=false. User supplied cluster
prefix /home/aero/arausa/global/Libs/mmg-5.8.0/install; README now configures it
explicitly and exports its lib search path. submit.py now sends PATH and defined
LD_LIBRARY_PATH via qsub -v names, preserving compiler/MPI/Python/shared-library
paths into the existing GCC-prepending SGE script without full environment export.
Freeze a new bundle after updating if the previous one contains old submission code.

package_checks_v10 passes18 lightweight checks, including actual Bash launcher
with a fake Python probe, mocked scheduler env/serialization/gates/partial receipt,
plus syntax/diff/source/artifact checks. No build/MPI/CFD/qsub launched; remote
MMG install/header/library/dependencies/link/load unverified. Production C++,
configured driver, targets/guards and auditors unchanged. Report:
integration_evidence/native_cluster_3d_core_v1/CLUSTER_MMG_SETUP_REPORT.md.

Goal1 ACTIVE; actual MPI/reference-admission qualification, repeated cluster
performance/n8 limits and full traffic/wait/memory calibration remain. Communication
coverage tracing is incomplete: World statistics omit failure elections/direct
passive/ADT/communicator/vendor calls. Do not infer MPI waiting from wall-minus-CPU.
No owned jobs live; AdapNoExt, foreign submodule work/results and prior evidence
untouched. No MMG-disable workaround or numerical PASS claimed.

## Native3D offline phase/rank analysis — 2026-10-11; checked, analysis only

Published analysis/evidence checkpoint 3e55caee966eef795f60b317a24ba7d948a999a9 on origin/codex/native-3d-core; full remote SHA verified. No executable or run-recipe change.

NativeIntegrated / codex/native-3d-core; predecessor ffeeac545762fb95b116fe47c797f3747e8c0d6f.
The analyzer now retains lifecycle OFF/ON timings and wall/CPU stage rows, plus
closed critical-rank phase breakdowns, active-worker CPU/work/query imbalance
and separately labelled nested detail for timing/limits/lifecycle. M<N reports a
separate closed active-worker row so inactive subset waiting is not mistaken for
the expensive worker phase. Unknown worker count stays null; no CPU-derived MPI
waiting estimate. Preparation includes the helper; C++/driver/runner/auditors and
targets unchanged, no rebuild/new cluster submission needed for analysis alone.

package_checks_v9 passes16 lightweight checks plus syntax/diff checks;
profile_analysis_replay_v2 verifies two pinned historical binaries/logs and
summarizes21 native,76 exclusive stage and84 nested events, preserving old audits.
Historical N4M2 coarsening:8.5271s native versus7.3171s active-worker coarsening;
private metric CPU max/mean1.2571. Contended pilot, no current speedup or wait-cause
claim. Report: integration_evidence/native_cluster_3d_core_v1/PROFILE_ANALYSIS_REPORT.md.

Goal1 ACTIVE. MPI2/4/reference-admission fixed-work/restart qualification, repeated
SGE timings/n8 limits and full privateCPU/wait/traffic/allocator/vendor calibration
remain. User received the seven-job plus optional eighth --limits launch commands.
No owned build/MPI/CFD/qsub jobs live; foreign ClusterResults/submodule work and
AdapNoExt untouched. Preserve already frozen packages and use current checkout
for offline analysis rather than changing a running package.

## Native3D query accounting — 2026-10-11; built and serial checked

Implementation/evidence checkpoint 542ba6375219cb84ddc4c7b8ab1bc02e42fe4a3b pushed to origin/codex/native-3d-core; full remote SHA verified. Serial checks pass; MPI and broader cluster qualification remain pending.

NativeIntegrated / codex/native-3d-core, predecessor9fddd081a8435e83a7f069043608d2b1f68ac437.
FIELD_ACCOUNTING_DRAFT_REPORT.md supersedes the initial uncompiled status below.
reference_admission_build_v2 passed its measured host gate (idle27.33%,load1=6.76)
and completed19 nice19/j1 build steps, complete68-object configured driver with
source/binary/object pins. reference_admission_serial_v1 passes serialMMG48 plus
all69 selected native/reference/2D+3Dbridge cases and9996 assertions, including
512-facet reference/300-facet CFD import/reload/budget refusal and retained failed
engine/weight/reader queries with timingOFFON and accepted/output preservation.
One owned job/rank at a time, threads1, scratch on4TB; both jobs terminal.

Native engine/bridge detail now exports actual cache misses, invalid queries,
failures and evictions. Local guards/after-catch aggregation preserve rejected
query work, without changing original-P1 interpolation, geometric composition,
mesh operators/targets/guards or2D/MMG implementations. Shared current auditors
check complete request closure and bounds; historical incomplete logs use their
saved original tools. package_checks_v6 passes12 lightweight checks and syntax;
source/artifact pins verified. initial resource refusal preserved in build_v1.

MPI2/4 and independent fixed-work n4/persisted-restart qualification remain
pending; last FULL MPI-qualified production is1fafe9e83c. REFERENCE_ADMISSION_MATRIX
freezes15 exact source changes and the original69/full68/26-byte-identical gates.
FullGoal1 ACTIVE, no larger/3DBL/curved/fullunsteady/PDE/affordability claim.

The user asked about cluster work: package is ready for their existing on-cluster
normal MPI/MMG/CGNS build of current branch, then prepare.py and submit.py
--limits. Default7 serialized correctness/lifecycle/timing jobs; optional eighth
n8 (405points/1536tets/512facets) first-limit job covers N1M1,N2M2,N4M2,N4M4 and
stops on first failure. Nodes need no Git; retain/download native3d_* and launcher
only. No actual bundle or qsub was launched here. See package README for commands.

## Native3D field accounting draft — 2026-10-11; C++ NOT VALIDATED

NativeIntegrated / codex/native-3d-core, predecessor9fddd081a8435e83a7f069043608d2b1f68ac437.
FIELD_ACCOUNTING_DRAFT_REPORT.md records missing engine misses/invalid/failure/
eviction exports and lost query work on thrown selection/bridge/private checks.
The draft aggregates actual FrozenField counters on failure via local stack
guards/after-catch collection, preserving the existing interpolation/composition,
mesh operators, targets/guards and2D/MMG sources. No new queries/cache allocations/
collectives. Existing selected tests now inject engine/weight/reader composition
failures with detailOFFON and verify counters plus accepted/output preservation;
these C++ assertions remain uncompiled and NOT RUN.

Shared current lifecycle/envelope/cluster audits now require complete counter
closure/bounds; old logs use their saved original auditors. package_checks_v6
passes12 lightweight fake/mock/synthetic-counter checks plus Python/shell syntax
in0.303s. Source/artifact hashes verified; no native numerical PASS claim.

REFERENCE_ADMISSION_MATRIX now freezes15 exact intended source differences against
both retained reference-lookup baselines, including accounting/auditors and prior
n8 fixture/runner. Its n4 fixed-work/n2 actual persisted-restart targets,69 selected
cases/rank,full68 objects and26 byte-identical output gates remain unchanged.

reference_admission_build_v1 retains source snapshots and a terminal preflight
FAIL due resource refusal, not a C++ compile error: idle27.33% but one-minute load
above8; no compiler PID or compile stage, sources/artifacts verified unchanged.
No owned numerical/build job remains. Scratch stayed on4TB. NEXT new contention-
checked build folder, then configured reference_admission_regression_v1 and
frozen fixed-work/restart qualification. Last validated production1fafe9e83c;
fullGoal1 ACTIVE, actual overhead/traffic/privateCPU/wait/memory/larger-size and
cluster limits remain. Foreign workers/results,AdapNoExt and earlier evidence
untouched; do not treat draft publication as numerical validation.

## Larger native3D practical-limit preparation — 2026-10-11; not a numerical result

Implementation/evidence checkpoint 6565e5575e5b4e00f4f938434d05040227a1d4a7 pushed to origin/codex/native-3d-core; full remote SHA verified. C++/MPI validation remains pending.

NativeIntegrated / codex/native-3d-core, predecessor6db495a38bc. Prepared optional
n8 wedge first-limit campaign:405 points/1536 tets/512 surface triangles, dyadic
coordinates, unchanged rotated/spatial original-P1 target64/16/64 and guards.
Only existing test fixture/runner/package/docs change; no production source change.
submit.py --limits appends one gated four-slot job after the default seven jobs,
with sequential N1M1,N2M2,N4M2,N4M4 and stop on first failure. Case parameters and
relevant environment are now retained before launch, including rejected runs.
Single-pilot summaries retain elapsed/accepted work/imbalance/bounds/RSS, without
fixed-work cross-partition or repeated-timing claims.

PRACTICAL_LIMIT_PREPARATION_REPORT.md and package_checks_v5 retain11 passing
lightweight orchestration checks plus Python/shell syntax; initial mock-recipe
failure is preserved in package_checks_v4. No C++ build, MPI/CFD, freeze or qsub.
Real host sampled0-1% idle, foreign FSI/conservation workers active; no owned heavy
job started. Scratch stayed on4TB. Measured /tmp/native*9MB;9.3GiB metric build is
the other agent's worktree,2.2GiB wall build another session; no foreign cleanup.

NEXT: when host permits, run reference_admission_regression_v1 and the unchanged
REFERENCE_ADMISSION_MATRIX fixed-work/restart cases, qualify and publish actual
numerical evidence; then current bundle freeze and user-authorized cluster launch.
C++ reference-admission/n8 fixture remain uncompiled. Last validated production
1fafe9e83c; fullGoal1 ACTIVE. Larger adaptation, detailed traffic/cache/privateCPU/
wait/memory calibration and disabled-detail overhead remain unproven.

## Initial native3D SGE package — 2026-10-11; orchestration checked only

Package implementation/evidence checkpoint b8add069d4009706fa297a48125bcb52b65522d8 pushed to origin/codex/native-3d-core; full remote SHA verified. Numerical validation still pending.

NativeIntegrated / codex/native-3d-core, predecessorb441f55b412ff30dbd997890746c6aff62480bdf.
integration_evidence/native_cluster_3d_core_v1/README.md and PACKAGE_PREPARATION_REPORT.md describe a closed initial MPI1/2/4 correctness/lifecycle/repeated OFFON timing package. No new C++ build/mesh run/real qsub was performed; reference-admission draft still pending validation, last validated production1fafe9e83c. Nine lightweight fake-file/synthetic-timing/process/mocked-scheduler checks pass in package_checks_v3, with source/artifact pins; priorv1/v2 preserved. Actual configured Ninja dry-run finds19 pending steps and preparation_refusal_v1 proves the stale executable is refused without binary mutation/output bundle.

Preparation requires the current complete68-object normal MPI/MMG/CGNS/test driver and records source/object/archive/thin-member/build-option pins with drift checks. Bundles are closed regular files; no Git/compiler on nodes. Seven serialized SGE jobs: correctness4slots (serialMMG/native/ref/2D3Dbridge MPI1/2/4 and persisted SU2N1M1/CGNSN4M2), actual metric/Hessian lifecycle N1/2/4, then manufactured n4 timing N1/2/4. Jobs gate same-package correctness; timings also gate same-rank lifecycle. Partial submission receipts preserve IDs/pending acknowledgements; no automatic retry or real submission here.

Timing uses N=M/M=max(1,N/2), warm-up plus3 paired repetitions with alternating detailOFFON;13 raw data files must match within each fixed N/M. Raw logs/SU2CGNS/sidecars/BDF remain inspectable; audited large JSON gzip is verified reversible, with no-overwrite restore. Results retain code/pins/audits and exclude ELF/object/build trees. Download native3d_* result and launcher folders only. Reviewer separates exclusive/nested phases, reports median/spread/useful edit rates/RSS/bounds/imbalance and refuses changed artifacts/work.

CPU remained saturated by foreign FSI after the foreign build ended (load~19 ->~10, sampled idle4–7%); no owned heavy job started. Root filesystem transient ENOSPC prevented one heredoc before execution; retry using python-c succeeded. Workspace~77GiB free, root~1GiB. Only small owned /tmp scratch; no foreign cleanup. NEXT remains contention-gated reference_admission_regression_v1 and frozen fixed-work/restart matrix, then actual package freeze/cluster launch only when requested. Detailed cache-miss/traffic/privateCPU/wait/memory calibration, larger sizes/ranks/practical limits remain. FullGoal1 ACTIVE; orchestration PASS is not numerical/3DBL/curved/fullunsteady/PDE/affordability completion.

## Reference admission draft — 2026-10-11; NOT VALIDATED

Draft checkpoint `d0d3fdaa1ecccd88c981fc770a69729449804444` pushed to
origin/codex/native-3d-core; full remote SHA verified. C++/MPI/runtime
validation remains pending; the last validated production checkpoint is1fafe9e83c.

NativeIntegrated / codex/native-3d-core, based on pushed validated checkpoint
`1fafe9e83c9c8d3f9518740c04dba25c15d8e8aa`. The draft replaces the global256-facet
count guard with requested-memory admission in PlanarReference, ImportGeometry,
DecodeReference and collective LoadReference/factory. It bounds declared sidecar
counts by actual file bytes before facet allocation, keeps the independent1MiB
file limit and private64-cell/256-donor bounds, and distinguishes missing sidecar
from recoverable elected memory refusal without changing published authority.
Reference I/O now admits actual byte/parse/construction demand rather than the
old fixed8MiB placeholder. Construction scratch remains a conservative proxy,
not calibrated allocator/RSS accounting.

Exact forward-collinear border subdivisions are reduced to identical oriented
half-planes before the existing all-vertex convex certificate. Separate hole
loops and nonconvex fallback remain; no hull filling, tolerance or geometry
projection is introduced. No sensor/BL/fade/noise/operator/quality defaults or
2D/MMG sources change. New targeted tests cover512 tiled planar facets, constant
rectangular border query work, oblique/nextafter/orientation controls, actual300
facet CFD import/encode/reload, malformed oversized declared counts, and root-only
early/late memory refusal preserving state. They are written but NOT RUN.

`integration_evidence/native_3d_core_v1/REFERENCE_ADMISSION_MATRIX.json` freezes
configured regression, two n4 fixed-work N1M1/N4M2 cases and actual persisted
restart SU2N1M1/CGNSN4M2 cases. `analyze_reference_admission.py` is prepared,
syntax checked, not executed against new results. `reference_admission_preflight_v1`
retains source pins, clean diff check and real host logs: eight CPUs, load~17,
zero sampled CPU idle, four foreign FSI ranks plus foreign nice19/j2 build
(`/tmp/su2-wall-fixes-build`, PID3859854 at inspection). NO owned build/run/audit
was launched. Do not treat draft publication as any C++/MPI/numerical PASS.

Next: check actual host contention; once available run one nice19/j1 configured
regression in `reference_admission_regression_v1`, preserve failures, then
sequential actual fixed-work/restart cases from the frozen matrix and run the
qualifier. Announce each full folder first; no source/pinned-helper changes while
a campaign is live. Compare26 raw mesh/state/readback files and rank-local edits,
verify production/member pins and independent exact audits, then publish a
validated checkpoint. Global-cap removal does NOT yet certify larger adaptation;
follow with closed larger-domain first-limit cases/SGE package. Complete detailed
traffic/cache/privateCPU/wait/memory calibration and disabled-detail overhead
remain open. FullGoal1 ACTIVE, no full3DBL/curved/unsteady/PDE/affordability claim.

## Canonical planar reference lookup checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessore99dee967611ec960a4e9368bcd7df8b7aba1b3d.
Implementation/evidence checkpoint `31df202a90d549fc834fa8d9b16a38edef4c7f36` pushed to origin/codex/native-3d-core; full remote SHA verified.
REFERENCE_LOOKUP_REPORT.md/REFERENCE_LOOKUP_VALIDATION.json qualify canonical same-marker physical binding: direct singleton lookup, house local CADT box broad phase for multiple patches and unchanged exact finite containment/orientation/lowest-ID checks. Resident/construction/query-workspace admission updated; conservative component scratch still needs allocation calibration. No geometry certification, sensor,BL,operator,guard or cap changes. Global256-facet/1MiB-sidecar limits remain; next work reduces collinear certification cost and replaces the global count cap with memory admission across import/decode/restart, then larger first-limit cases including closed same-marker multiple patches.

Actual complete68-object configured regression passes67 native/reference/2D+3Dbridge cases per MPI1/2/4 rank plus serialMMG48 assertions. New lookup test compares against exhaustive containment for tiled/disconnected/marker/plane/gap/reversed/nextafter/nonfinite and output-preservation cases. Two actual n4 wedge three-event N1M1/N4M2 cases pass independent exact geometry/q/J/L/P1/currentBDF/SU2CGNS and6corruption controls;26 raw mesh/source/state/readback files and rank-local accepted edits match vertex_clip baselines byte-for-byte. Only boundary header/implementation, import caller and targeted test sources change;9 native3D/factory production members rebuild, external/MMG/2D members pinned.

Single contended serial import-face samples135/128/55ms ->9/6/2ms; no sustained speedup/scaling claim. Closed phase CSV/useful edit rates retained, nested costs separate; packed bytes include self traffic and collective elapsed is not purewait. Complete communication/cache/private-wait/memory calibration,detail overhead and closedSGE campaigns remain. No fullAD/PDE/curved/robust3DBL/fullunsteady/affordability claim. FullGoal1 ACTIVE. All owned campaigns terminal; foreign FSI,AdapNoExt,ClusterResults and prior evidence untouched.

## Larger wedge and exact-vertex clipping checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, baseline7bc3869babb19171fe402fdfb0e96f745b769703.
Implementation/evidence checkpoint `9dc21db012be53a1e723b3f01e7649c7cad2b5ab` pushed to origin/codex/native-3d-core; full remote SHA verified.
LARGER_WEDGE_REPORT.md and VERTEX_CLIP_REPORT.md under integration_evidence/native_3d_core_v1 qualify four n4 wedge cases each:N1M1,N2M2,N4M2,N4M4. Input75points/192tets/128triangles; oblique and marker-junction geometry; three rotated/spatial originalP1 events16/4/16; guards.20/.05/1.8/default512/noise0 unchanged. All exact geometry/q/J/L/P1/currentBDF/SU2CGNS audits pass:minq.2474003062/minJ.07957138227/maxL1.7987750917934346; states<=3.865e-16,tensors0. Larger baseline uses unchanged previously qualified production.

The measured local hotspot is exact edge trace (~99% of metric-edge work). Production now reuses validated exact-volume/zero barycentric numerators at exactly matching donor vertices, retaining all other exact arithmetic and rejection checks. Only CNativePredicates3D.cpp.o changes in production archives; one targeted NativeMesh3D test added. All52 fixed-work geometry/source/state/readback files and accepted rank-local edits match baselines byte-for-byte. Full configured68-object regression passes66 native/reference/2D+3Dbridge test cases per MPI1/2/4 rank, plus serial MMG2D/3D48 assertions; passive predicate scalar/DD/AD macro syntax passes, no active AD run claim.

Isolated1622-chord probe compares exact returned data,3 alternating O2 samples/event:kernel median lower31.24%,38.20%,20.72%. Actual full-cycle pilots are single contended samples; MPI4/M4 is slower in two events. No end-to-end speedup claim. Both matrices retain374 closed phase rows each, elapsed event maxima and useful edit rates; nested work is labelled separately. Packed bytes include self traffic, wrapper collective time is not purewait. Native requested bound8MiB vs observed lifetimeRSS baseline31.06MiB/candidate30.74MiB distinct/uncalibrated.

Hard global reference limit256 original triangles remains (separate from64-cell cavity/256-cell donor bounds and1MiB sidecar). Next implement scalable authority storage/search/admission, complete traffic/cache/private-wait/memory calibration and detail-OFF overhead, then prepare closed SGE correctness/repetition/scaling package. User permits studying/reusing existing MMG/refine/Omega_h/ParMmg/Tucanos operators where helpful, with file-specific license and matched-contract tests. No dependency/operator imported here. No owned job remains after terminal regressions/syntax; foreign FSI,AdapNoExt,ClusterResults and historic failures untouched. FullGoal1 ACTIVE, no curved/robust3DBL/physicalunsteady/affordability claim.

## Persistent manufactured restart checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessorb936b29e160905e070cab5ca86ff7618bea135a4.
RESTART_REPORT.md / RESTART_CHECKPOINT_VALIDATION.json qualify eight real SU2/CGNS
mesh/reference/two-file BDF2 restart cases N1M1,N2M2,N4M2,N4M4. Refine, write
currentstep2/olderstep1, destroy driver, actual RESTART_ITER3 constructor and
Preprocess(3), then coarsen/refine; no manual restarted-history initialization.
Original sidecars stay byte-identical, pending authority publishes only on accepted
replacement. Fresh sensor-only originalP1 source per next event; complete frozen
sensor-source persistence is still unproven. Wedge target/guards/default512 unchanged.
Binary checkpoint state residual0, current/BDF manufactured residual<=3.845e-16,
transported/reloadedtensor0; exact geometry/marker/incidence/q/J/L passes all events.
Each restart checker rejects4 state/file corruptions, plus existing6 wedge controls.
No new runtime missing-sidecar fault injection claimed. Disabled control13rawfiles
byte-identical to wedge control. Complete68-object configured regression passes
65native/reference/2D+3Dbridge cases per MPI1/2/4 rank plus serialMMG48assertions.
Production archives/thin members unchanged, all pins stable; no numerical failures.
826closed phase rows; restart write/teardown/reload separate from event parent.
Fixture write includes bounded state checks, teardown includes config serialization.
Single contended pilots/different resumed trajectories, no speedup/scaling/private
balance claim. Requested native bound8MiB, lifetimeRSS34.77MiB distinct/uncalibrated.
Inspectable VTU, adapted SU2/CGNS/sidecars, actual binary histories and states in
integration_evidence/native_3d_core_v1/restart_{su2,cgns}_n2_n{1,2,4}_m{1,2,4}_v1.

FullGoal1 ACTIVE: larger first-failure/size-rank limits, complete traffic/wait/private
CPU/memory calibration, overhead and closedSGE repeated/scaling remain. No PDE
iteration/curved/3DBL/fullunsteady/affordability claim. No owned job remains after
regression. Foreign FSI, AdapNoExt, ClusterResults and historical failures untouched.
Implementation/evidence checkpoint `425c32f7480f989e4967c4dd79c1ee7e6e1f6c70` pushed to
origin/codex/native-3d-core; full remote SHA verified.

## Oblique wedge checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor8acadb1bf67bba237cf245acc4f2d7774cf3c65f.
Implementation/evidence checkpoint `413f02c899031a216fb3ca64eaa9983938950704` pushed to
origin/codex/native-3d-core; full remote SHA verified.
WEDGE_REPORT.md/WEDGE_CHECKPOINT_VALIDATION.json qualify four actual default512
n2 triangular-prism three-event cases N1M1,N2M2,N4M2,N4M4, plus13 byte-identical
box-control files. Input18points/24tets/32triangles, exact volume1/2 and oblique
x+y=1; six finite marker patches, bottom junction x=.5. OriginalP1 rotated/
spatial-direction sensor unchanged; physicalq/J/L guards.20/.05/1.8 unchanged.
Wedge qmin.321433/Jmin.152513/Lmax1.793063, sensor/loadedtensor0,
current/BDF2<=3.845e-16. Each audit rejects6 corruptions incl oblique vertex/name.
Accepted tets N1 46/9/38,N2M2 46/9/36,N4M2 46/14/53,N4M4 46/14/46.
Oblique triangles input8 ->16 ->4 ->10 (N1/N2), ->16 ->6 ->14 (N4);
adapted boundary connectivity preserved exact facets and marker junctions.
Actual SU2CGNS output/readback independently passes; fullsensor persistence open.

wedge_regression_v1 complete68-object actual driver passes64native/2D+3Dbridge
cases MPI1/2/4 and serialMMG2D/3D48assertions. All pins stable, production archives
and thin members unchanged from thin checkpoint. Default seven-marker helpers
also pass six retained thin/box SU2CGNS pairs and reject mismatched wedge markers.
No production/2D/MMG edits. Wedge helper n4 supported but not qualified here.
408closed phase rows/useful edit rates retained. One contended sample per changed
trajectory; no speedup/scaling/private balance claim. Native requested bound8MiB,
lifetimeRSS28.96MiB distinct, calibration incomplete. No ownedjob after regression.

FullGoal1 ACTIVE: persistent manufactured restart/larger first-failure limits,
complete traffic/wait/private CPU/memory calibration, overhead and closedSGE
repeated/scaling packages remain. No 3DBL/curved/fullunsteady/CFD affordability
claim or overnight clusterlaunch request. Foreignwork/AdapNoExt untouched.

## Affine thin-domain checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor58b6b40fb046f130be4eef1fa03c4b9104763f47.
Implementation/evidence checkpoint `df5bcf48e758c9845ffea6bcf0dc7e17ab2dc355` pushed to
origin/codex/native-3d-core; full remote SHA verified.
THIN_REPORT.md and THIN_CHECKPOINT_VALIDATION.json qualify six actual default512
three-event cases: zero-height-change control13files byte-identical to direction
baseline; h=2^-10 on N1M1,N2M2,N4M2,N4M4 and h=2^-20 serial. Production archives/
thin members unchanged. Test-only affine tensor pullback preserves physical q/J/L
.20/.05/1.8 checks; tensor residuals in physical and reference units both0.
Current/BDF2 residual<=3.861e-16, qmin.345975/Jmin.150717/Lmax1.799380104.
Thin cases reject7 corruptions each, including1% normal tensor/schema controls.
Accepted cells86/14/85 serial; N2M2 86/13/91; N4M2 86/12/79; N4M4 86/8/73.
Actual SU2CGNS output/readback plus exact geometry/marker/embedding pass.

thin_regression_v1 actual68-object driver PASS64native/2D+3Dbridge tests at
MPI1/2/4 and serialMMG2D/3D48assertions. All pins stable; no unmatchedselectors.
Retain controlv1 independentPASS/exactcomparisonFAIL (multiply-by-one fast-math
association), controlv2 compileFAIL (Tensor named members), finalcontrolv3PASS;
original/revised matrices and diagnoses preserved. No gate or production change.
442closed rows and per-event useful rates in thin_rank_phases.csv/validation;
one contended pilot each, different N/M trajectories, no speedup/scaling claim.
Requested native bound8MiB/lifetime RSS29.74MiB distinct; calibration still open.
Grids/states in thin_h10_* and thin_h20_n1_v1; no ownedjob after regression.

FullGoal1 ACTIVE: wedge/persistent manufactured restart/larger limits,
communication/wait/private CPU/memory calibration, overhead and closedSGE
repeated/scaling packages remain. No BL-generation/curved/unsteady/affordability
claim or overnight clusterlaunch request. Foreignwork/AdapNoExt untouched.

