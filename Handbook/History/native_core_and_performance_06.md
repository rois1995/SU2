## Spatial metric-direction envelope — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor8b64fe7dba33b62612065009981f1cae660952e1.
Implementation/evidence checkpoint `ed79f20caf9708809722db5d0b871c71a495000f` pushed to origin/codex/native-3d-core; full remote SHA verified. Test-only --direction1 prescribed
spatially rotating SPD tensor with azimuth45–90deg/elevation0–30deg, strength1+.125x,
aspect1.5, scales4/1/4; freeze original sensor P1 per event. Production unchanged.
DIRECTION_REPORT.md/VALIDATION.json qualify five actual default512 driver cases,
408closed accounting rows, current/BDF2/SU2CGNS and exact geometry/metric audits.
DirectionN1/M1 86/14/85tets, N2/M2 86/13/91, N4/M2 86/12/79, N4/M4 86/8/73 PASS.
qmin.345975/Jmin.150717/Lmax1.7993801; tensors/loadedtensors0, state<=3.84415e-16.
Legacy control direction_control_a2_n2_n1_v2 retains11 byte-identical datafiles.
Final direction_regression_v1 complete68objects passes64native/2D+3Dbridge tests
at MPI1/2/4; MMG2D/3D round trips serially (48assertions), no unmatchedselectors.
Existing324 independent automatic frames use unchanged production identities;
not rerun for fixture-only edits. Source/object/archive/binary pins stable.

Retained direction_a15_n2_n2_m2_v1 FAIL independentreload: final frozenP1
Lmax1.7993801, analytic-resampled reloadtarget1.8146257. Rootcause oldhelper
changed nonlinear target during reload; no productionmesh or guard error.
Helper now fetches transported nodes from acceptedSlices and saves separate
reload_nodal/reload_target records; independent nodal topology/SPD/P1+frozen-target
geometry checks pass SU2CGNS, including missing/corrupt-source negatives.
Actual filenames *_reload.json (nodal), *_reload_target.json (originalfield).
This retains oldfield in memory, does not prove relinearized sensor interior
admissibility or full frozenfield persistence in mesh files; actualrestart open.
Initial serialv1 and both default controlv1/v2/failure kept; new serial/MPI2v2 pass.
Allnine adaptation/source/state datafiles from failedMPI2v1 and passedv2 identical.

Parent pilotsN1 .510/.549/.448s, N2 .535/.621/.399, N4M2 .542/.617/.331,
N4M4 1.298/1.149/.699. Different trajectories/singlecontendedruns do not prove
scaling. Parentmax/mean near1 does not prove private CPU balance or no MPI waits.
Requestedbound8MiB/lifetimeRSS31.3MB distinct, full calibration incomplete.
AcceptedVTU/SU2/CGNS/states in direction_a15_* folders, no ownedjob after audits.

FullGoal1 ACTIVE: thin/wedge/restart/larger limits, complete wait/traffic/private
CPU/memory calibration, overhead and closedSGE repetitions/scaling remain.
No actualCFD/fullBL3D/unsteady/affordability claim or overnight cluster request.
Foreignwork/AdapNoExt/submodules/ClusterResults unchanged.

## Stronger anisotropy envelope — 2026-10-11

NativeIntegrated / codex/native-3d-core; tested production unchanged from donor
implementation a406a1281a7d7bd16d45c88070737c82b1347897, prior remote tip verified
157750a75bece663540c36e1ee6d68fbed07ce06. ROBUSTNESS_EXPANSION_MATRIX.json was frozen
before runs. ROBUSTNESS_STRENGTH_REPORT.md/VALIDATION.json qualify actual default512
aspect2, n2 box, fixed45-degree eigenframe and1+0.125x strength, three events:
robustness_a2_n2_n1_v1 PASS128/17/79tets;
robustness_a2_n2_n4_m2_v1 PASS128/16/109tets including return to four CFD ranks.
Independent geometry/marker/embedding/P1/current+BDF2/SU2CGNS audits PASS;
qmin.246842/Jmin.0870495/Lmax1.798437, tensorerror0/state<=3.81218e-16.
Source/object/archive identities stable, exclusive phases close. VTU/SU2/CGNS
and states in both named folders. No owned heavy job live after final audits.

Parent pilot secondsN1 .815206/.970013/.328394, N4M2 .978312/1.442031/.376798;
useful rates77.28/40.21/182.71 and64.40/26.35/138.00edits/s. Different trajectories,
one nice19/thread1 pilot each with foreignFSI; no scaling/speedup/affordability
claim. Maximum bound8MiB/lifetimeRSS31.1MB distinct, not fullmemory calibration.
Initial analyzer dictionary equality failed because full driver lists three
additional system archive aliases; runtime production archives/thin members match.
Corrected using existing donor verifier rule, retained originalscript/failure.
No run/receipt/guard/production edit.

Next: add frozen genuinely spatial direction-change fixture (currentvarying means
strength only), thin/wedge/restart and size limits, then complete wait/traffic/
memory calibration and closedSGE overhead/repeated/scaling packages. FullGoal1
ACTIVE; no physicalCFD/fullBL3D qualification or overnight clusterlaunch request.
Foreignwork/AdapNoExt/submodules/ClusterResults unchanged.

## Native3D donor reuse and certified segment filter checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor89225cf0cbc9215fd92080638274c602c5bd9005.
Implementation/evidence checkpoint `a406a1281a7d7bd16d45c88070737c82b1347897` pushed to origin/codex/native-3d-core; full remote SHA verified. DONOR_REUSE_REPORT.md and
DONOR_REUSE_CHECKPOINT_VALIDATION.json qualify four actual default512 driver
cases (12 events),44 byte-identical geometry/source/state/readback files,
four unchanged rejected-replay files,340 closed phase rows,324 independently
audited automatic frames and nine negative controls. All final receipts PASS;
source/object/archive identities stable. No owned process live after final audit.

DonorImport reuses an immutable donor already selected in this caller union,
using the existing bounded map; no persistent cache/new heap or payload change.
TraceSegment validates finite positive donors then rejects only certified strict
same-face endpoint negatives. Unresolved/touching/crossing intervals retain exact
rational roots/P1/coverage/overlap checks. New donor_reused/edge_separated counts.
No target/geometry/operator/BL/sensor/fade/noise/2D/MMG production changes.

Serial alignedn3 default288/50/312tets: secondcoarsen482/66/416,2717requests/10hits
unchanged;125165 donor tests and21750 clips skipped. Singlecontended engine
4.10262->2.61079s, donorfilter1.41236->0.31636, edgeTrace2.14006->1.76457.
Isolated1547chord comparison preserves all exact interval/weight values;
three alternating samples per3sets show lower medians29.5/21.9/36.4%, but exclude
import/ADT/MPI/CFD. No repeat-qualified total-speedup/scaling/affordability claim.
N4M2/N4M4 default288/47/277 and288/51/264; originaln2N1 88/36/94.
qmin.202935/Jmin.062176/Lmax1.799641; tensorerror0/state<=5.80681e-16.
Originaln3 replay175rounds57privateaccepted/42insertions,420tets remainsrejected;
acceptedCFDstate and allfour original/rejected datafiles preserved.

Verification correction: legacy [MMGRoundTrip] selector did not match registered
[MMG] tests, so historical runner PASS does not prove MMG execution. Correctedv3
passes serial but fails MPI2 because direct MMG fixtures call master-only SetMesh
on empty nonmaster extraction. Failure retained, no production/fixture edit.
Final regression_v4 actual68objects passes62native/2Dbridge tests at MPI1/2/4;
actual2D/3D MMG round trips runserially (48assertions). Unmatched selectors fail.
Probe preflightv1FAIL and superseded exact-signv2 mixed timings retained;
latest production/qualified runtime matches probev3 and actualN1_v2.

FullGoal1 ACTIVE: stronger/direction-changing/thin/wedge/restart/practicallimits,
complete wait/traffic/memory calibration, overhead and closedSGE repetitions/
scaling remain. AcceptedVTU/SU2/CGNS and states in donor_reuse_* casefolders;
rejectedSU2 in donor_reuse_replay_original_n3_v1. No CFDiterations/fullBL3D claim.
No overnight clusterlaunch request; foreignwork/AdapNoExt/submodules unchanged.

## Native3D exact coarsening filter checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessorf05ca3a8828d56619bdcafc763cbe4c8facae1d0.
Implementation/evidence checkpoint `80a4a153da2a3945666661c15c1f6fdb3381da8f` pushed to origin/codex/native-3d-core; full remote SHA verified. COARSEN_FILTER_REPORT.md and
COARSEN_FILTER_CHECKPOINT_VALIDATION.json qualify four actual default512 driver
cases (12 events),340 closed phase rows, complete68-object MPI1/2/4 filter/
coplanar/insertion/automatic repair/budget/default/native2D/MMG regression,
324 independent automatic frames and9 negative controls. Source/library/object/
artifact pins agree. No owned process live after the final audit.

Directed boundary collapse counterexamples use exact immutable finite-facet
Contains, cached in per-cell bits/per-edge counts, updated with mesh edits.
Filter enables only once globallyadmissible; invalid-field collapse repair is
preserved. Counters report facet checks and queued directions actually removed,
not all suppressed future enqueues. Extra storage charged by mapmodels; nested
geometry timer belongs to seed/update. Local witnesses can miss remote-only
obstructions (safe underfilter), full imported validation remains authoritative.
Successful vote preserves local rejection reason; v2 tests invalid-field no-prune
and nonblank exact geometry reason, failure timing return moved outside scope.

Serial alignedn3 full default512 nowpasses288/50/312tets. Second coarsen482
rounds66accepted416rejected versus old diagnostic619/66/553, seven initial/first
accepted/state/sidecar/secondinput files byte-identical to faileddefaultbaseline.
Exactpredicate2933, queuedpruned443, nestedgeometry0.03144s. Fieldrequests2717/
hits10 unchanged. Singlecontended enginewalls4.18517old/4.10262new do NOT prove
speedup. Nestededgetrace2.14006 and donorfilter1.41236 remain dominantquestions.
N4M2 defaultpasses288/47/277; N4M4 passes288/51/264. Originaln2 N1 passes88/36/94.
Qualifiedqmin0.202935/Jmin0.062176/Lmax1.799641, tensorerror0/state<=5.80681e-16.

Inspectable grids: integration_evidence/native_3d_core_v1/
coarsen_filter_aligned_n3_n1_v1/envelope_s0.vtu through envelope_s2.vtu,
plus N4M2/M4 folders, SU2/eventsidecars/finalCGNS. All historic failures retained.
No CFDiterations run. Current reusable runtimebinary lives in this newN1folder.

Next: address measured exact donor clipping/edge tracing without compromising
source coverage/P1/metric/geometry guards. Continue stronger/direction-varying/
thin/wedge/restart/larger practical limits, full wait/traffic/memory/calibration,
instrumentationOFFON/repeated scaling and closed SGE packages. FullGoal1 ACTIVE;
no overnight clusterlaunch question, no speedup/CFDaffordability/full3D claim.
Foreignwork/AdapNoExt/submodules/ClusterResults remain untouched.

## Native3D cavity insertion checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor16029e00cba59c54a05c71eb320c4833845762a2.
Implementation/evidence checkpoint `128c9e58e267462b2761804df44708048f359216` pushed to origin/codex/native-3d-core; full remote SHA verified. INSERTION_REPORT.md and
INSERTION_CHECKPOINT_VALIDATION.json qualify nine actual manufactured driver
cases (27 events), 884 accounting rows, complete configured68-object MPI1/2/4
regression and324 independent automatic repair frames plus9 negative controls.
Six originaln2 N/M controls preserve66 predecessor geometry/source/state files
byte-for-byte. Source/object/archive/artifact identities verified.

Bounded InsertEdgeCavity removes exact-placement obstacles by positive coning
with unchanged interior skin or a coupled same-canonical-planar-facet surface
fan. Marker junctions/incomplete stars reject. MPI ownership routing and inserted
statistics reconcile arbitrary fans; existing metric/quality/geometry/atomic
checks, default512 and all2D/MMG/BL/sensor/fade/noise policies stay unchanged.
Direct unit/thin1e-8 insertion/junction/rollback tests pass. Automatic324-frame
audit covers existing repair paths, not insertion itself.

Larger alignedn3 defaultN4M4 passes288/61/308tets; defaultN4M2 passes288/47/281.
Serialdefault accepts first288tets then rejects second coarsening at512;
diagnostic explicit1024adapter passes288/50/312 after619 coarsening rounds
(66accepted/553rejected). Seven common input/first accepted mesh/state/sidecar/
secondinput files identical; full default serial3events still fail.
Originaln3 replay remains rejected:175rounds,57privateaccepted/42insertions,
118rejected,420tets121points,qmin0.554689/Jmin0.334497,6longedges12cells,
Lmax2.229036; geometryvolume1 and accepted CFD state preservation pass.
Historical infeasible original marker junction remains intact.

Inspectable accepted grids: integration_evidence/native_3d_core_v1/
insertion_aligned_n3_n4_m2_v1/envelope_s0.vtu through envelope_s2.vtu,
SU2 meshes/reference sidecars and finalCGNS alongside. Rejected original grid:
insertion_replay_original_n3_v1/replay_rejected.su2. No CFD iterations run.
Single contended pilots/different trajectories establish no speedup/scaling;
serial coarsening candidate failures dominate the next optimization question.

Next: geometry counterexamples can prune impossible directed boundary collapses
only when the current frozen field is globally admissible, preserving useful
invalid-field repair fallback and strict/default completion. Fix successful
vote clearing local rejection reason separately. Both are unimplemented designs.
FullGoal1 ACTIVE: complete wait/traffic/memory/overhead/repeated scaling,
stronger/direction-varying/thin/wedge/restart/practical limits and closedSGE remain.
No owned case live; no overnight clusterlaunch request. Foreignwork unchanged.

## Native3D recovery coordination and placement diagnosis — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor a410dad7a47bee500d9ccc2d32d8d08250937470.
Implementation/evidence checkpoint `3b84e6a3b9ba9ae1467da4f556dacab84e3ef073` pushed to origin/codex/native-3d-core; full remote SHA verified. COORDINATION_REPORT.md,
COORDINATION_CHECKPOINT_VALIDATION.json and coordination_rank_phases.csv bind
six N/M actual driver controls (18 refine/coarsen/refine events, original-P1
current/BDF2 transfer and SU2/CGNS output/readback) plus configured68-object
MPI1/2/4 automatic repair/coarsening/single-recovery/budget/default/2D/MMG checks.
All PASS; source/object/archive identities stable. 578 exclusive/nested rows.
qmin0.258663/Jmin0.062176/Lmax1.785100; tensor error0/state3.84415e-16.

Production Refine now collectively retries unblocked splits while retaining its
single dependency-tracked recovery queue; retained repair storage is charged
in split rounds. One extra scalar collective per incomplete recovered round.
Default512, source/geometry/quality/metric and 2D/MMG policies unchanged.

Original n3 still rejects:512 rounds64 privateaccepted/448 rejected;1024 allowance
stagnates at631 with64 accepted/567 rejected. Same private240tets/81points,
qmin0.201655/Jmin0.091521,22longedges/53cells/Lmax2.464292. Geometry/volume1
independent checks and accepted CFD state preservation PASS. Inspect rejected
meshes in coordination_replay_r512_v1 and coordination_replay_r1024_v1.

placement_certificate_v2 proves14 remaining edges have no binary64 interior
point; all22 rounded midpoints are off-edge. Original marker junction9–14
(facets30/31, z_minus_b/z_minus_a) is one certified edge, L2.18657>1.8.
Strict exact junction preservation and length completion are incompatible for
that segment. Interior certificates only establish an operator-placement limit;
retriangulation can remove interior edges. No silent geometry tolerance change.
Nonconvex same-marker raw seams are a separate geometry limitation.

Next: preserve originaln3 as feasibility/rejection evidence, add an aligned
physical marker fixture and bounded representable cavity insertion/reconnection
using existing Cone/fixed-skin certification, including MPI routing and coupled
surface checks. Full Goal1 ACTIVE: larger/direction-changing/thin/wedge/restart,
full wait/traffic/memory calibration, overhead/repeated timings and closed SGE
remain. Single contended pilots/different trajectories prove no speedup or CFD
affordability. No owned case live after finalN4M4 terminalPASS; no overnight
cluster launch questions. Foreign FSI/AdapNoExt/submodules/ClusterResults untouched.

## Native3D exact rejected-private replay — 2026-10-11

NativeIntegrated / codex/native-3d-core; predecessorce942b5d6fba97985577b2416e1d4256018b6e7c.
REPLAY_REPORT.md / REPLAY_CHECKPOINT_VALIDATION.json and frozenREPLAY_MATRIX.json
retain exact original n3 input/import/sensor match and bounded512/1024 direct
Refine probes. No production change.512 replay exactlymatches original backend:
59 privateaccepted/453 rejected,18moves/26reconnections,229tets,79points,
qmin0.267933/Jmin0.146700,25longedges/65cells, Lmax2.464292.1024 stillrejects:
79accepted/945rejected,231tets,21longedges/51cells, qmin0.211317/Jmin0.098595,
sameLmax. Both independentpositivevolume/nooverlap/incidence/finitefacet/marker/
volume1 checks pass; accepted CFD mesh/solution/reference/source unchanged.

Inspectable private diagnostics (never accepted CFD grids):
 integration_evidence/native_3d_core_v1/replay_size3_r512_v2/replay_rejected.su2
 and replay_size3_r1024_v1/replay_rejected.su2. Audits list everylongedge with
coordinates and incidentcells.512 enginewall8.046s/exclusivemetric6.425s;
1024wall12.943s/metric10.308s. Nestededge tracing overlapsmultiplephases;
singlecontendedruns, no scaling claim/usefulaccepted CFD edits for rejectedevent.

Initialrunner syntaxfailure retainedbeforebuild. Firstreplayfixturev1 crashed
querying uninitialized driver native reference afterRefine: fixcreatesBackend
before queryingReference, onlytestcode. v2 all17Cppassertionspass; initialPython
raw/derivedfacetcomparisonfailure retainedwithhash-bound correctedwhole-raw-input
comparison recovery, no remeshingrerun.1024 terminalPASS and ordinary runner
controlPASS:11 mesh/source/state files identicalto published N1varyingcase.
Allproduction archives/members equalreader checkpoint; compactlogs/grids/pins.

Next: production Refine currentlylocksinto repairmode untilwholefieldpasses,
suppressing newlyunblocked splits after acceptedrepairs. Test alternating available
splits while retaining a single seeded/tracked repairqueue, dependency blocking,
512/defaultstoragebounds and alltarget/geometry/atomicguards. Separate operation
selection from repairtracking, keep globalMPIoperationchoice consistent. Coarsen/
pureRepair/current<=1recovery tests must remain valid. This is a control-flow
hypothesis, not yet an implemented/provedfix. Exact segmentclipping hotspot remains
separate; preserve tiny/tied/overlap/invalidsource checks if filtering is improved.

FullGoal1 ACTIVE; noownedcase live after ordinarycontrol terminalPASS. Full MPI
wait/traffic, memory calibration, stronger/directionchanging/thin/wedge/restart
and practicalrank/size limits, repeatedOFFON and closedSGE remain. Noovernight
clusterlaunchquestions; foreignwork untouched. Implementation/evidence checkpoint 2463e523c2827a5182139fcdd4a71ff7083053fe pushed
to origin/codex/native-3d-core; full remote SHA verified. This confirmation
follows that publication.

## Native 3D anisotropic driver envelope and first size limit — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessord41c49de169444c6053674860da0285b6347fe18.
ENVELOPE_REPORT.md / ENVELOPE_CHECKPOINT_VALIDATION.json retain frozen initial
matrix, actual complete-archive fixtures/pins, seven qualified cases, and first
162-tetrahedron rejection. No production code changes; archives/members equal
pushed reader checkpoint. N1M1/N2M1/M2/N4M1/M2/M4 varying-strength45deg metric
passes three actual refine/coarsen/refine and ReplaceMesh/BDF events each; also
constant metric control.56 geometry/readback and21 transported-state frames,
234 exclusive+378 nested rows pass. Tensorerror0/stateerror3.84415e-16. Different
partition trajectories independently qualified. Original N2M1 timing-only FAIL
(-4.3368e-19 CPU remainder) kept with hash-bound existing-tolerance recovery.
Legacy helper/default sensor/native2D/MMG MPI1 control passes;84 old stateframes
re-audited. Actual SU2/CGNS/readback and inspectable VTUs with synthetic histories.

Size3 N1M1 first event rejects after512 rounds/59 privateaccepted/453 rejected.
Accepted CFD mesh/solution/reference preservation checks pass; source/input/logs
and FAIL receipt kept. Remesh7.917s/refine7.823s, exclusivemetric6.234s and
reconstruction0.806s; nestededge trace6.496s overlaps phases. No proved cycling
or larger-budget fix. Stop size4/a2 expansion until exact rejected-private replay
and feature/residual diagnostics explain budget versus operator blockage.
Private rejected progress not exported yet. Passing grids:
 integration_evidence/native_3d_core_v1/envelope_mpi4_m4_v1/envelope_s0.vtu
 throughs2.vtu, actualdriverSU2/eventsidecars and finalCGNS nearby.

Full Goal1 ACTIVE. No owned case live after legacy control terminal PASS.
Next exact failure replay, direction-changing/thin/wedge/restart/practical limits,
full wait/traffic and memory calibration; repeated OFFON and closed SGE package
still required. One contended pilot is not scaling/overhead/affordability proof.
Do not ask overnight cluster launches. ForeignFSI/submodules/AdapNoExt/
ClusterResults untouched. Implementation/evidence checkpoint dd673805245e4946bc995ba193a986f418299155
pushed to origin/codex/native-3d-core; full remote SHA verified. Two redundant
terminal-PASS test binaries removed after identical-hash verification, reclaiming
1,217,722,368bytes; qualified shared binary remains in envelope_mpi2_m2_v1.
ENVELOPE_DISK_COMPACTION.json preserves receipts; no numerical evidence removed.

## Native 3D reader admission validated — 2026-10-11

NativeIntegrated / codex/native-3d-core; predecessor
4c9f56b8c861433158e58459ee0d5c157742bb9b. READER_REPORT.md /
READER_CHECKPOINT_VALIDATION.json: new recoverable requested-storage reader gates
cover validation/point/signature/linear/volume/root-boundary buffers; native
assembly maps/vectors and old output included. Targets, original sensor-only P1,
actual-query composition and legacy2D/MMG unbudgeted calls preserved. No new CFG.
reader_runtime_v1 interrupted initial duplicate-block build retained; corrected
reader_runtime_v2 terminal PASS42 stages/session79225exit0. All production/test
pins unchanged during full check.84 main+36 computed-metric accepted events,
42 OFFON/42 SU2CGNS;126 mesh frames/84 transported-state frames/36 computed-metric
frames identical to predecessor.357 closed rank-stage and357 nested records.
Native2D/MMG/default/rollback/refusal regressions pass. reader_skew_v1 terminal
PASS11 stages/session1666exit0; only test source changed, all production archives/
members equal full-run pins. All-key-to-root skew, late linear-point refusal/
retry and seven rank bounds pass; N4 root18,258B vs others10,419B. No owned job live.

Largest main model8MiB, main lifetimeRSS37,904,384B, separate quantities. Budget
success adds three count Alltoalls/nine scalar Allreduces within existing reader
stage; full house/vendor traffic/wait still open. Matched3-repeat phase timings
saved but foreign FSI variability prevents isolated overhead/speedup claims.
Native tree/vector bounds exclude allocator/MPI/ParMETIS/CFD internals; cache
scratch calibration and practical envelope remain. Goal1 ACTIVE: next complete
wait/communication accounting, larger/rotated/thin/restart and rank/size pilots,
then closed SGE campaigns without overnight launch requests. Unsteady sampling
detail also open. Inspect reader_runtime_v2/lifecycle_mpi4 and mpi4 SU2/CGNS plus
metric/current/BDF/source JSON. Implementation/evidence checkpoint
c8c6ffaf9121b19463cd8ca8e6f528e3f269d9bf pushed to origin; full remote SHA
verified with git ls-remote. This handoff confirmation follows that publication. Foreign submodules,
FSI/AdapNoExt/ClusterResults untouched.

## Native 3D lifecycle profiling validated — 2026-10-11

Working NativeIntegrated / codex/native-3d-core; predecessor
 d5df2785676e610b2dcaec2c5aee7ea93ceb9670. PROFILE_REPORT.md and
PROFILE_CHECKPOINT_VALIDATION.json retain exact scope/provenance. Opt-in3D
per-rank exclusive wall/process CPU/lifetimeRSS records cover actual metric/
Hessian build, CFD ParMETIS/return migration/geometry preprocessing, solver/BDF
replacement and requested mesh/reference output. Nested source/partition/engine/
query counters exported. No targets or operators changed; 2D/MMG gate disabled.
profile_build_v1 compile failure and profile_runtime_v1 invalid equal-size CFG
failure preserved; corrected profile_build_v2 and final42-stage profile_runtime_v2
PASS. Final actual test objects recompiled for outer timing/config correction.
Session25990 terminal0, no owned job live. Source/archive/object pins unchanged.
Main84 accepted events/42 OFFON controls/42 SU2CGNS roundtrips;126 predecessor-
identical mesh/source/transported-state frames. New actual-computed-sensor fixture
adds36 accepted events, three fresh repeats per M1/2/4<=N/OFFON;36 strict frames,
357 closed rank-stage rows and357 nested rows, parent containment/adversary checks.
Native2D/MMG/default ownership/private-progress rollback/budget regressions PASS.
Phase CSV and lifecycle_phase_summary.json retained; summarize_lifecycle.py
regenerates them. New fixture C++ affine current/BDF checks; no CFD iteration.

N1/M1 ON sample remesher62.54ms: refine45.48/coarsen14.83; within refinement
metric17.49/reconstruction6.80ms. N4M4 high contention/spread; ON/OFF ratio1.177
cannot establish instrumentation penalty or scaling. Stage own emission excluded,
parent includes nested emission; outer testcase includes test metric observation,
output separately timed. Do not sum stage maxima or infer MPI wait from wall-CPU.

Full Goal1 ACTIVE. Next: complete MPI wait/house traffic and reader/ParMETIS
memory calibration; practical larger/rotated/thin lifecycle/restart and rank/size
limits; then closed SGE campaigns. WINDOW_AVERAGE/PREDICT sampling detail also
not fully covered. No full-memory/scaling/BL/unsteady affordability claim. New
cases profile_runtime_v2/lifecycle_mpi4; transported metrics/states and CGNS in
profile_runtime_v2/mpi4. Implementation checkpoint
 de21d8a462de57ac4f2ba5b96b9418922dbb7e79 pushed to origin and full remote SHA
verified; this handoff update follows that publication. No cluster-launch
questions tonight; foreign FSI/AdapNoExt/
submodules/ClusterResults untouched.

## Native 3D memory checkpoint validated — 2026-10-11

Repository NativeIntegrated; branch codex/native-3d-core; remote predecessor
0864f56e60f0f7d66e5db6cf8779cb8aaa1703c7. MEMORY_REPORT.md and
MEMORY_CHECKPOINT_VALIDATION.json retain scope/pins. memory_build_v1 and
memory_runtime_v1 terminal PASS; session95453 finished, no owned job live.
New budget refusal/retry tests PASS MPI1/2/4. Actual driver matrix:84 accepted
events,42 OFF/ON pairs,42 SU2/CGNS roundtrips;126 mesh/readback signatures and
transported states identical to predecessor. Sensor error0; max affine BDF2
error1.119683e-12; qmin.628068/Jmin.397369/Lmax1.793742. Native2D/MMG/default
ownership and refusal-after-private-edits regressions PASS MPI1/2/4; raw default
mesh/reference audits PASS.336 runtime rank rows include modeled bytes and
process-lifetime peak RSS; largest model8MiB vs matrixRSS38,666,240bytes,
separate quantities, not an allocation certificate. No source/archive/object
changes during checks. Implementation checkpoint
7f004951f080013d82c9fda5d8cd8bc4043f58d4 pushed to origin. Publication tip
64b13aa2f0ff0006e9b134cf29b55b2abfa16f4b verified with git ls-remote; this
handoff confirmation follows that published implementation/evidence.

Full Goal1 remains ACTIVE. Initial import/caller-held storage gates now cover
point directory, face exchange, original authority and retained runtime buffers;
redundant working maps/source/operation records released. Reader/partitioner
internal memory and existing reader/cache model calibration remain open. Next:
close metric/import/query/partition/return/rebuild/transfer per-rank lifecycle
profiling, then restart/larger/rotated cases and repeated practical envelope.
No speedup, full memory or affordability claim; no cluster-launch questions tonight.
Inspectable memory_runtime_v1/mpi4/runtime_barycentric_m4_d0_mesh_00003.su2,
adjacent sidecar, s2_written_CGNS.cgns and s2_states.json. Foreign FSI,
AdapNoExt, submodules and ClusterResults untouched.

